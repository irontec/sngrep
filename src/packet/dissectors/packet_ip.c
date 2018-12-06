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
 * @file packet_ip.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_ip.h
 *
 * Support for IPv4 and IPv6 packets
 *
 */

#include "config.h"
#include <netdb.h>
#include <arpa/inet.h>
#include "glib-extra.h"
#include "packet/packet.h"
#include "packet/parser.h"
#include "packet/dissectors/packet_ip.h"

static gint
packet_ip_sort_fragments(const PacketIpFragment **a, const PacketIpFragment **b)
{
    return (*b)->frag_off - (*a)->frag_off;
}

static GByteArray *
packet_ip_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    DissectorIpData *priv = g_ptr_array_index(parser->dissectors, PACKET_IP);
    g_return_val_if_fail(priv != NULL, NULL);

    // Get IP header
    struct ip *ip4 = (struct ip *) data->data;

#ifdef USE_IPV6
    // Get IPv6 header
    struct ip6_hdr *ip6 = (struct ip6_hdr *) data->data;
#endif

    // Reserve memory for storing fragment information
    PacketIpFragment *fragment = g_malloc0(sizeof(PacketIpFragment));

    // Store packet information
    fragment->packet = packet;

    // Set IP version
    fragment->version = ip4->ip_v;

    // Get IP version
    switch (fragment->version) {
        case 4:
            fragment->hl = ip4->ip_hl * 4;
            fragment->proto = ip4->ip_p;
            fragment->off = g_ntohs(ip4->ip_off);
            fragment->len = g_ntohs(ip4->ip_len);

            fragment->frag = (guint16) (fragment->off & (IP_MF | IP_OFFMASK));
            fragment->frag_off = (guint16) ((fragment->frag) ? (fragment->off & IP_OFFMASK) * 8 : 0);
            fragment->id = g_ntohs(ip4->ip_id);
            fragment->more = (guint16) (fragment->off & IP_MF);

            // Get source and destination IP addresses
            inet_ntop(AF_INET, &ip4->ip_src, fragment->src.ip, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &ip4->ip_dst, fragment->dst.ip, INET_ADDRSTRLEN);
            break;
#ifdef USE_IPV6
        case 6:
            fragment->hl = sizeof(struct ip6_hdr);
            fragment->proto = ip6->ip6_nxt;
            fragment->len = g_ntohs(ip6->ip6_ctlun.ip6_un1.ip6_un1_plen) + fragment->hl;

            if (fragment->proto == IPPROTO_FRAGMENT) {
                struct ip6_frag *ip6f = (struct ip6_frag *) (ip6 + fragment->hl);
                fragment->frag_off = g_ntohs(ip6f->ip6f_offlg & IP6F_OFF_MASK);
                fragment->id = ntohl(ip6f->ip6f_ident);
            }

            // Get source and destination IP addresses
            inet_ntop(AF_INET6, &ip6->ip6_src, fragment->src.ip, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &ip6->ip6_dst, fragment->dst.ip, INET6_ADDRSTRLEN);
            break;
#endif
        default:
            return NULL;
    }

    // IP packet without payload
    if (fragment->len == 0)
        return NULL;

    // Save IP Addresses into packet
    PacketIpData *ipdata = g_malloc0(sizeof(PacketIpData));
    ipdata->saddr       = fragment->src;
    ipdata->daddr       = fragment->dst;
    ipdata->version     = fragment->version;
    ipdata->protocol    = fragment->proto;
    packet->proto->pdata[PACKET_IP] = ipdata;

    // Get pending payload
    g_byte_array_remove_range(data, 0, fragment->hl);

    // Remove any payload trailer (trust IP len content field)
    g_byte_array_set_size(data, fragment->len - fragment->hl);

    // If no fragmentation
    if (fragment->frag == 0) {
        // Single fragment packet
        g_free(fragment);
        // Call next dissector
        return packet_parser_next_dissector(parser, packet, data);
    }

    // Set fragment payload for future reassembly
    fragment->data = data;

    // Look for another packet with same id in IP reassembly vector
    PacketIpDatagram *datagram = NULL;
    for (GList *l = priv->assembly; l != NULL; l = l->next) {
        datagram = l->data;
        if (addressport_equals(fragment->src, datagram->src)
            && addressport_equals(fragment->dst, datagram->dst)
            && fragment->id == datagram->id) {
            break;
        }
        datagram = NULL;
    }

    // If we already have this packet datagram, add a new fragment
    if (datagram != NULL) {
        g_ptr_array_add(datagram->fragments, fragment);
    } else {
        datagram = g_malloc0(sizeof(PacketIpDatagram));
        datagram->src = fragment->src;
        datagram->dst = fragment->dst;
        datagram->id  = fragment->id;
        datagram->fragments = g_ptr_array_new_with_free_func(g_free);
        g_ptr_array_add(datagram->fragments, fragment);
        priv->assembly = g_list_append(priv->assembly, datagram);
    }

    // Add this IP content length to the total captured of the packet
    datagram->seen += data->len;

    // Calculate how much data we need to complete this packet
    // The total packet size can only be known using the last fragment of the packet
    // where 'No more fragments is enabled' and it's calculated based on the
    // last fragment offset
    if (fragment->more == 0) {
        datagram->len = fragment->frag_off + data->len;
    }

    // If we have the whole packet (captured length is expected length)
    if (datagram->seen == datagram->len) {
        // Calculate assembled IP payload data
        g_ptr_array_sort(datagram->fragments, (GCompareFunc) packet_ip_sort_fragments);

        // Join all fragment payload
        GList *frames = NULL;
        data = g_byte_array_new();
        for (guint i = 0; i < datagram->fragments->len; i++) {
            fragment = g_ptr_array_index(datagram->fragments, i);
            g_byte_array_append(data, fragment->data->data, fragment->data->len);
            // Store all the fragments in current packet
            frames = g_list_append(frames, fragment->packet->frames);
            // Free fragment data
            g_byte_array_free(fragment->data, FALSE);
        }

        // Remove the datagram information
        priv->assembly = g_list_remove(priv->assembly, datagram);
        g_ptr_array_free(datagram->fragments, TRUE);
        g_free(datagram);

        // Call next dissector
        return packet_parser_next_dissector(parser, packet, data);
    }

    return data;
}

static void
packet_ip_init(PacketParser *parser)
{
    // Initialize parser private data
    DissectorIpData *ipdata = g_malloc0(sizeof(DissectorIpData));
    g_return_if_fail(ipdata != NULL);

    // Store parser private information
    g_ptr_array_set(parser->dissectors, PACKET_IP, ipdata);

}

static void
packet_ip_deinit(PacketParser *parser)
{
    // Get Dissector data for this parser
    DissectorIpData *ipdata = g_ptr_array_index(parser->dissectors, PACKET_IP);
    g_return_if_fail(ipdata != NULL);

    // Free used memory
    g_list_free(ipdata->assembly);
}

PacketDissector*
packet_ip_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_IP;
    proto->init = packet_ip_init;
    proto->dissect = packet_ip_parse;
    proto->deinit = packet_ip_deinit;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_UDP));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_TCP));
    return proto;
}
