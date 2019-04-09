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
 * @file packet_tcp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_tcp.h
 *
 * Support for TCP transport layer dissection
 *
 */

#include "config.h"
#include <glib.h>
#include <glib/gprintf.h>
#include "glib/glib-extra.h"
#include "capture/packet.h"
#include "packet_ip.h"
#include "packet_tcp.h"

static void
packet_tcp_segment_free(PacketTcpSegment *segment)
{
    g_byte_array_free(segment->data, TRUE);
    g_free(segment);
}

static gint
packet_tcp_sort_segments(const PacketTcpData **a, const PacketTcpData **b)
{
    return (*a)->seq - (*b)->seq;
}

static gchar *
packet_tcp_assembly_hashkey(Packet *packet)
{
    PacketIpData *ipdata = packet_ip_data(packet);
    g_return_val_if_fail(ipdata != NULL, NULL);
    PacketTcpData *tcpdata = g_ptr_array_index(packet->proto, PACKET_TCP);
    g_return_val_if_fail(tcpdata != NULL, NULL);

    return g_strdup_printf(
        "%s:%hu-%s:%hu",
        ipdata->srcip, tcpdata->sport,
        ipdata->dstip, tcpdata->dport
    );
}

static GByteArray *
packet_tcp_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    DissectorTcpData *priv = g_ptr_array_index(parser->dissectors_priv, PACKET_TCP);

    // Get Packet IP protocol information
    PacketIpData *ipdata = packet_ip_data(packet);
    g_return_val_if_fail(ipdata != NULL, NULL);

    // Is this a IP/TCP packet?
    if (ipdata->protocol != IPPROTO_TCP)
        return data;

    // Get TCP Header content
    struct tcphdr *tcp = (struct tcphdr *) data->data;

    // TCP packet data
    PacketTcpData *tcp_data = g_malloc0(sizeof(PacketTcpData));
#ifdef __FAVOR_BSD
    tcp_data->off = (tcp->th_off * 4);
    tcp_data->seq = g_ntohl(tcp->th_seq);
    tcp_data->psh = (guint16) (tcp->th_flags & TH_PUSH);
    tcp_data->ack = (guint16) (tcp->th_flags & TH_ACK);
    tcp_data->syn = (guint16) (tcp->th_flags & TH_SYN);
    tcp_data->sport = g_ntohs(tcp->th_sport);
    tcp_data->dport = g_ntohs(tcp->th_dport);
#else
    tcp_data->off = (guint16) (tcp->doff * 4);
    tcp_data->seq = g_ntohl(tcp->seq);
    tcp_data->psh = tcp->psh;
    tcp_data->ack = tcp->ack;
    tcp_data->syn = tcp->syn;
    tcp_data->sport = g_ntohs(tcp->source);
    tcp_data->dport = g_ntohs(tcp->dest);
#endif

    // Set packet protocol data
    g_ptr_array_set(packet->proto, PACKET_TCP, tcp_data);

    // Remove TCP header length
    g_byte_array_remove_range(data, 0, tcp_data->off);

    // Get TCP stream assembly hash key
    gchar *assembly_hashkey = packet_tcp_assembly_hashkey(packet);

    // Find segment stream in assembly hash
    PacketTcpStream *stream = g_hash_table_lookup(priv->assembly, assembly_hashkey);

    // New stream, Check if this packet is interesting
    if (stream == NULL) {
        GByteArray *pending = packet_parser_next_dissector(parser, packet, data);
        // Packet fully parsed, all done!
        if (pending == NULL) {
            g_free(assembly_hashkey);
            return NULL;
        }

        // This packet is SIP but it's not yet complete, add to the assembly list
        if (packet_has_type(packet, PACKET_SIP)) {
            // Create first segment for this stream
            PacketTcpSegment *segment = g_malloc(sizeof(PacketTcpSegment));
            segment->packet = packet_ref(packet);
            segment->data = g_byte_array_ref(data);

            // Create a new stream
            stream = g_malloc0(sizeof(PacketTcpStream));
            stream->segments = g_ptr_array_new_with_free_func((GDestroyNotify) packet_tcp_segment_free);
            g_ptr_array_add(stream->segments, segment);

            // Add stream to assmebly list
            g_hash_table_insert(priv->assembly, assembly_hashkey, stream);
        }

        // Not a SIP packet
        return data;
    }

    // Assembly of existing stream
    PacketTcpSegment *segment = g_malloc(sizeof(PacketTcpSegment));
    segment->packet = packet;
    segment->data = data;

    // Add new segment
    g_ptr_array_add(stream->segments, segment);

    // Check if stream is too segmented
    if (g_ptr_array_len(stream->segments) > TCP_MAX_SEGMENTS) {
        g_hash_table_remove(priv->assembly, assembly_hashkey);
        g_free(assembly_hashkey);
        g_ptr_array_free(stream->segments, TRUE);
        g_free(stream);
        return data;
    }

    // Sort TCP segments based on their sequence number
    g_ptr_array_sort(stream->segments, (GCompareFunc) packet_tcp_sort_segments);

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

    // Call next dissector
    GByteArray *pending = packet_parser_next_dissector(parser, packet, data);
    // Packet fully parsed, all done!
    if (pending == NULL) {
        g_hash_table_remove(priv->assembly, assembly_hashkey);
        g_free(assembly_hashkey);
        g_ptr_array_free(stream->segments, TRUE);
        g_free(stream);
    } else {
        // Partially parsed, keep pending information in the stream
        /* TODO */
    }

    return pending;
}

static void
packet_tcp_init(PacketParser *parser)
{
    DissectorTcpData *tcp_data = g_malloc0(sizeof(DissectorTcpData));
    tcp_data->assembly = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    g_ptr_array_set(parser->dissectors_priv, PACKET_TCP, tcp_data);
}

static void
packet_tcp_deinit(PacketParser *parser)
{
    DissectorTcpData *priv = g_ptr_array_index(parser->dissectors_priv, PACKET_TCP);
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
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SIP));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_TLS));
    return proto;
}
