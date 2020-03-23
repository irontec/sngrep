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
 * @file packet_udp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_udp.h
 *
 * Support for UDP transport layer
 */

#include "config.h"
#include <glib.h>
#include <netinet/udp.h>
#include "glib-extra/glib.h"
#include "packet_ip.h"
#include "packet.h"
#include "packet_udp.h"

G_DEFINE_TYPE(PacketDissectorUdp, packet_dissector_udp, PACKET_TYPE_DISSECTOR)

PacketUdpData *
packet_udp_data(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);
    return packet_get_protocol_data(packet, PACKET_PROTO_UDP);
}

static GBytes *
packet_dissector_udp_dissect(PacketDissector *self, Packet *packet, GBytes *data)
{
    struct udphdr *udp = (struct udphdr *) g_bytes_get_data(data, NULL);
    uint16_t udp_off = sizeof(struct udphdr);

    //! Is this a IP/TCP packet?
    PacketIpData *ip_data = packet_ip_data(packet);
    g_return_val_if_fail(ip_data != NULL, data);
    if (ip_data->protocol != IPPROTO_UDP)
        return data;

    // Check payload can contain an UDP header
    if (g_bytes_get_size(data) < udp_off)
        return NULL;

    // UDP packet data
    PacketUdpData *udp_data = g_malloc0(sizeof(PacketUdpData));
    udp_data->proto.id = PACKET_PROTO_UDP;

    // Set packet ports
#ifdef __FAVOR_BSD
    udp_data->sport = g_htons(udp->uh_sport);
    udp_data->dport = g_htons(udp->uh_dport);
#else
    udp_data->sport = g_htons(udp->source);
    udp_data->dport = g_htons(udp->dest);
#endif

    // Store udp data
    packet_set_protocol_data(packet, PACKET_PROTO_UDP, udp_data);

    // Get pending payload
    data = g_bytes_offset(data, udp_off);

    // Call next dissector
    return packet_dissector_next(self, packet, data);
}

static void
packet_dissector_udp_free_data(Packet *packet)
{
    PacketUdpData *udp_data = packet_udp_data(packet);
    g_return_if_fail(udp_data != NULL);
    g_free(udp_data);
}

static void
packet_dissector_udp_class_init(PacketDissectorUdpClass *klass)
{
    PacketDissectorClass *dissector_class = PACKET_DISSECTOR_CLASS(klass);
    dissector_class->dissect = packet_dissector_udp_dissect;
    dissector_class->free_data = packet_dissector_udp_free_data;
}

static void
packet_dissector_udp_init(PacketDissectorUdp *self)
{
    // UDP Dissector base information
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_SIP);
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_RTP);
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_RTCP);
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_HEP);
}

PacketDissector *
packet_dissector_udp_new()
{
    return g_object_new(
        PACKET_DISSECTOR_TYPE_UDP,
        "id", PACKET_PROTO_UDP,
        "name", "UDP",
        NULL
    );
}
