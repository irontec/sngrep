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
 * @file packet_link.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_link.h
 */

#include "config.h"
#include <glib.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <pcap.h>
#ifdef DLT_LINUX_SLL
#include <pcap/sll.h>
#endif
#include "glib-extra.h"
#include "capture/capture_pcap.h"
#include "packet_ip.h"
#include "packet_link.h"

static GByteArray *
packet_link_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    DissectorLinkData *priv = g_ptr_array_index(parser->dissectors, PACKET_LINK);
    g_return_val_if_fail(priv, NULL);

    // Get Layer header size from link type
    guint offset = (guint) priv->link_size;

    // For ethernet, skip VLAN header if present
    if (priv->link_type == DLT_EN10MB) {
        struct ether_header *eth = (struct ether_header *) data->data;
        if (g_ntohs(eth->ether_type) == ETHERTYPE_8021Q) {
            offset += 4;
        }
    }

#ifdef DLT_LINUX_SLL
    if (priv->link_type == DLT_LINUX_SLL) {
        struct sll_header *sll = (struct sll_header *) packet;
        if (g_ntohs(sll->sll_protocol) == ETHERTYPE_8021Q) {
            offset += 4;
        }
    }
#endif

    // Skip NFLOG header if present
    if (priv->link_type == DLT_NFLOG) {
        // Parse NFLOG TLV headers
        while (offset + 8 <= data->len) {
            LinkNflogHdr *tlv = (LinkNflogHdr *) (packet + offset);

            if (!tlv) break;

            if (tlv->tlv_type == NFULA_PAYLOAD) {
                offset += 4;
                break;
            }

            if (tlv->tlv_length >= 4) {
                offset += ((tlv->tlv_length + 3) & ~3); /* next TLV aligned to 4B */
            }
        }
    }

    // Not enough data
    if (data->len <= offset) {
        return NULL;
    }

    // Update pending data
    data = g_byte_array_remove_range(data, 0, offset);

    // Call next dissector
    return packet_parser_next_dissector(parser, packet, data);

}

guint8
proto_link_size(int linktype)
{
    // Datalink header size
    switch (linktype) {
        case DLT_EN10MB:
            return 14;
        case DLT_IEEE802:
            return 22;
        case DLT_LOOP:
        case DLT_NULL:
            return 4;
        case DLT_SLIP:
        case DLT_SLIP_BSDOS:
            return 16;
        case DLT_PPP:
        case DLT_PPP_BSDOS:
        case DLT_PPP_SERIAL:
        case DLT_PPP_ETHER:
            return 4;
        case DLT_RAW:
            return 0;
        case DLT_FDDI:
            return 21;
        case DLT_ENC:
            return 12;
        case DLT_NFLOG:
            return 4;
#ifdef DLT_LINUX_SLL
        case DLT_LINUX_SLL:
            return 16;
#endif
#ifdef DLT_IPNET
        case DLT_IPNET:
            return 24;
#endif
        default:
            // Not handled datalink type
            g_printerr("Unsupported datalink type: %d", linktype);
            return 0;
    }
}

static void
packet_link_init(PacketParser *parser)
{
    // Get capture input from this parser
    CaptureInput *input = parser->input;
    g_return_if_fail(input);
    // Packet Link parser only works with PCAP input
    g_return_if_fail(input->tech == CAPTURE_TECH_PCAP);
    // Check input has its private data initialized
    g_return_if_fail(input->priv);

    CapturePcap *pcap = (CapturePcap *) input->priv;

    // Initialize parser private data
    DissectorLinkData *link_data = g_malloc0(sizeof(DissectorLinkData));
    link_data->link_type = pcap->link;
    link_data->link_size = proto_link_size(pcap->link);

    // Store private data for this protocol
    g_ptr_array_set(parser->dissectors, PACKET_LINK, link_data);
}

static void
packet_link_deinit(G_GNUC_UNUSED PacketParser *parser)
{
//    g_free(proto->priv);
//    g_free(proto);
}

PacketDissector *
packet_link_new()
{
    PacketDissector *dissector = g_malloc0(sizeof(PacketDissector));
    dissector->id = PACKET_LINK;
    dissector->init = packet_link_init;
    dissector->dissect = packet_link_parse;
    dissector->deinit = packet_link_deinit;
    dissector->subdissectors = g_slist_append(dissector->subdissectors, GUINT_TO_POINTER(PACKET_IP));
    return dissector;
}
