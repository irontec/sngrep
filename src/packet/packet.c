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
#include "glib-extra/glib.h"
#include "packet_ip.h"
#include "packet_tcp.h"
#include "packet_udp.h"
#include "packet.h"
#include "storage/storage.h"

/**
 * @brief Packet class definition
 */
G_DEFINE_TYPE(Packet, packet, G_TYPE_OBJECT)

void
packet_set_protocol_data(Packet *packet, G_GNUC_UNUSED PacketProtocolId id, gpointer data)
{
    packet->proto = g_slist_append(packet->proto, data);
}

gpointer
packet_get_protocol_data(const Packet *packet, PacketProtocolId id)
{
    for (GSList *l = packet->proto; l != NULL; l = l->next) {
        PacketProtocol *proto = l->data;
        if (proto->id == id) {
            return l->data;
        }
    }
    return NULL;
}

gboolean
packet_has_protocol(const Packet *packet, PacketProtocolId proto)
{
    return packet_get_protocol_data(packet, proto) != NULL;
}

Address
packet_src_address(Packet *packet)
{
    Address addr = { 0 };

    // Get IP address from IP parsed protocol
    PacketIpData *ip = packet_get_protocol_data(packet, PACKET_PROTO_IP);
    g_return_val_if_fail(ip, addr);
    addr.ip = ip->srcip;

    // Get Port from UDP or TCP parsed protocol
    if (packet_has_protocol(packet, PACKET_PROTO_UDP)) {
        PacketUdpData *udp = packet_get_protocol_data(packet, PACKET_PROTO_UDP);
        g_return_val_if_fail(udp, addr);
        addr.port = udp->sport;
    } else {
        PacketTcpData *tcp = packet_get_protocol_data(packet, PACKET_PROTO_TCP);
        g_return_val_if_fail(tcp, addr);
        addr.port = tcp->sport;
    }

    return addr;
}

Address
packet_dst_address(Packet *packet)
{
    Address addr = ADDRESS_ZERO;

    // Get IP address from IP parsed protocol
    PacketIpData *ip = packet_get_protocol_data(packet, PACKET_PROTO_IP);
    g_return_val_if_fail(ip, addr);
    addr.ip = ip->dstip;

    // Get Port from UDP or TCP parsed protocol
    if (packet_has_protocol(packet, PACKET_PROTO_UDP)) {
        PacketUdpData *udp = packet_get_protocol_data(packet, PACKET_PROTO_UDP);
        g_return_val_if_fail(udp, addr);
        addr.port = udp->dport;
    } else {
        PacketTcpData *tcp = packet_get_protocol_data(packet, PACKET_PROTO_TCP);
        g_return_val_if_fail(tcp, addr);
        addr.port = tcp->dport;
    }

    return addr;
}

const char *
packet_transport(Packet *packet)
{
    if (packet_has_protocol(packet, PACKET_PROTO_UDP))
        return "UDP";

    if (packet_has_protocol(packet, PACKET_PROTO_TCP)) {
        if (packet_has_protocol(packet, PACKET_PROTO_WS)) {
            return (packet_has_protocol(packet, PACKET_PROTO_TLS)) ? "WSS" : "WS";
        }
        return packet_has_protocol(packet, PACKET_PROTO_TLS) ? "TLS" : "TCP";
    }
    return "???";
}

CaptureInput *
packet_get_input(Packet *packet)
{
    return packet->input;
}

guint64
packet_time(const Packet *packet)
{
    PacketFrame *last = g_list_last_data(packet->frames);
    g_return_val_if_fail(last != NULL, 0);
    return last->ts;
}

gint
packet_time_sorter(const Packet **a, const Packet **b)
{
    return (gint) (packet_time(*a) - packet_time(*b));
}

const PacketFrame *
packet_first_frame(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);
    g_return_val_if_fail(packet->frames != NULL, NULL);
    return g_list_first_data(packet->frames);
}

guint64
packet_frame_seconds(const PacketFrame *frame)
{
    g_return_val_if_fail(frame != NULL, 0);
    return frame->ts / G_USEC_PER_SEC;
}

guint64
packet_frame_microseconds(const PacketFrame *frame)
{
    g_return_val_if_fail(frame != NULL, 0);
    return frame->ts - ((frame->ts / G_USEC_PER_SEC) * G_USEC_PER_SEC);
}

void
packet_frame_free(PacketFrame *frame)
{
    g_bytes_unref(frame->data);
    g_free(frame);
}

PacketFrame *
packet_frame_new()
{
    PacketFrame *frame = g_malloc0(sizeof(PacketFrame));
    return frame;
}

static void
packet_proto_free(gpointer data, Packet *packet)
{
    PacketProtocol *proto = data;
    g_return_if_fail(proto != NULL);

    // Use dissector free function
    PacketDissector *dissector = storage_find_dissector(proto->id);
    packet_dissector_free_data(dissector, packet);

    // Remove protocol information from the list
    packet->proto = g_slist_remove(packet->proto, data);
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

static void
packet_finalize(GObject *self)
{
    Packet *packet = SNGREP_PACKET(self);

    // Free each protocol data
    g_slist_foreach(packet->proto, (GFunc) packet_proto_free, packet);
    g_slist_free(packet->proto);

    // Free each frame data
    g_list_free_full(packet->frames, (GDestroyNotify) packet_frame_free);

    // Chain GObject dispose
    G_OBJECT_CLASS(packet_parent_class)->dispose(self);
}

Packet *
packet_new(CaptureInput *input)
{
    // Create a new packet
    Packet *packet = g_object_new(CAPTURE_TYPE_PACKET, NULL);
    packet->input = input;
    return packet;
}

static void
packet_init(Packet *self)
{
    self->proto = NULL;
    self->frames = NULL;
}

static void
packet_class_init(PacketClass *klass)
{
    // Get the parent gobject class
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    // Set finalize functions
    object_class->finalize = packet_finalize;
}
