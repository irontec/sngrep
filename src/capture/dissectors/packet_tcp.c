/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
#include "capture/capture.h"
#include "packet_ip.h"
#include "packet_tcp.h"

PacketTcpData *
packet_tcp_data(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);
    return g_ptr_array_index(packet->proto, PACKET_TCP);
}

static PacketTcpSegment *
packet_tcp_segment_new(Packet *packet, GByteArray *data)
{
    // Reserve memory for storing segment information
    PacketTcpSegment *segment = g_malloc(sizeof(PacketTcpSegment));
    // Store packet information
    segment->packet = packet_ref(packet);
    // Set segment payload for future reassembly
    segment->data = g_byte_array_ref(data);
    return segment;
}

static void
packet_tcp_segment_free(PacketTcpSegment *segment)
{
    g_byte_array_unref(segment->data);
    packet_unref(segment->packet);
    g_free(segment);
}

static gchar *
packet_tcp_assembly_hashkey(PacketTcpSegment *segment)
{
    Packet *packet = segment->packet;
    g_return_val_if_fail(packet != NULL, NULL);
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

static PacketTcpStream *
packet_tcp_find_stream(DissectorTcpData *priv, PacketTcpSegment *segment)
{
    // Get TCP stream assembly hash key
    g_autofree gchar *hashkey = packet_tcp_assembly_hashkey(segment);

    // Find segment stream in assembly hash
    return g_hash_table_lookup(priv->assembly, hashkey);
}

static PacketTcpStream *
packet_tcp_stream_new(PacketTcpSegment *segment)
{
    // Create a new stream
    PacketTcpStream *stream = g_malloc0(sizeof(PacketTcpStream));
    stream->data = g_byte_array_new();
    stream->segments = g_ptr_array_new_with_free_func((GDestroyNotify) packet_tcp_segment_free);
    stream->hashkey = packet_tcp_assembly_hashkey(segment);
    return stream;
}

static void
packet_tcp_stream_free(PacketTcpStream *stream)
{
    g_free(stream->hashkey);
    g_byte_array_free(stream->data, TRUE);
    g_ptr_array_free(stream->segments, TRUE);
    g_free(stream);
}

static GList *
packet_tcp_stream_take_frames(PacketTcpStream *stream)
{
    GList *frames = NULL;
    for (guint i = 0; i < g_ptr_array_len(stream->segments); i++) {
        PacketTcpSegment *segment = g_ptr_array_index(stream->segments, i);
        g_return_val_if_fail(segment != NULL, NULL);
        Packet *packet = segment->packet;
        g_return_val_if_fail(packet != NULL, NULL);

        // Append frames to datagram list
        frames = g_list_concat_deep(frames, packet->frames);

        // Remove fragment frames (but not free them!)
        g_list_free(packet->frames);
        packet->frames = NULL;
    }

    return frames;
}

static void
packet_tcp_stream_add_segment(PacketTcpStream *stream, PacketTcpSegment *segment)
{
    // Add segment to stream
    g_ptr_array_add(stream->segments, segment);
    // Add payload to stream
    g_byte_array_append(stream->data, segment->data->data, segment->data->len);
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
    packet_add_type(packet, PACKET_TCP, tcp_data);

    // Remove TCP header length
    g_byte_array_remove_range(data, 0, tcp_data->off);

    // Create new segment for this stream
    PacketTcpSegment *segment = packet_tcp_segment_new(packet, data);

    // Look for another packet with same ip/port data in reassembly list
    PacketTcpStream *stream = packet_tcp_find_stream(priv, segment);

    // Create a new stream if none matches
    if (stream == NULL) {
        stream = packet_tcp_stream_new(segment);
        // Add stream to assmebly list
        g_hash_table_insert(priv->assembly, stream->hashkey, stream);
    }

    // Add segment to stream
    packet_tcp_stream_add_segment(stream, segment);

    // Check max number of stream segments (let the garbate collector clean it)
    if (g_ptr_array_len(stream->segments) > TCP_MAX_SEGMENTS) {
        return data;
    }

    // Sort and take packet frames in the last packet
    packet->frames = packet_tcp_stream_take_frames(stream);

    // Check if this packet is interesting
    guint stream_len = stream->data->len;
    packet_parser_next_dissector(parser, packet, stream->data);

    // Not interesting stream
    if (!packet_has_type(packet, PACKET_SIP)) {
        g_hash_table_remove(priv->assembly, stream->hashkey);
        return data;
    }

    // Stream has been parsed, but still has interesting data
    if (stream->data->len < stream_len) {
        // Remove current segments, keep pending data
        g_ptr_array_remove_all(stream->segments);
    }

    // Incomplete SIP TCP packet, keep storing data
    return data;
}

/**
 * @brief TCP Stream garbage collector remove callback
 *
 * Determine if a stream should be removed by garbage collector

 * @return TRUE if stream must be removed
 */
static gboolean
packet_tcp_assembly_remove(G_GNUC_UNUSED gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data)
{
    PacketTcpStream *stream = value;

    if (g_ptr_array_len(stream->segments) > TCP_MAX_SEGMENTS)
        return TRUE;

    if (stream->age++ > TCP_MAX_AGE)
        return TRUE;

    return FALSE;
}

/**
 * @brief TCP Stream garbage collector
 *
 * This callback is invoked periodically to remove existing streams in
 * assembly hash table that are greater than a number of frames.
 *
 * @param parser Parser information owner of dissector data
 * @return TRUE always
 */
static gboolean
packet_tcp_assembly_gc(PacketParser *parser)
{
    g_return_val_if_fail(parser != NULL, FALSE);
    DissectorTcpData *priv = g_ptr_array_index(parser->dissectors_priv, PACKET_TCP);
    g_return_val_if_fail(priv != NULL, FALSE);

    // Remove big unassembled streams
    g_hash_table_foreach_remove(priv->assembly, packet_tcp_assembly_remove, NULL);
    return TRUE;
}

static void
packet_tcp_init(PacketParser *parser)
{
    CaptureManager *manager = capture_manager_get_instance();
    g_return_if_fail(manager != NULL);

    GSource *tcp_gc = g_timeout_source_new(10000);
    g_source_set_callback(tcp_gc, (GSourceFunc) packet_tcp_assembly_gc, parser, NULL);
    g_source_attach(tcp_gc, g_main_loop_get_context(manager->loop));

    DissectorTcpData *tcp_data = g_malloc0(sizeof(DissectorTcpData));
    tcp_data->assembly = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        NULL,
        (GDestroyNotify) packet_tcp_stream_free
    );

    g_ptr_array_set(parser->dissectors_priv, PACKET_TCP, tcp_data);
}

static void
packet_tcp_free(G_GNUC_UNUSED PacketParser *parser, Packet *packet)
{
    PacketTcpData *tcpdata = packet_tcp_data(packet);
    g_return_if_fail(tcpdata != NULL);
    g_free(tcpdata);
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
    proto->free = packet_tcp_free;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SIP));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_TLS));
    return proto;
}
