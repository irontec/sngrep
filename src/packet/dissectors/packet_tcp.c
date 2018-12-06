/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
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
 * @file capture_tcp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in capture_tcp.h
 *
 * Support for TCP transport layer
 *
 */

#include <glib-extra.h>
#include <glib/gprintf.h>
#include "config.h"
#include "packet/packet.h"
#include "packet/dissectors/packet_ip.h"
#include "packet_tcp.h"

#if 0
vector_t *tcp_reasm;

Packet *
capture_packet_reasm_tcp(Packet *packet, struct tcphdr *tcp, u_char *payload, int size_payload) {

    vector_iter_t it = vector_iterator(tcp_reasm);
    Packet *pkt;

    //! Assembled
    if ((int32_t) size_payload <= 0)
        return packet;

    while ((pkt = vector_iterator_next(&it))) {
        if (addressport_equals(pkt->src, packet->src) &&
                addressport_equals(pkt->dst, packet->dst)) {
            break;
        }
    }
/*
    // If we already have this packet stored
    if (pkt) {
        frame_t *frame;
        // Append this frames to the original packet
        vector_iter_t frames = vector_iterator(packet->frames);
        while ((frame = vector_iterator_next(&frames)))
            packet_add_frame(pkt, frame->header, frame->data);
        // Destroy current packet as its frames belong to the stored packet
        packet_destroy(packet);
    } else {
        // First time this packet has been seen
        pkt = packet;
        // Add To the possible reassembly list
        vector_append(tcp_reasm, packet);
    }

    // Store firt tcp sequence
    if (pkt->tcp_seq == 0) {
        pkt->tcp_seq = ntohl(tcp->th_seq);
    }

    // If the first frame of this packet
    if (vector_count(pkt->frames) == 1) {
        // Set initial payload
        packet_set_payload(pkt, payload, size_payload);
    } else {
        // Check payload length. Dont handle too big payload packets
        if (pkt->payload_len + size_payload > MAX_HEP_BUFSIZE) {
            packet_destroy(pkt);
            vector_remove(capture_cfg.tcp_reasm, pkt);
            return NULL;
        }
        new_payload = sng_malloc(pkt->payload_len + size_payload);
        if (pkt->tcp_seq < ntohl(tcp->th_seq)) {
            // Append payload to the existing
            pkt->tcp_seq =  ntohl(tcp->th_seq);
            memcpy(new_payload, pkt->payload, pkt->payload_len);
            memcpy(new_payload + pkt->payload_len, payload, size_payload);
        } else {
            // Prepend payload to the existing
            memcpy(new_payload, payload, size_payload);
            memcpy(new_payload + size_payload, pkt->payload, pkt->payload_len);
        }
        packet_set_payload(pkt, new_payload, pkt->payload_len + size_payload);
        sng_free(new_payload);
    }

    // Store full payload content
    memset(full_payload, 0, MAX_HEP_BUFSIZE);
    memcpy(full_payload, pkt->payload, pkt->payload_len);

    // This packet is ready to be parsed
    int valid = sip_validate_packet(pkt);
    if (valid == VALIDATE_COMPLETE_SIP) {
        // Full SIP packet!
        vector_remove(capture_cfg.tcp_reasm, pkt);
        return pkt;
    } else if (valid == VALIDATE_MULTIPLE_SIP) {
        vector_remove(capture_cfg.tcp_reasm, pkt);

        // We have a full SIP Packet, but do not remove everything from the reasm queue
        packet_t *cont = packet_clone(pkt);
        int pldiff = size_payload - pkt->payload_len;
        packet_set_payload(cont, full_payload + pkt->payload_len, pldiff);
        vector_append(capture_cfg.tcp_reasm, cont);

        // Return the full initial packet
        return pkt;
    } else if (valid == VALIDATE_NOT_SIP) {
        // Not a SIP packet, store until PSH flag
        if (tcp->th_flags & TH_PUSH) {
            vector_remove(capture_cfg.tcp_reasm, pkt);
            return pkt;
        }
    }
*/
    // An incomplete SIP Packet
    return NULL;
}
#endif

static gint
packet_tcp_sort_segments(const PacketTcpSegment **a, const PacketTcpSegment **b)
{
    return (*a)->seq - (*b)->seq;
}

static gchar *
packet_tcp_segment_hashkey(PacketTcpSegment *segment)
{
    static gchar hashkey[ADDRESSLEN + ADDRESSLEN + 12];
    g_sprintf(hashkey, "%s:%hu-%s:%hu",
            segment->src.ip, segment->src.port,
            segment->dst.ip, segment->dst.port
    );
    return hashkey;
}

