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
#include "glib/glib-extra.h"
#include "parser/packet.h"
#include "parser/parser.h"
#include "packet_ip.h"

PacketIpData *
packet_ip_data(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);
    return g_ptr_array_index(packet->proto, PACKET_IP);
}

static PacketIpFragment *
packet_ip_fragment_new(Packet *packet, GByteArray *data)
{
    // Reserve memory for storing fragment information
    PacketIpFragment *fragment = g_malloc0(sizeof(PacketIpFragment));
    // Store packet information
    fragment->packet = packet_ref(packet);
    // Set fragment payload for future reassembly
    fragment->data = g_byte_array_ref(data);
    return fragment;
}

static void
packet_ip_fragment_free(PacketIpFragment *fragment)
{
    // Remove no longer required data
    g_byte_array_unref(fragment->data);
    packet_unref(fragment->packet);
    g_free(fragment);
}

static PacketIpDatagram *
packet_ip_find_datagram(DissectorIpData *priv, PacketIpFragment *fragment)
{
    for (GList *l = priv->assembly; l != NULL; l = l->next) {
        PacketIpDatagram *datagram = l->data;
        if (g_strcmp0(fragment->srcip, datagram->srcip) == 0
            && g_strcmp0(fragment->dstip, datagram->dstip) == 0
            && fragment->id == datagram->id) {
            return datagram;
        }
    }
    return NULL;
}

static PacketIpDatagram *
packet_ip_datagram_new(const PacketIpFragment *fragment)
{
    PacketIpDatagram *datagram = g_malloc0(sizeof(PacketIpDatagram));
    datagram->fragments = g_ptr_array_new_with_free_func((GDestroyNotify) packet_ip_fragment_free);

    // Copy fragment data
    g_utf8_strncpy(datagram->srcip, fragment->srcip, ADDRESSLEN);
    g_utf8_strncpy(datagram->dstip, fragment->dstip, ADDRESSLEN);
    datagram->id = fragment->id;

    return datagram;
}

static void
packet_ip_datagram_free(PacketIpDatagram *datagram)
{
    // Free all datagram fragments
    g_ptr_array_free(datagram->fragments, TRUE);
    // Free datagram
    g_free(datagram);
}

static gint
packet_ip_sort_fragments(const PacketIpFragment **a, const PacketIpFragment **b)
{
    return (*a)->frag_off - (*b)->frag_off;
}

static GByteArray *
packet_ip_datagram_payload(PacketIpDatagram *datagram)
{
    // Join all fragment payload
    GByteArray *data = g_byte_array_new();
    for (guint i = 0; i < g_ptr_array_len(datagram->fragments); i++) {
        PacketIpFragment *fragment = g_ptr_array_index(datagram->fragments, i);
        g_byte_array_append(data, fragment->data->data, fragment->data->len);
    }

    return data;
}

static GList *
packet_ip_datagram_take_frames(PacketIpDatagram *datagram)
{
    GList *frames = NULL;
    for (guint i = 0; i < g_ptr_array_len(datagram->fragments); i++) {
        PacketIpFragment *fragment = g_ptr_array_index(datagram->fragments, i);
        g_return_val_if_fail(fragment != NULL, NULL);
        Packet *packet = fragment->packet;
        g_return_val_if_fail(packet != NULL, NULL);

        // Append frames to datagram list
        frames = g_list_concat_deep(frames, packet->frames);

        // Remove fragment frames (but not free them!)
        g_list_free(packet->frames);
        packet->frames = NULL;
    }

    return frames;
}

static GByteArray *
packet_ip_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    DissectorIpData *priv = g_ptr_array_index(parser->dissectors_priv, PACKET_IP);
    g_return_val_if_fail(priv != NULL, NULL);

    // Get IP header
    struct ip *ip4 = (struct ip *) data->data;

#ifdef USE_IPV6
    // Get IPv6 header
    struct ip6_hdr *ip6 = (struct ip6_hdr *) data->data;
#endif

    // Create an IP fragment for current data
    PacketIpFragment *fragment = packet_ip_fragment_new(packet, data);

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
            inet_ntop(AF_INET, &ip4->ip_src, fragment->srcip, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &ip4->ip_dst, fragment->dstip, INET_ADDRSTRLEN);
            break;
