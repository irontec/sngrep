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
 * @file capture_bpf.c
 * @author Jurijs Ivolga <jurijs.ivolga@gmail.com>
 *
 * @brief eBPF based capture of SIP over TLS
 *
 */
#include "config.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <bpf/libbpf.h>
// Deliberately no util.h: it includes capture.h, and therefore pcap.h, whose
// struct bpf_insn collides with the one from linux/bpf.h
#include "capture_bpf.h"
#include "capture_bpf_util.h"
#include "setting.h"
#include "sngrep_tls.skel.h"

//! Loaded eBPF programs and maps
static struct sngrep_tls_bpf *bpf_obj = NULL;
//! Ring buffer consumer
static struct ring_buffer *bpf_rb = NULL;
//! Attached probes, one vector entry per bpf_link
static vector_t *bpf_links = NULL;
//! Distinct libssl objects found on the system
static vector_t *bpf_libs = NULL;

/**
 * @brief Route libbpf diagnostics away from the ncurses display
 */
static int
capture_bpf_print(enum libbpf_print_level level, const char *fmt, va_list args)
{
    // libbpf is noisy about conditions sngrep reports itself in plainer
    // words, so its own output is kept for when debugging is asked for
    if (level != LIBBPF_WARN || !setting_enabled(SETTING_BPF_DEBUG))
        return 0;

    return vfprintf(stderr, fmt, args);
}

/**
 * @brief Attach one program to @lib, recording the link so it can be freed
 *
 * A missing symbol is not an error: SSL_read_ex and SSL_write_ex do not exist
 * before OpenSSL 1.1.1.
 *
 * @return 1 if attached, 0 otherwise
 */
static int
capture_bpf_attach_uprobe(struct bpf_program *prog, const char *lib,
                          const char *func, bool retprobe)
{
    struct bpf_link *link;
    LIBBPF_OPTS(bpf_uprobe_opts, opts,
                .func_name = func,
                .retprobe = retprobe);

    // A pid of -1 attaches to every process mapping this library, including
    // processes that start after us
    link = bpf_program__attach_uprobe_opts(prog, -1, lib, 0, &opts);

    if (setting_enabled(SETTING_BPF_DEBUG))
        fprintf(stderr, "[bpf] attach %s%s: %s\n", func,
                retprobe ? " (return)" : "",
                link ? "ok" : strerror(errno));

    if (!link)
        return 0;

    vector_append(bpf_links, link);
    return 1;
}

/**
 * @brief Attach a kernel probe, recording the link so it can be freed
 *
 * @return 1 if attached, 0 otherwise with a message already printed
 */
static int
capture_bpf_attach_kprobe(struct bpf_program *prog, const char *func)
{
    struct bpf_link *link;

    if (!(link = bpf_program__attach(prog))) {
        fprintf(stderr, "eBPF: failed to attach %s probe: %s\n",
                func, strerror(errno));
        return 0;
    }

    if (setting_enabled(SETTING_BPF_DEBUG))
        fprintf(stderr, "[bpf] attach %s: ok\n", func);

    vector_append(bpf_links, link);
    return 1;
}

/**
 * @brief Hand one captured chunk to the capture source
 */
static int
capture_bpf_handle_event(void *ctx, void *data, size_t size)
{
    const struct tls_event *e = data;

    if (size < SNGREP_TLS_EVENT_HDRLEN)
        return 0;

    if (setting_enabled(SETTING_BPF_DEBUG)) {
        char s[INET6_ADDRSTRLEN] = { 0 }, d[INET6_ADDRSTRLEN] = { 0 };
        int af = (e->family == 6) ? AF_INET6 : AF_INET;

        inet_ntop(af, e->saddr, s, sizeof(s));
        inet_ntop(af, e->daddr, d, sizeof(d));

        fprintf(stderr, "[bpf] %s pid=%u %s ipv%u %s:%u -> %s:%u len=%u\n",
                e->comm, e->pid,
                e->direction == SNGREP_TLS_EGRESS ? "OUT" : "IN",
                e->family, s, e->sport, d, e->dport, e->len);
    }

    capture_bpf_source_handle(e, size);
    return 0;
}

