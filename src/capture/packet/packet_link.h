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
 * @file packet_link.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to link layer packet contents
 */

#ifndef __SNGREP_PROTO_LINK_H_
#define __SNGREP_PROTO_LINK_H_

#include <glib.h>
#include <netinet/if_ether.h>
#ifdef SLL_HDR_LEN
#include <pcap/sll.h>
#endif
#include "capture/parser.h"

//! Define VLAN 802.1Q Ethernet type
#ifndef ETHERTYPE_8021Q
#define ETHERTYPE_8021Q 0x8100
#endif

//! NFLOG Support (for libpcap <1.6.0)
#define DLT_NFLOG       239
#define NFULA_PAYLOAD   9

typedef struct _DissectorLinkData DissectorLinkData;
typedef struct _LinkNflogHdr LinkNflogHdr;

//! Private information structure for Link Protocol
struct _DissectorLinkData
{
    gint link_type;
    gint link_size;
};

struct _LinkNflogHdr
{
    guint16 tlv_length;
    guint16 tlv_type;
};

/**
 * @brief Create a Link layer parser
 *
 * @return a protocols' parsers tree
 */
PacketDissector *
packet_link_new();

guint8
proto_link_size(int linktype);

#endif /* __SNGREP_PROTO_LINK_H_ */
