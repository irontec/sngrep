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
#include "packet/dissectors/packet_ip.h"
#include "packet/dissectors/packet_tcp.h"
#include "packet/dissectors/packet_udp.h"
#include "packet/dissectors/packet_sip.h"
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
packet_has_type(const Packet *packet, enum packet_proto type)
{
    return g_ptr_array_index(packet->proto, type) != NULL;
}

Address
packet_src_address(const Packet *packet)
{
    Address address;

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
    Address address;

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

packet_t *
packet_to_oldpkt(Packet *packet)
{
    packet_t *oldpkt = g_malloc0(sizeof(packet_t));

    PacketIpData *ipdata = g_ptr_array_index(packet->proto, PACKET_IP);
    oldpkt->newpacket = packet;
    oldpkt->src = ipdata->saddr;
    oldpkt->dst = ipdata->daddr;

    if (packet_has_type(packet, PACKET_TCP)) {
        PacketTcpData *tcpdata = g_ptr_array_index(packet->proto, PACKET_TCP);
        oldpkt->src.port = tcpdata->sport;
        oldpkt->dst.port = tcpdata->dport;
    } else {
        PacketUdpData *udpdata = g_ptr_array_index(packet->proto, PACKET_UDP);
        oldpkt->src.port = udpdata->sport;
        oldpkt->dst.port = udpdata->dport;
    }

    PacketSipData *sipdata = g_ptr_array_index(packet->proto, PACKET_SIP);
    if (sipdata) {
        packet_set_payload(oldpkt, sipdata->payload, strlen(sipdata->payload));
    }

    oldpkt->frames = g_sequence_new(NULL);
    for (GList *l = packet->frames; l != NULL; l = l->next) {
        PacketFrame *frame = l->data;
        packet_add_frame(oldpkt, frame->header, frame->data);
    }

    return oldpkt;
}