#ifdef USE_IPV6
        case 6:
            fragment->hl = sizeof(struct ip6_hdr);
            fragment->proto = ip6->ip6_nxt;
            fragment->len = g_ntohs(ip6->ip6_ctlun.ip6_un1.ip6_un1_plen) + fragment->hl;

            if (fragment->proto == IPPROTO_FRAGMENT) {
                struct ip6_frag *ip6f = (struct ip6_frag *) (ip6 + fragment->hl);
                fragment->frag_off = g_ntohs(ip6f->ip6f_offlg & IP6F_OFF_MASK);
                fragment->id = g_ntohl(ip6f->ip6f_ident);
            }

            // Get source and destination IP addresses
            inet_ntop(AF_INET6, &ip6->ip6_src, fragment->srcip, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &ip6->ip6_dst, fragment->dstip, INET6_ADDRSTRLEN);
            break;
#endif
        default:
            packet_ip_fragment_free(fragment);
            return data;
    }

    // IP packet without payload
    if (fragment->len == 0) {
        packet_ip_fragment_free(fragment);
        return data;
    }

    // Save IP Addresses into packet
    PacketIpData *ipdata = g_malloc0(sizeof(PacketIpData));
    g_utf8_strncpy(ipdata->srcip, fragment->srcip, ADDRESSLEN);
    g_utf8_strncpy(ipdata->dstip, fragment->dstip, ADDRESSLEN);
    ipdata->version = fragment->version;
    ipdata->protocol = fragment->proto;
    packet_add_type(packet, PACKET_IP, ipdata);

    // Get pending payload
    g_byte_array_remove_range(data, 0, fragment->hl);

    // Remove any payload trailer (trust IP len content field)
    g_byte_array_set_size(data, fragment->len - fragment->hl);

    // If no fragmentation
    if (fragment->frag == 0) {
        // Single fragment packet
        packet_ip_fragment_free(fragment);
        // Call next dissector
        return packet_parser_next_dissector(parser, packet, data);
    }

    // Look for another packet with same id in IP reassembly list
    PacketIpDatagram *datagram = packet_ip_find_datagram(priv, fragment);

    // Create a new datagram if none matches
    if (datagram == NULL) {
        datagram = packet_ip_datagram_new(fragment);
        priv->assembly = g_list_append(priv->assembly, datagram);
    }

    // Add fragment to the datagram
    g_ptr_array_add(datagram->fragments, fragment);

    // Calculate how much data we need to complete this packet
    // The total packet size can only be known using the last fragment of the packet
    // where 'No more fragments is enabled' and it's calculated based on the
    // last fragment offset
    if (fragment->more == 0) {
        datagram->len = fragment->frag_off + data->len;
    }

    // Add this IP content length to the total captured of the packet
    datagram->seen += data->len;

    // If we have the whole packet (captured length is expected length)
    if (datagram->seen == datagram->len) {
        // Sort IP fragments
        g_ptr_array_sort(datagram->fragments, (GCompareFunc) packet_ip_sort_fragments);
        // Sort and glue all fragments payload
        data = packet_ip_datagram_payload(datagram);
        // Sort and take packet frames
        packet->frames = packet_ip_datagram_take_frames(datagram);
        // Remove the datagram information
        priv->assembly = g_list_remove(priv->assembly, datagram);
        packet_ip_datagram_free(datagram);
        // Call next dissector
        return packet_parser_next_dissector(parser, packet, data);
    }

    // Packet handled and stored for IP assembly
    return data;
}

static void
packet_ip_free(G_GNUC_UNUSED PacketParser *parser, Packet *packet)
{
    PacketIpData *ipdata = packet_ip_data(packet);
    g_return_if_fail(ipdata != NULL);

    g_free(ipdata);
}

static void
packet_ip_init(PacketParser *parser)
{
    // Initialize parser private data
    DissectorIpData *priv = g_malloc0(sizeof(DissectorIpData));
    g_return_if_fail(priv != NULL);

    // Store parser private information
    g_ptr_array_set(parser->dissectors_priv, PACKET_IP, priv);

}

static void
packet_ip_deinit(PacketParser *parser)
{
    // Get Dissector data for this parser
    DissectorIpData *priv = g_ptr_array_index(parser->dissectors_priv, PACKET_IP);
    g_return_if_fail(priv != NULL);

    // Free used memory
    g_list_free_full(priv->assembly, (GDestroyNotify) packet_ip_datagram_free);
    g_free(priv);
}

PacketDissector *
packet_ip_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_IP;
    proto->init = packet_ip_init;
    proto->deinit = packet_ip_deinit;
    proto->dissect = packet_ip_parse;
    proto->free = packet_ip_free;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_UDP));
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_TCP));
    return proto;
}