int
capture_bpf_init(void)
{
    int i, attached = 0;

    libbpf_set_print(capture_bpf_print);

    // CO-RE relocates against the running kernel's BTF. Without it the load
    // fails with a relocation error that explains nothing.
    if (access("/sys/kernel/btf/vmlinux", R_OK) != 0) {
        fprintf(stderr, "eBPF: kernel lacks BTF support "
                        "(needs CONFIG_DEBUG_INFO_BTF, kernel 5.8+)\n");
        return 1;
    }

    if (!(bpf_obj = sngrep_tls_bpf__open())) {
        fprintf(stderr, "eBPF: failed to open programs: %s\n",
                strerror(errno));
        return 1;
    }

    // The prefilter lives in kernel space, so it can only be chosen before
    // the programs are loaded
    bpf_obj->rodata->sngrep_filter_enabled =
        setting_enabled(SETTING_BPF_FILTER) ? 1 : 0;

    if (sngrep_tls_bpf__load(bpf_obj) != 0) {
        if (errno == EPERM)
            fprintf(stderr, "eBPF: permission denied. sngrep needs root or "
                            "CAP_BPF+CAP_PERFMON\n");
        else
            fprintf(stderr, "eBPF: failed to load programs: %s\n",
                    strerror(errno));
        capture_bpf_deinit();
        return 1;
    }

    bpf_links = vector_create(0, 16);
    bpf_libs = capture_bpf_find_libssl("/proc");

    for (i = 0; i < vector_count(bpf_libs); i++) {
        const char *lib = vector_item(bpf_libs, i);
        int n = 0;

        n += capture_bpf_attach_uprobe(bpf_obj->progs.sngrep_ssl_write,
                                       lib, "SSL_write", false);
        n += capture_bpf_attach_uprobe(bpf_obj->progs.sngrep_ssl_read,
                                       lib, "SSL_read", false);
        n += capture_bpf_attach_uprobe(bpf_obj->progs.sngrep_ssl_read_ret,
                                       lib, "SSL_read", true);

        // Absent before OpenSSL 1.1.1, so failure here is not fatal
        capture_bpf_attach_uprobe(bpf_obj->progs.sngrep_ssl_write_ex,
                                  lib, "SSL_write_ex", false);
        capture_bpf_attach_uprobe(bpf_obj->progs.sngrep_ssl_read_ex,
                                  lib, "SSL_read_ex", false);
        capture_bpf_attach_uprobe(bpf_obj->progs.sngrep_ssl_read_ex_ret,
                                  lib, "SSL_read_ex", true);

        if (n)
            attached++;
        else
            fprintf(stderr, "eBPF: could not attach to %s\n", lib);
    }

    if (attached == 0) {
        if (vector_count(bpf_libs) == 0)
            fprintf(stderr, "eBPF: no process using libssl found. Start "
                            "sngrep after your SIP daemon is running.\n");
        capture_bpf_deinit();
        return 1;
    }

    // The kernel side of the correlation, where the connection tuple comes
    // from. Without these the plaintext could not be placed on a connection.
    if (!capture_bpf_attach_kprobe(bpf_obj->progs.sngrep_tcp_sendmsg,
                                   "tcp_sendmsg")
        || !capture_bpf_attach_kprobe(bpf_obj->progs.sngrep_tcp_recvmsg,
                                      "tcp_recvmsg")) {
        capture_bpf_deinit();
        return 1;
    }

    bpf_rb = ring_buffer__new(bpf_map__fd(bpf_obj->maps.events),
                              capture_bpf_handle_event, NULL, NULL);
    if (!bpf_rb) {
        fprintf(stderr, "eBPF: failed to open ring buffer: %s\n",
                strerror(errno));
        capture_bpf_deinit();
        return 1;
    }

    // Register with the capture layer, which owns the polling thread
    if (capture_bpf_source_create() != 0) {
        capture_bpf_deinit();
        return 1;
    }

    return 0;
}

int
capture_bpf_poll(int timeout_ms)
{
    int err = ring_buffer__poll(bpf_rb, timeout_ms);

    // A signal arriving during the poll is not a failure
    if (err == -EINTR)
        return 0;

    return err;
}

/**
 * @brief Report plaintext the kernel side had to drop
 *
 * Silence when nothing was lost, so this only appears when it matters.
 */
static void
capture_bpf_report_stats(void)
{
    static const char *reason[SNGREP_TLS_STAT_NR] = {
        "ring buffer full",
        "could not read process memory",
    };
    int ncpu = libbpf_num_possible_cpus();
    __u64 *vals;
    __u32 idx;

    if (ncpu <= 0 || !(vals = malloc(sizeof(__u64) * ncpu)))
        return;

    for (idx = 0; idx < SNGREP_TLS_STAT_NR; idx++) {
        __u64 total = 0;
        int c;

        if (bpf_map__lookup_elem(bpf_obj->maps.stats, &idx, sizeof(idx),
                                 vals, sizeof(__u64) * ncpu, 0) != 0)
            continue;

        for (c = 0; c < ncpu; c++)
            total += vals[c];

        if (total)
            fprintf(stderr, "eBPF: %llu message(s) lost: %s\n",
                    (unsigned long long) total, reason[idx]);
    }

    free(vals);
}

void
capture_bpf_deinit(void)
{
    int i;

    if (bpf_obj)
        capture_bpf_report_stats();

    capture_bpf_source_destroy();

    if (bpf_rb) {
        ring_buffer__free(bpf_rb);
        bpf_rb = NULL;
    }

    if (bpf_links) {
        // The vector has no destroyer, so the links are released here
        for (i = 0; i < vector_count(bpf_links); i++)
            bpf_link__destroy(vector_item(bpf_links, i));
        vector_destroy(bpf_links);
        bpf_links = NULL;
    }

    if (bpf_obj) {
        sngrep_tls_bpf__destroy(bpf_obj);
        bpf_obj = NULL;
    }

    if (bpf_libs) {
        vector_destroy(bpf_libs);
        bpf_libs = NULL;
    }
}
