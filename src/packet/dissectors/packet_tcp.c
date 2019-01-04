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

static void
packet_tcp_segment_free(PacketTcpSegment *segment)
{
    g_byte_array_free(segment->data, TRUE);
    g_free(segment);
}

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

    // Set segment payload (without TCP header)
    segment->data = g_byte_array_sized_new(data->len);
    g_byte_array_append(segment->data, data->data, data->len);

    // Find segment stream in assembly hash
    stream = g_hash_table_lookup(priv->assembly, packet_tcp_segment_hashkey(segment));

    // New stream, store in the assembly list
    if (stream == NULL) {
        stream = g_malloc0(sizeof(PacketTcpStream));
        stream->src = segment->src;
        stream->dst = segment->dst;
        stream->segments = g_ptr_array_new_with_free_func((GDestroyNotify) packet_tcp_segment_free);
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
    for (guint i = 0; i < g_ptr_array_len(stream->segments); i++) {
        segment = g_ptr_array_index(stream->segments, i);
        // Join all sorted segments data toguether
        g_byte_array_append(data, segment->data->data, segment->data->len);
        // Store all stream frames
        frames = g_list_concat_deep(frames, segment->packet->frames);
    }

    // Set all frames on current packet
    packet->frames = frames;

    // Set packet protocol data
    PacketTcpData *tcp_data = g_malloc0(sizeof(PacketTcpData));
    tcp_data->sport = segment->src.port;
    tcp_data->dport = segment->dst.port;
    tcp_data->ack = segment->ack;
    tcp_data->syn = segment->syn;
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
        return NULL;
    } else {
        // @TODO multiples messages in multiples TCP packets
        return NULL;
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

static void
packet_tcp_deinit(PacketParser *parser)
{
    DissectorTcpData *priv = g_ptr_array_index(parser->dissectors, PACKET_TCP);
    g_return_if_fail(priv != NULL);

    g_hash_table_destroy(priv->assembly);
    g_free(priv);
}

PacketDissector *
packet_tcp_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_TCP;
    proto->init = packet_tcp_init;
    proto->deinit = packet_tcp_deinit;
    proto->dissect = packet_tcp_parse;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_TLS));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SIP));
    return proto;
}
