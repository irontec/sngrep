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
#include "capture.h"
#include "capture_reasm.h"
#ifdef WITH_IPV6
#include <netinet/ip6.h>
#endif

// Capture information
extern capture_config_t capture_cfg;


capture_packet_t *
capture_packet_reasm_ip(capture_info_t *capinfo, const struct pcap_pkthdr *header, u_char **packet, uint32_t *size, uint32_t *caplen)
{
    // IP header data
    struct ip *ip4;
#ifdef WITH_IPV6
    // IPv6 header data
    struct ip6_hdr *ip6;
#endif
    // IP version
    uint32_t ip_ver;
    // IP protocol
    uint8_t ip_proto;
    // IP header size
    uint32_t ip_hl = 0;
    // Fragment offset
    uint16_t ip_off = 0;
    // Fragmentation flag
    uint16_t ip_frag = 0;
    // Fragmentation identifier
    uint32_t ip_id = 0;
    // Fragmentation offset
    uint16_t ip_frag_off = 0;
    //! Source Address
    char ip_src[ADDRESSLEN];
    //! Destination Address
    char ip_dst[ADDRESSLEN];
    //!
    vector_iter_t it;
    //!
    capture_packet_t *pkt;
    //!
    capture_frame_t *frame;
    u_char *new_data;
    uint32_t len_data = 0;

    // Get IP header
    ip4 = (struct ip *) (*packet + capinfo->link_hl);

#ifdef WITH_IPV6
    // Get IPv6 header
    ip6 = (struct ip6_hdr *) (packet + capinfo->link_hl);
#endif

    // Get IP version
    ip_ver = ip4->ip_v;

    switch (ip_ver) {
        case 4:
            ip_hl = ip4->ip_hl * 4;
            ip_proto = ip4->ip_p;
            ip_off = ntohs(ip4->ip_off);

            ip_frag = ip_off & (IP_MF | IP_OFFMASK);
            ip_frag_off = (ip_frag) ? (ip_off & IP_OFFMASK) * 8 : 0;
            ip_id = ntohs(ip4->ip_id);

            inet_ntop(AF_INET, &ip4->ip_src, ip_src, sizeof(ip_src));
            inet_ntop(AF_INET, &ip4->ip_dst, ip_dst, sizeof(ip_dst));
            break;
#ifdef WITH_IPV6
        case 6:
            ip_hl = sizeof(struct ip6_hdr);
            ip_proto = ip6->ip6_nxt;

            if (ip_proto == IPPROTO_FRAGMENT) {
                struct ip6_frag *ip6f = (struct ip6_frag *) (ip6 + ip_hl);
                ip_frag_off = ntohs(ip6f->ip6f_offlg & IP6F_OFF_MASK);
                ip_id = ntohl(ip6f->ip6f_ident);
            }

            inet_ntop(AF_INET6, &ip6->ip6_src, ip_src, sizeof(ip_src));
            inet_ntop(AF_INET6, &ip6->ip6_dst, ip_dst, sizeof(ip_dst));
            break;
#endif
        default:
            return NULL;
    }

    // Remove IP Header length from payload
    *size = *caplen - capinfo->link_hl - ip_hl;

#ifdef WITH_IPV6
    if (ip_ver == 6)
        *size -= ntohs(ip6->ip6_ctlun.ip6_un1.ip6_un1_plen);
#endif

    // If no fragmentation
    if (ip_frag == 0) {
        // Just create a new packet with given network data
        pkt = capture_packet_create(ip_proto, ip_src, ip_dst, ip_id);
        capture_packet_add_frame(pkt, header, *packet);
        return pkt;
    }

    // Look for another packet with same id in IP reassembly vector
    it = vector_iterator(capture_cfg.ip_reasm);
    while ((pkt = vector_iterator_next(&it))) {
        if (!strcmp(pkt->ip_src, ip_src) && !strcmp(pkt->ip_dst, ip_dst) && pkt->ip_id == ip_id)
            break;
    }

    // If we already have this packet stored, append this frames to existing one
    if (pkt) {
        capture_packet_add_frame(pkt, header, *packet);
    } else {
        // Add To the possible reassembly list
        pkt = capture_packet_create(ip_proto, ip_src, ip_dst, ip_id);
        capture_packet_add_frame(pkt, header, *packet);
        vector_append(capture_cfg.ip_reasm, pkt);
        return NULL;
    }

    // If no more fragments
    if ((ip_off & IP_MF) == 0) {
        // TODO Dont check the flag, check the holes
        // Calculate assembled IP payload data
        it = vector_iterator(pkt->frames);
        while ((frame = vector_iterator_next(&it))) {
            struct ip *frame_ip = (struct ip *) (frame->data + capinfo->link_hl);
            len_data += frame->header->caplen - capinfo->link_hl - frame_ip->ip_hl * 4;
        }

        // Create memory for the assembly packet
        new_data = sng_malloc(capinfo->link_hl + ip_hl + len_data);
        memset(new_data, 0, capinfo->link_hl + ip_hl + len_data);
        memcpy(new_data, *packet, capinfo->link_hl + ip_hl);

        it = vector_iterator(pkt->frames);
        while ((frame = vector_iterator_next(&it))) {
            // Get IP header
            struct ip *frame_ip = (struct ip *) (frame->data + capinfo->link_hl);
            memcpy(new_data + capinfo->link_hl + ip_hl + (ntohs(frame_ip->ip_off) & IP_OFFMASK) * 8,
                   frame->data + capinfo->link_hl + frame_ip->ip_hl * 4,
                   frame->header->caplen - capinfo->link_hl - frame_ip->ip_hl * 4);
        }

        *caplen = capinfo->link_hl + ip_hl + len_data;
        *size = len_data;
        *packet = new_data;

        // Return the assembled IP packet
        return pkt;
    }

    return NULL;
}

capture_packet_t *
capture_packet_reasm_tcp(capture_packet_t *packet, struct tcphdr *tcp, u_char *payload, int size_payload) {

    vector_iter_t it = vector_iterator(capture_cfg.tcp_reasm);
    capture_packet_t *pkt;
    u_char *new_payload;

    if ((int32_t) size_payload <= 0)
        return NULL;

    while ((pkt = vector_iterator_next(&it))) {
        if (!strcmp(pkt->ip_src, packet->ip_src) && !strcmp(pkt->ip_dst, packet->ip_dst)
                && pkt->sport == packet->sport && pkt->dport == packet->dport)
            break;
    }

    // If we already have this packet stored, append this frames to existing one
    if (pkt) {
        capture_frame_t *frame;
        vector_iter_t frames = vector_iterator(packet->frames);
        while ((frame = vector_iterator_next(&frames)))
            capture_packet_add_frame(pkt, frame->header, frame->data);
        // TODO We should destroy packet.
    } else {
        // First time this packet has been seen
        pkt = packet;
        // Add To the possible reassembly list
        vector_append(capture_cfg.tcp_reasm, packet);
    }

    // If the first frame of this packet
    if (vector_count(pkt->frames) == 1) {
        // Set initial payload
        capture_packet_set_payload(pkt, payload, size_payload);
    } else {
        // Append payload to the existing
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