static GByteArray *
packet_tcp_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    DissectorTcpData *priv = g_ptr_array_index(parser->dissectors, PACKET_TCP);
    PacketTcpSegment *segment = NULL;
    PacketTcpStream *stream = NULL;

    // Get Packet IP protocol information
    PacketIpData *ipdata = g_ptr_array_index(packet->proto, PACKET_IP);
    g_return_val_if_fail(ipdata != NULL, NULL);

    // Is this a IP/TCP packet?
    if (ipdata->protocol != IPPROTO_TCP)
        return data;

    // Get TCP Header content
    struct tcphdr *tcp = (struct tcphdr *) data->data;

    // TCP packet data
    segment = g_malloc0(sizeof(PacketTcpSegment));
    segment->packet = packet;
    segment->src = ipdata->saddr;
    segment->dst = ipdata->daddr;
    segment->data = g_byte_array_sized_new(data->len);
    g_byte_array_append(segment->data, data->data, data->len);

#ifdef __FAVOR_BSD
    segment->off = (tcp->th_off * 4);
    segment->seq = g_ntohl(tcp->th_seq);
    segment->psh = (guint16) (tcp->th_flags & TH_PUSH);
    segment->ack = (guint16) (tcp->th_flags & TH_ACK);
    segment->syn = (guint16) (tcp->th_flags & TH_SYN);
    segment->src.port = g_htons(tcp->th_sport);
    segment->dst.port = g_htons(tcp->th_dport);
#else
    segment->off = (guint16) (tcp->doff * 4);
    segment->seq = g_ntohl(tcp->seq);
    segment->psh = tcp->psh;
    segment->ack = tcp->ack;
    segment->syn = tcp->syn;
    segment->src.port = g_htons(tcp->source);
    segment->dst.port = g_htons(tcp->dest);
#endif

    // Remove TCP header length
    g_byte_array_remove_range(data, 0, segment->off);

    // Find segment stream in assembly hash
    stream = g_hash_table_lookup(priv->assembly, packet_tcp_segment_hashkey(segment));

    // New stream, store in the assembly list
    if (stream == NULL) {
        stream = g_malloc0(sizeof(PacketTcpStream));
        stream->src = segment->src;
        stream->dst = segment->dst;
        stream->segments = g_ptr_array_new_with_free_func(g_free);
        g_hash_table_insert(priv->assembly, g_strdup(packet_tcp_segment_hashkey(segment)), stream);
    }

    // Add new segment
    g_ptr_array_add(stream->segments, segment);
    g_ptr_array_sort(stream->segments, (GCompareFunc) packet_tcp_sort_segments);

    // Check if stream is too segmented
    if (g_ptr_array_len(stream->segments) > TCP_MAX_SEGMENTS) {
        g_hash_table_remove(priv->assembly, packet_tcp_segment_hashkey(segment));
        g_ptr_array_free(stream->segments, TRUE);
        g_free(stream);
        return data;
    }

    // Assemble all sorted contents and frames on current packet
    GList *frames = NULL;
    data = g_byte_array_new();
    for (guint i = 0; i < stream->segments->len; i++) {
        segment = g_ptr_array_index(stream->segments, i);
        // Join all sorted segments data toguether
        g_byte_array_append(data, segment->data->data, segment->data->len);
        // Store all stream frames
        frames = g_list_append(frames, segment->packet->frames);
    }

    // Set all frames on current packet
    packet->frames = frames;

    // Set packet protocol data
    PacketTcpData *tcp_data = g_malloc0(sizeof(PacketTcpData));
    tcp_data->sport = segment->src.port;
    tcp_data->dport = segment->dst.port;
    tcp_data->ack   = segment->ack;
    tcp_data->syn   = segment->syn;
    g_ptr_array_set(packet->proto, PACKET_TCP, tcp_data);

    // Call next dissector
    GByteArray *pending = packet_parser_next_dissector(parser, packet, data);

    // Check if the next dissectors left something
    if (pending == NULL) {
        // Stream fully parsed!
        g_hash_table_remove(priv->assembly, packet_tcp_segment_hashkey(segment));
        g_free(segment);
        g_free(stream);
    } else if (pending->len < data->len) {
        // Partially parsed
    }

    return pending;
}

static void
packet_tcp_init(PacketParser *parser)
{
    DissectorTcpData *tcp_data = g_malloc0(sizeof(DissectorTcpData));
    tcp_data->assembly = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    g_ptr_array_set(parser->dissectors, PACKET_TCP, tcp_data);
}

PacketDissector *
packet_tcp_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_TCP;
    proto->init = packet_tcp_init;
    proto->dissect = packet_tcp_parse;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_TLS));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SIP));
    return proto;
}
