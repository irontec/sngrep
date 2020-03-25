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
#include "capture/capture_pcap.h"
#include "packet_ip.h"
#include "packet_link.h"
#include "glib-extra/glib.h"

G_DEFINE_TYPE(PacketDissectorLink, packet_dissector_link, PACKET_TYPE_DISSECTOR)

static GBytes *
packet_dissector_link_dissect(PacketDissector *self, Packet *packet, GBytes *data)
{
    // Get capture input from this packet
    CaptureInput *input = packet->input;
    g_return_val_if_fail(input, NULL);

    // Packet Link packet only works with PCAP input
    g_return_val_if_fail(capture_input_tech(input) == CAPTURE_TECH_PCAP, NULL);

    // Initialize packet private data
    CaptureInputPcap *pcap = CAPTURE_INPUT_PCAP(input);
    gint link_type = capture_input_pcap_datalink(input);
    guint8 link_size = packet_link_size(pcap->link);

    // Get Layer header size from link type
    guint offset = (guint) link_size;

    // For ethernet, skip VLAN header if present
    if (link_type == DLT_EN10MB) {
        struct ether_header *eth = (struct ether_header *) g_bytes_get_data(data, NULL);
        if (g_ntohs(eth->ether_type) == ETHERTYPE_8021Q) {
            offset += 4;
        }
    }

#ifdef DLT_LINUX_SLL
    if (link_type == DLT_LINUX_SLL) {
        struct sll_header *sll = (struct sll_header *) packet;
        if (g_ntohs(sll->sll_protocol) == ETHERTYPE_8021Q) {
            offset += 4;
        }
    }
#endif

    // Skip NFLOG header if present
    if (link_type == DLT_NFLOG) {
        // Parse NFLOG TLV headers
        while (offset + 8 <= g_bytes_get_size(data)) {
            data = g_bytes_offset(data, offset);
            LinkNflogHdr *tlv = (LinkNflogHdr *) g_bytes_get_data(data, NULL);

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
    if (g_bytes_get_size(data) <= offset) {
        return NULL;
    }

    // Update pending data
    data = g_bytes_offset(data, offset);

    // Call next dissector
    return packet_dissector_next(self, packet, data);
}

guint8
packet_link_size(gint link_type)
{
    // Datalink header size
    switch (link_type) {
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
            g_printerr("Unsupported datalink type: %d", link_type);
            return 0;
    }
}

static void
packet_dissector_link_class_init(PacketDissectorLinkClass *klass)
{
    PacketDissectorClass *dissector_class = PACKET_DISSECTOR_CLASS(klass);
    dissector_class->dissect = packet_dissector_link_dissect;
}

static void
packet_dissector_link_init(PacketDissectorLink *self)
{
    packet_dissector_add_subdissector(
        PACKET_DISSECTOR(self),
        PACKET_PROTO_IP
    );
}

PacketDissector *
packet_dissector_link_new()
{
    return g_object_new(
        PACKET_DISSECTOR_TYPE_LINK,
        "id", PACKET_PROTO_LINK,
        "name", "LINK",
        NULL
    );
}
