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
 * @file packet.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet.h
 *
 */
#include "config.h"
#include <glib.h>
#include "glib-extra.h"
#include "capture/dissectors/packet_ip.h"
#include "capture/dissectors/packet_tcp.h"
#include "capture/dissectors/packet_udp.h"
#include "capture/dissectors/packet_sip.h"
#include "capture/capture_pcap.h"
#include "packet.h"

Packet *
packet_new(PacketParser *parser)
{
    // Create a new packet
    Packet *packet = g_malloc0(sizeof(Packet));
    packet->parser = parser;
    packet->proto = g_ptr_array_sized_new(PACKET_PROTO_COUNT);
    g_ptr_array_set_size(packet->proto, PACKET_PROTO_COUNT);
    return packet;
}

void
packet_free(Packet *packet)
{
    g_return_if_fail(packet != NULL);

    // Free each protocol data
    for (guint i = 0; i < g_ptr_array_len(packet->proto); i++) {
        if (g_ptr_array_index(packet->proto, i) != NULL) {
            packet_parser_dissector_free(packet->parser, packet, i);
        }
    }
    g_ptr_array_free(packet->proto, TRUE);

    // Free each frame data
    for (GList *l = packet->frames; l != NULL; l = l->next) {
        PacketFrame *frame = l->data;
        g_byte_array_free(frame->data, TRUE);
    }
    g_list_free_full(packet->frames, g_free);

    g_free(packet);
}

gboolean
packet_has_type(const Packet *packet, enum PacketProtoId type)
{
    return g_ptr_array_index(packet->proto, type) != NULL;
}

Address
packet_src_address(const Packet *packet)
{
    Address address = { 0 };

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
    Address address = { 0 };

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
    g_free(frame);
}

PacketFrame *
packet_frame_new()
{
    PacketFrame *frame = g_malloc0(sizeof(PacketFrame));
    return frame;
}
