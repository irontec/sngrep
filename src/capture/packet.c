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
 * @file packet.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet.h
 *
 */
#include "config.h"
#include <glib.h>
#include "glib/glib-extra.h"
#include "capture/dissectors/packet_ip.h"
#include "capture/dissectors/packet_tcp.h"
#include "capture/dissectors/packet_udp.h"
#include "capture/dissectors/packet_sip.h"
#include "capture/capture_pcap.h"
#include "packet.h"

/**
 * @brief Packet class definition
 */
G_DEFINE_TYPE(Packet, packet, G_TYPE_OBJECT)

G_GNUC_UNUSED static void
packet_init(Packet *self)
{
    self->proto = g_ptr_array_sized_new(PACKET_PROTO_COUNT);
    g_ptr_array_set_size(self->proto, PACKET_PROTO_COUNT);
}

static void
packet_dispose(GObject *gobject)
{
    // Free packet data
    packet_free(SNGREP_PACKET(gobject));
    // Chain Gobject dispose
    G_OBJECT_CLASS (packet_parent_class)->dispose(gobject);
}

Packet *
packet_new(PacketParser *parser)
{
    // Create a new packet
    Packet *packet = g_object_new(CAPTURE_TYPE_PACKET, NULL);
    packet->parser = parser;
    return packet;
}

static void
packet_proto_free(gpointer proto_id, Packet *packet)
{
    if (packet_has_type(packet, GPOINTER_TO_INT(proto_id))) {
        packet_parser_dissector_free(packet->parser, packet, GPOINTER_TO_INT(proto_id));
    }
}

void
packet_free(Packet *packet)
{
    g_return_if_fail(packet != NULL);

    // Free each protocol data
    g_ptr_array_foreach_idx(packet->proto, (GFunc) packet_proto_free, packet);
    g_ptr_array_free(packet->proto, TRUE);

    // Free each frame data
    g_list_free_full(packet->frames, (GDestroyNotify) packet_frame_free);

    // Free packet data
    address_free(packet->src);
    address_free(packet->dst);
}

Packet *
packet_ref(Packet *packet)
{
    return g_object_ref(packet);
}

void
packet_unref(Packet *packet)
{
    g_object_unref(packet);
}

/**
 * @todo Fix address retro-compatibilities
 */
Address *
packet_src_address(Packet *packet)
{
    if (packet->src == NULL) {
        // Get IP address from IP parsed protocol
        PacketIpData *ip = packet_ip_data(packet);
        g_return_val_if_fail(ip, NULL);

        // Get Port from UDP or TCP parsed protocol
        if (packet_has_type(packet, PACKET_UDP)) {
            PacketUdpData *udp = g_ptr_array_index(packet->proto, PACKET_UDP);
            g_return_val_if_fail(udp, NULL);
            packet->src = address_new(ip->srcip, udp->sport);
        } else {
            PacketTcpData *tcp = g_ptr_array_index(packet->proto, PACKET_TCP);
            g_return_val_if_fail(tcp, NULL);
            packet->src = address_new(ip->srcip, tcp->sport);
        }
    }

    return packet->src;
}

/**
 * @todo Fix address retro-compatibilities
 */
Address *
packet_dst_address(Packet *packet)
{
    if (packet->dst == NULL) {
        // Get IP address from IP parsed protocol
        PacketIpData *ip = packet_ip_data(packet);
        g_return_val_if_fail(ip, NULL);

        // Get Port from UDP or TCP parsed protocol
        if (packet_has_type(packet, PACKET_UDP)) {
            PacketUdpData *udp = g_ptr_array_index(packet->proto, PACKET_UDP);
            g_return_val_if_fail(udp, NULL);
            packet->dst = address_new(ip->dstip, udp->dport);
        } else {
            PacketUdpData *tcp = g_ptr_array_index(packet->proto, PACKET_TCP);
            g_return_val_if_fail(tcp, NULL);
            packet->dst = address_new(ip->dstip, tcp->dport);
        }
    }

    return packet->dst;
}

const char *
packet_transport(Packet *packet)
{
    if (packet_has_type(packet, PACKET_UDP))
        return "UDP";

    if (packet_has_type(packet, PACKET_TCP)) {
        if (packet_has_type(packet, PACKET_WS)) {
            return (packet_has_type(packet, PACKET_TLS)) ? "WSS" : "WS";
        }
        return packet_has_type(packet, PACKET_TLS) ? "TLS" : "TCP";
    }
    return "???";
}

GDateTime *
packet_time(const Packet *packet)
{
    PacketFrame *last;
    GDateTime *ts = NULL;

    // Return last frame timestamp
    if (packet && (last = g_list_last_data(packet->frames))) {
        ts = last->ts;
    }

    // Return packe timestamp
    return ts;
}

void
packet_frame_free(PacketFrame *frame)
{
    if (NULL != frame->ts) {
        g_date_time_unref(frame->ts);
    }
    g_byte_array_free(frame->data, TRUE);
    g_free(frame);
}

PacketFrame *
packet_frame_new()
{
    PacketFrame *frame = g_malloc0(sizeof(PacketFrame));
    return frame;
}

G_GNUC_UNUSED static void
packet_class_init(G_GNUC_UNUSED PacketClass *klass)
{
    // Get the parent gobject class
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    // Set finalize functions
    object_class->dispose = packet_dispose;
}
