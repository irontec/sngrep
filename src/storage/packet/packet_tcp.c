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
 */
#include "config.h"
#include <glib.h>
#include "glib-extra/glib.h"
#include "packet.h"
#include "capture/capture.h"
#include "packet_ip.h"
#include "packet_tcp.h"

G_DEFINE_TYPE(PacketDissectorTcp, packet_dissector_tcp, PACKET_TYPE_DISSECTOR)

PacketTcpData *
packet_tcp_data(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);
    return packet_get_protocol_data(packet, PACKET_PROTO_TCP);
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
packet_tcp_assembly_gc(PacketDissectorTcp *dissector)
{
    g_return_val_if_fail(PACKET_DISSECTOR_IS_TCP(dissector), FALSE);
    // Remove big un-assembled streams
    g_hash_table_foreach_remove(dissector->assembly, packet_tcp_assembly_remove, NULL);
    return TRUE;
}

static gchar *
packet_tcp_assembly_hashkey(PacketTcpSegment *segment)
{
    Packet *packet = segment->packet;
    g_return_val_if_fail(packet != NULL, NULL);
    PacketIpData *ipdata = packet_ip_data(packet);
    g_return_val_if_fail(ipdata != NULL, NULL);
    PacketTcpData *tcpdata = packet_get_protocol_data(packet, PACKET_PROTO_TCP);
    g_return_val_if_fail(tcpdata != NULL, NULL);

    return g_strdup_printf(
        "%s:%hu-%s:%hu",
        ipdata->srcip, tcpdata->sport,
        ipdata->dstip, tcpdata->dport
    );
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

static PacketTcpStream *
packet_dissector_tcp_find_stream(PacketDissectorTcp *priv, PacketTcpSegment *segment)
{
    // Get TCP stream assembly hash key
    g_autofree gchar *hashkey = packet_tcp_assembly_hashkey(segment);

    // Find segment stream in assembly hash
    return g_hash_table_lookup(priv->assembly, hashkey);
}

static GByteArray *
packet_dissector_tcp_dissect(PacketDissector *self, Packet *packet, GByteArray *data)
{
    // Get TCP dissector information
    g_return_val_if_fail(PACKET_DISSECTOR_IS_TCP(self), NULL);
    PacketDissectorTcp *dissector = PACKET_DISSECTOR_TCP(self);

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
    tcp_data->proto.id = PACKET_PROTO_TCP;
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
    packet_set_protocol_data(packet, PACKET_PROTO_TCP, tcp_data);

    // Remove TCP header length
    g_byte_array_remove_range(data, 0, tcp_data->off);

    // Create new segment for this stream
    PacketTcpSegment *segment = packet_tcp_segment_new(packet, data);

    // Look for another packet with same ip/port data in reassembly list
    PacketTcpStream *stream = packet_dissector_tcp_find_stream(dissector, segment);

    // Create a new stream if none matches
    if (stream == NULL) {
        stream = packet_tcp_stream_new(segment);
        // Add stream to assembly list
        g_hash_table_insert(dissector->assembly, stream->hashkey, stream);
    }

    // Add segment to stream
    packet_tcp_stream_add_segment(stream, segment);

    // Check max number of stream segments (let the garbage collector clean it)
    if (g_ptr_array_len(stream->segments) > TCP_MAX_SEGMENTS) {
        return data;
    }

    // Sort and take packet frames in the last packet
    packet->frames = packet_tcp_stream_take_frames(stream);

    // Check if this packet is interesting
    guint stream_len = stream->data->len;
    packet_dissector_next(self, packet, stream->data);

    // Not interesting stream
    if (!packet_has_protocol(packet, PACKET_PROTO_SIP)) {
        g_hash_table_remove(dissector->assembly, stream->hashkey);
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

static void
packet_dissector_tcp_finalize(GObject *self)
{
    // Get TCP dissector information
    g_return_if_fail(PACKET_DISSECTOR_IS_TCP(self));
    PacketDissectorTcp *dissector = PACKET_DISSECTOR_TCP(self);
    g_hash_table_destroy(dissector->assembly);
}

static void
packet_dissector_tcp_free_data(Packet *packet)
{
    PacketTcpData *tcp_data = packet_tcp_data(packet);
    g_return_if_fail(tcp_data != NULL);
    g_free(tcp_data);
}

static void
packet_dissector_tcp_class_init(PacketDissectorTcpClass *klass)
{
    PacketDissectorClass *dissector_class = PACKET_DISSECTOR_CLASS(klass);
    dissector_class->dissect = packet_dissector_tcp_dissect;
    dissector_class->free_data = packet_dissector_tcp_free_data;

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = packet_dissector_tcp_finalize;
}

static void
packet_dissector_tcp_init(PacketDissectorTcp *self)
{
    // TCP Dissector base information
    packet_dissector_set_protocol(PACKET_DISSECTOR(self), PACKET_PROTO_TCP);
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_SIP);
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_TLS);

    // TCP assembly garbage collector
    GSource *tcp_garbage_collector = g_timeout_source_new(10000);
    g_source_set_callback(tcp_garbage_collector, (GSourceFunc) packet_tcp_assembly_gc, self, NULL);
    g_source_attach(tcp_garbage_collector, NULL);

    // TCP fragment assembly hash table
    self->assembly = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        NULL,
        (GDestroyNotify) packet_tcp_stream_free
    );
}

PacketDissector *
packet_dissector_tcp_new()
{
    return g_object_new(PACKET_DISSECTOR_TYPE_TCP, NULL);
}
