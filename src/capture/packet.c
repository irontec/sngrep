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
#include "capture/packet/packet_ip.h"
#include "capture/packet/packet_tcp.h"
#include "capture/packet/packet_udp.h"
#include "capture/packet/packet_sip.h"
#include "capture/capture_pcap.h"
#include "packet.h"

Packet *
packet_new()
{
    // Create a new packet
    Packet *packet = g_malloc0(sizeof(Packet));
    packet->proto = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_set_size(packet->proto, PACKET_PROTO_COUNT);
    return packet;
}

void
packet_free(Packet *packet)
{
    g_return_if_fail(packet != NULL);
    g_ptr_array_free(packet->proto, TRUE);
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
    PacketIpData *ip = g_ptr_array_index(packet->proto, PACKET_IP);
    g_return_val_if_fail(ip, address);
    g_utf8_strncpy(address.ip, ip->saddr.ip, ADDRESSLEN);

    // Get Port from UDP or TCP parsed protocol
    if (packet_has_type(packet, PACKET_UDP)) {
        PacketUdpData *udp = g_ptr_array_index(packet->proto, PACKET_UDP);
        g_return_val_if_fail(udp, address);
        address.port = udp->sport;
    } else {
        PacketUdpData *tcp = g_ptr_array_index(packet->proto, PACKET_TCP);
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
    PacketIpData *ip = g_ptr_array_index(packet->proto, PACKET_IP);
    g_return_val_if_fail(ip, address);
    g_utf8_strncpy(address.ip, ip->daddr.ip, ADDRESSLEN);

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
    PacketFrame *first;
    GTimeVal ts = { 0 };

    // Return first frame timestamp
    if (packet && (first = g_list_nth_data(packet->frames, 0))) {
        ts.tv_sec = first->ts.tv_sec;
        ts.tv_usec = first->ts.tv_usec;
    }

    // Return packe timestamp
    return ts;
}
