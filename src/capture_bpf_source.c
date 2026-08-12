/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file capture_bpf_source.c
 * @author Jurijs Ivolga <jurijs.ivolga@gmail.com>
 *
 * @brief libpcap side of the eBPF capture source
 *
 * Captured plaintext is wrapped in a synthetic Ethernet, IP and TCP frame and
 * handed to parse_packet(), the same callback libpcap drives. That reuses TCP
 * reassembly, WebSocket detection, filtering, the user interface and pcap
 * output rather than reimplementing any of it.
 *
 * This file exists separately from capture_bpf.c because libpcap and libbpf
 * both define struct bpf_insn, and neither guards against the other, so no
 * translation unit may include both.
 *
 */
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "capture.h"
#include "capture_bpf.h"
#include "capture_bpf_util.h"
#include "util.h"

//! Capture source registered with the capture layer
static capture_info_t *bpf_capinfo = NULL;
//! Synthetic TCP sequence numbers, one counter per direction
static capture_bpf_seqtab_t *bpf_seqtab = NULL;
//! Nanoseconds to add to a boot relative timestamp to reach epoch time
static uint64_t bpf_boot_epoch_ns = 0;

int
capture_bpf_source_create(void)
{
    struct timespec rt, bt;

    // BPF timestamps count from boot, sngrep wants wall clock. The offset is
    // computed once, so eBPF packets stay ordered against any pcap source
    // running alongside.
    clock_gettime(CLOCK_REALTIME, &rt);
    clock_gettime(CLOCK_BOOTTIME, &bt);
    bpf_boot_epoch_ns = ((uint64_t) rt.tv_sec * 1000000000ULL + rt.tv_nsec)
                      - ((uint64_t) bt.tv_sec * 1000000000ULL + bt.tv_nsec);

    if (!(bpf_seqtab = capture_bpf_seqtab_create(CAPTURE_BPF_MAX_CONNS))) {
        fprintf(stderr, "eBPF: out of memory\n");
        return 1;
    }

    if (!(bpf_capinfo = sng_malloc(sizeof(capture_info_t)))) {
        fprintf(stderr, "Can't allocate memory for capture data!\n");
        capture_bpf_source_destroy();
        return 1;
    }

    bpf_capinfo->capture_fn = capture_bpf_thread;
    bpf_capinfo->ispcap = false;
    bpf_capinfo->isbpf = true;

    // parse_packet() derives payload offsets from the link header size, so
    // this source needs a datalink even though it never reads from a device
    bpf_capinfo->handle = pcap_open_dead(DLT_EN10MB, MAXIMUM_SNAPLEN);
    bpf_capinfo->link = pcap_datalink(bpf_capinfo->handle);
    if ((bpf_capinfo->link_hl = datalink_size(bpf_capinfo->link)) == -1) {
        fprintf(stderr, "Unable to handle linktype %d\n", bpf_capinfo->link);
        capture_bpf_source_destroy();
        return 1;
    }

    bpf_capinfo->tcp_reasm = vector_create(0, 10);
    bpf_capinfo->ip_reasm = vector_create(0, 10);

    capture_add_source(bpf_capinfo);
    return 0;
}

void
capture_bpf_source_destroy(void)
{
    if (bpf_seqtab) {
        capture_bpf_seqtab_destroy(bpf_seqtab);
        bpf_seqtab = NULL;
    }

    // bpf_capinfo itself belongs to the capture layer once registered, and is
    // released along with every other source by capture_deinit()
    bpf_capinfo = NULL;
}

void *
capture_bpf_thread(void *info)
{
    capture_info_t *capinfo = (capture_info_t *) info;

    while (capinfo->running) {
        if (capture_bpf_poll(100 /* ms */) < 0)
            break;
    }

    capinfo->running = false;
    return NULL;
}

void
capture_bpf_source_handle(const struct tls_event *e, size_t size)
{
    u_char frame[CAPTURE_BPF_MAX_FRAME];
    char key[CAPTURE_BPF_KEYLEN], revkey[CAPTURE_BPF_KEYLEN];
    struct pcap_pkthdr hdr;
    uint32_t seq, ack = 0;
    uint64_t ts;
    size_t framelen;

    if (!bpf_capinfo || !bpf_seqtab)
        return;

    // Guard against a truncated event rather than trusting the length field
    if (size < SNGREP_TLS_EVENT_HDRLEN || e->len == 0)
        return;
    if (size < SNGREP_TLS_EVENT_HDRLEN + e->len)
        return;

    // A payload the correlation could not place has nowhere to go
    if (e->family != 4 && e->family != 6)
        return;

    capture_bpf_conn_key(key, sizeof(key), e->family,
                         e->saddr, e->sport, e->daddr, e->dport);
    capture_bpf_conn_key(revkey, sizeof(revkey), e->family,
                         e->daddr, e->dport, e->saddr, e->sport);

    seq = capture_bpf_seqtab_next(bpf_seqtab, key, revkey, e->len, &ack);

    // PSH marks the last chunk of a message, so reassembly knows to flush
    framelen = capture_bpf_build_frame(frame, sizeof(frame), e->family,
                                       e->saddr, e->sport,
                                       e->daddr, e->dport,
                                       seq, ack, e->more == 0,
                                       e->data, e->len);
    if (framelen == 0)
        return;

    ts = e->ts_ns + bpf_boot_epoch_ns;
    hdr.ts.tv_sec  = (time_t) (ts / 1000000000ULL);
    hdr.ts.tv_usec = (suseconds_t) ((ts % 1000000000ULL) / 1000);
    hdr.caplen = (bpf_u_int32) framelen;
    hdr.len    = (bpf_u_int32) framelen;

    // parse_packet() takes capture_lock() itself, the same contract the EEP
    // capture thread relies on
    parse_packet((u_char *) bpf_capinfo, &hdr, frame);
}
