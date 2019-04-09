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
 *
 */

#include "config.h"
#include <netinet/udp.h>
#include "glib/glib-extra.h"
#include "packet_ip.h"
#include "capture/packet.h"
#include "packet_udp.h"

static GByteArray *
packet_udp_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    struct udphdr *udp = (struct udphdr *) data->data;
    uint16_t udp_off = sizeof(struct udphdr);

    //! Is this a IP/TCP packet?
    PacketIpData *ipdata = packet_ip_data(packet);
    if (ipdata->protocol != IPPROTO_UDP)
        return data;

    // Check payload can contain an UDP header
    if (data->len < udp_off)
        return NULL;

    // UDP packet data
    PacketUdpData *udp_data = g_malloc0(sizeof(PacketUdpData));

    // Set packet ports
#ifdef __FAVOR_BSD
    udp_data->sport = g_htons(udp->uh_sport);
    udp_data->dport = g_htons(udp->uh_dport);
#else
    udp_data->sport = g_htons(udp->source);
    udp_data->dport = g_htons(udp->dest);
#endif

    // Store udp data
    g_ptr_array_set(packet->proto, PACKET_UDP, udp_data);

    // Get pending payload
    g_byte_array_remove_range(data, 0, udp_off);

    // Call next dissector
    return packet_parser_next_dissector(parser, packet, data);

}

static void
packet_udp_free(G_GNUC_UNUSED PacketParser *parser, Packet *packet)
{
    g_return_if_fail(packet != NULL);

    PacketUdpData *udp_data = g_ptr_array_index(packet->proto, PACKET_UDP);
    g_return_if_fail(udp_data);

    g_free(udp_data);
}

PacketDissector *
packet_udp_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_UDP;
    proto->dissect = packet_udp_parse;
    proto->free = packet_udp_free;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SIP));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_RTP));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_RTCP));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_HEP));

    return proto;
}
