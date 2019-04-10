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
packet_proto_free(gpointer proto_data, Packet *packet)
{
    gint proto_id = g_ptr_array_data_index(packet->proto, proto_data);
    g_return_if_fail(proto_id >= 0);
    packet_parser_dissector_free(packet->parser, packet, proto_id);
}

void
packet_free(Packet *packet)
{
    g_return_if_fail(packet != NULL);

    // Free each protocol data
    g_ptr_array_foreach(packet->proto, (GFunc) packet_proto_free, packet);
    g_ptr_array_free(packet->proto, TRUE);

    // Free each frame data
    g_list_free_full(packet->frames, (GDestroyNotify) packet_frame_free);
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

gboolean
packet_has_type(const Packet *packet, enum PacketProtoId type)
{
    return g_ptr_array_index(packet->proto, type) != NULL;
}

Address
packet_src_address(const Packet *packet)
{
    Address address = ADDRESS_ZERO;

    // Get IP address from IP parsed protocol
    PacketIpData *ip = packet_ip_data(packet);
    g_return_val_if_fail(ip, address);
    g_utf8_strncpy(address.ip, ip->srcip, ADDRESSLEN);

    // Get Port from UDP or TCP parsed protocol
    if (packet_has_type(packet, PACKET_UDP)) {
        PacketUdpData *udp = g_ptr_array_index(packet->proto, PACKET_UDP);
        g_return_val_if_fail(udp, address);
        address.port = udp->sport;
    } else {
        PacketTcpData *tcp = g_ptr_array_index(packet->proto, PACKET_TCP);
        g_return_val_if_fail(tcp, address);
        address.port = tcp->sport;
    }

    return address;
}

Address
packet_dst_address(const Packet *packet)
{
    Address address = ADDRESS_ZERO;

    // Get IP address from IP parsed protocol
    PacketIpData *ip = packet_ip_data(packet);
    g_return_val_if_fail(ip, address);
    g_utf8_strncpy(address.ip, ip->dstip, ADDRESSLEN);

    // Get Port from UDP or TCP parsed protocol
    if (packet_has_type(packet, PACKET_UDP)) {
        PacketUdpData *udp = g_ptr_array_index(packet->proto, PACKET_UDP);
        g_return_val_if_fail(udp, address);
        address.port = udp->dport;
    } else {
        PacketUdpData *tcp = g_ptr_array_index(packet->proto, PACKET_TCP);
        g_return_val_if_fail(tcp, address);
        address.port = tcp->dport;
    }

    return address;
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

GTimeVal
packet_time(const Packet *packet)
{
    PacketFrame *last;
    GTimeVal ts = { 0 };

    // Return last frame timestamp
    if (packet && (last = g_list_last_data(packet->frames))) {
        ts.tv_sec = last->ts.tv_sec;
        ts.tv_usec = last->ts.tv_usec;
    }

    // Return packe timestamp
    return ts;
}

void
packet_frame_free(PacketFrame *frame)
{
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
