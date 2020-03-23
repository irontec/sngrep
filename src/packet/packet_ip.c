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
 */
#include "config.h"
#include <arpa/inet.h>
#include "glib-extra/glib.h"
#include "packet.h"
#include "packet_ip.h"

G_DEFINE_TYPE(PacketDissectorIp, packet_dissector_ip, PACKET_TYPE_DISSECTOR)

PacketIpData *
packet_ip_data(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);
    return packet_get_protocol_data(packet, PACKET_PROTO_IP);
}

static gint
packet_ip_fragment_sort(const PacketIpFragment **a, const PacketIpFragment **b)
{
    return (*a)->frag_off - (*b)->frag_off;
}

static PacketIpFragment *
packet_ip_fragment_new(Packet *packet, GBytes *data)
{
    // Reserve memory for storing fragment information
    PacketIpFragment *fragment = g_malloc0(sizeof(PacketIpFragment));
    // Store packet information
    fragment->packet = packet_ref(packet);
    // Set fragment payload for future reassembly
    fragment->data = g_bytes_ref(data);
    return fragment;
}

static void
packet_ip_fragment_free(PacketIpFragment *fragment)
{
    // Remove no longer required data
    g_bytes_unref(fragment->data);
    packet_unref(fragment->packet);
    g_free(fragment);
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

static GBytes *
packet_ip_datagram_payload(PacketIpDatagram *datagram)
{
    // Join all fragment payload
    GByteArray *data = g_byte_array_new();
    for (guint i = 0; i < g_ptr_array_len(datagram->fragments); i++) {
        PacketIpFragment *fragment = g_ptr_array_index(datagram->fragments, i);
        g_byte_array_append(
            data,
            g_bytes_get_data(fragment->data, NULL),
            g_bytes_get_size(fragment->data)
        );
    }

    return g_byte_array_free_to_bytes(data);
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

static PacketIpDatagram *
packet_dissector_ip_find_datagram(PacketDissectorIp *dissector, PacketIpFragment *fragment)
{
    for (GList *l = dissector->assembly; l != NULL; l = l->next) {
        PacketIpDatagram *datagram = l->data;
        if (g_strcmp0(fragment->srcip, datagram->srcip) == 0
            && g_strcmp0(fragment->dstip, datagram->dstip) == 0
            && fragment->id == datagram->id) {
            return datagram;
        }
    }
    return NULL;
}

static GBytes *
packet_dissector_ip_dissect(PacketDissector *self, Packet *packet, GBytes *data)
{
    // Get IP dissector information
    g_return_val_if_fail(PACKET_DISSECTOR_IS_IP(self), NULL);
    PacketDissectorIp *dissector = PACKET_DISSECTOR_IP(self);

    // Get IP header
    struct ip *ip4 = (struct ip *) g_bytes_get_data(data, NULL);

#ifdef USE_IPV6
    // Get IPv6 header
    struct ip6_hdr *ip6 = (struct ip6_hdr *) g_bytes_get_data(data, NULL);
#endif

    // Create an IP fragment for current data
    PacketIpFragment *fragment = packet_ip_fragment_new(packet, data);

    // Set IP version
    fragment->version = ip4->ip_v;

    // Get IP version
    switch (fragment->version) {
        case 4:
            fragment->hl = (guint32) ip4->ip_hl * 4;
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
    PacketIpData *ip_data = g_malloc0(sizeof(PacketIpData));
    ip_data->proto.id = PACKET_PROTO_IP;
    ip_data->srcip = g_strdup(fragment->srcip);
    ip_data->dstip = g_strdup(fragment->dstip);
    ip_data->version = fragment->version;
    ip_data->protocol = fragment->proto;
    packet_set_protocol_data(packet, PACKET_PROTO_IP, ip_data);

    // Get pending payload
    data = g_bytes_offset(data, fragment->hl);

    // Remove any payload trailer (trust IP len content field)
    data = g_bytes_set_size(data, fragment->len - fragment->hl);

    // If no fragmentation
    if (fragment->frag == 0) {
        // Single fragment packet
        packet_ip_fragment_free(fragment);
        // Call next dissector
        return packet_dissector_next(self, packet, data);
    }

    // Look for another packet with same id in IP reassembly list
    PacketIpDatagram *datagram = packet_dissector_ip_find_datagram(dissector, fragment);

    // Create a new datagram if none matches
    if (datagram == NULL) {
        datagram = packet_ip_datagram_new(fragment);
        dissector->assembly = g_list_append(dissector->assembly, datagram);
    }

    // Add fragment to the datagram
    g_ptr_array_add(datagram->fragments, fragment);

    // Calculate how much data we need to complete this packet
    // The total packet size can only be known using the last fragment of the packet
    // where 'No more fragments is enabled' and it's calculated based on the
    // last fragment offset
    if (fragment->more == 0) {
        datagram->len = fragment->frag_off + g_bytes_get_size(data);
    }

    // Add this IP content length to the total captured of the packet
    datagram->seen += g_bytes_get_size(data);

    // If we have the whole packet (captured length is expected length)
    if (datagram->seen == datagram->len) {
        // Sort IP fragments
        g_ptr_array_sort(datagram->fragments, (GCompareFunc) packet_ip_fragment_sort);
        // Sort and glue all fragments payload
        data = packet_ip_datagram_payload(datagram);
        // Sort and take packet frames
        packet->frames = packet_ip_datagram_take_frames(datagram);
        // Remove the datagram information
        dissector->assembly = g_list_remove(dissector->assembly, datagram);
        packet_ip_datagram_free(datagram);
        // Call next dissector
        return packet_dissector_next(self, packet, data);
    }

    // Packet handled and stored for IP assembly
    return data;
}

static void
packet_dissector_ip_finalize(GObject *self)
{
    // Get Dissector data for this packet
    g_return_if_fail(PACKET_DISSECTOR_IS_IP(self));
    PacketDissectorIp *dissector = PACKET_DISSECTOR_IP(self);

    // Free used memory
    g_list_free_full(dissector->assembly, (GDestroyNotify) packet_ip_datagram_free);
}

static void
packet_dissector_ip_free_data(Packet *packet)
{
    PacketIpData *ip_data = packet_ip_data(packet);
    g_return_if_fail(ip_data != NULL);
    g_free(ip_data->srcip);
    g_free(ip_data->dstip);
    g_free(ip_data);
}

static void
packet_dissector_ip_class_init(PacketDissectorIpClass *klass)
{
    PacketDissectorClass *dissector_class = PACKET_DISSECTOR_CLASS(klass);
    dissector_class->dissect = packet_dissector_ip_dissect;
    dissector_class->free_data = packet_dissector_ip_free_data;

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = packet_dissector_ip_finalize;
}

static void
packet_dissector_ip_init(PacketDissectorIp *self)
{
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_UDP);
    packet_dissector_add_subdissector(PACKET_DISSECTOR(self), PACKET_PROTO_TCP);
}

PacketDissector *
packet_dissector_ip_new()
{
    return g_object_new(
        PACKET_DISSECTOR_TYPE_IP,
        "id", PACKET_PROTO_IP,
        "name", "IP",
        NULL
    );
}
