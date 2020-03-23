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
 * @file packet_link.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to link layer packet contents
 */

#ifndef __SNGREP_PACKET_LINK_H__
#define __SNGREP_PACKET_LINK_H__

#include <glib.h>
#include <netinet/if_ether.h>
#ifdef SLL_HDR_LEN
#include <pcap/sll.h>
#endif
#include "dissector.h"

G_BEGIN_DECLS

//! Define VLAN 802.1Q Ethernet type
#ifndef ETHERTYPE_8021Q
#define ETHERTYPE_8021Q 0x8100
#endif

//! NFLOG Support (for libpcap <1.6.0)
#define DLT_NFLOG       239
#define NFULA_PAYLOAD   9

#define PACKET_DISSECTOR_TYPE_LINK packet_link_get_type()
G_DECLARE_FINAL_TYPE(PacketDissectorLink, packet_link, PACKET_DISSECTOR, LINK, PacketDissector)

struct _PacketDissectorLink
{
    PacketDissector parent;
};

typedef struct
{
    guint16 tlv_length;
    guint16 tlv_type;
} LinkNflogHdr;

/**
 * @brief Return the number of bytes used by Link Layer Header
 * @param link_type Datalink value provided by libpcap
 * @return Size in bytes of Link header
 */
guint8
packet_link_size(gint link_type);

/**
 * @brief Create a Link layer parser
 *
 * @return a protocols' parsers tree
 */
PacketDissector *
packet_dissector_link_new();

G_END_DECLS

#endif /* __SNGREP_PACKET_LINK_H__ */
