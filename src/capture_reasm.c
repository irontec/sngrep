/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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
 * @file capture_tcpreasm.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage reassembly TCP frames
 *
 * This file contains the functions and structures to manage the reassembly of
 * captured tcp packets
 *
 */

#include "config.h"
#include "util.h"
#include "capture_reasm.h"

// Capture information
extern capture_config_t capture_cfg;

capture_packet_t *
capture_packet_reasm_tcp(const struct pcap_pkthdr *header, const u_char *packet, struct tcphdr *tcp,
                         const char *ip_src, u_short sport, const char *ip_dst, u_short dport, u_char *payload, int size_payload) {

    vector_iter_t it = vector_iterator(capture_cfg.tcp_reasm);
    capture_packet_t *pkt;
    u_char *new_payload;

    if ((int32_t) size_payload <= 0)
        return NULL;

    while ((pkt = vector_iterator_next(&it))) {
        if (!strcmp(pkt->ip_src, ip_src) && !strcmp(pkt->ip_dst, ip_dst) && pkt->sport == sport && pkt->dport == dport)
            break;
    }

    if (!pkt) {
        // Create a structure for this captured packet
        pkt = capture_packet_create(ip_src, sport, ip_dst, dport);
        capture_packet_set_type(pkt, CAPTURE_PACKET_SIP_TCP);
        vector_append(capture_cfg.tcp_reasm, pkt);
    }

    // Add this frame to the new or existing packet
    capture_packet_add_frame(pkt, header, packet);

    // If the first frame of this packet, set the payload
    if (vector_count(pkt->frames) == 1) {
        capture_packet_set_payload(pkt, payload, size_payload);
    } else {
        // Append this to the existing payload
        new_payload = sng_malloc(pkt->payload_len + size_payload);
        memcpy(new_payload, pkt->payload, pkt->payload_len);
        memcpy(new_payload + pkt->payload_len, payload, size_payload);
        capture_packet_set_payload(pkt, new_payload, pkt->payload_len + size_payload);
        sng_free(new_payload);
    }

    // This packet is ready to be parsed
    if (tcp->th_flags & TH_PUSH) {
        vector_remove(capture_cfg.tcp_reasm, pkt);
        return pkt;
    }

    return NULL;
}
