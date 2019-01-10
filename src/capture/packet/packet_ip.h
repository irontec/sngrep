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
 * @file packet_ip.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage IPv4 and IPv6 protocol
 *
 */
#ifndef __SNGREP_PACKET_IP_H
#define __SNGREP_PACKET_IP_H

#ifdef USE_IPV6
#include <netinet/ip6.h>
#endif

#include <glib.h>
#include <netinet/ip.h>
#include "capture/packet.h"
#include "capture/address.h"
#include "capture/parser.h"

typedef struct _PacketIpData PacketIpData;
typedef struct _PacketIpDatagram PacketIpDatagram;
typedef struct _PacketIpFragment PacketIpFragment;
typedef struct _DissectorIpData DissectorIpData;

struct _PacketIpData
{
    //! Version (IPv4, IPv6)
    guint32 version;
    //! IP Protocol
    guint8 protocol;
    //! Source Address
    Address saddr;
    //! Destination Address
    Address daddr;
};

struct _PacketIpDatagram
{
    //! Packet IP addresses
    Address src, dst;
    //! Fragmentation identifier
    guint32 id;
    //! Datagram length
    guint32 len;
    //! Datagram seen bytes
    guint32 seen;
    //! Fragments
    GPtrArray *fragments;
};

//! @brief IP assembly data.
struct _PacketIpFragment
{
    //! Packet IP addresses
    Address src, dst;
    // IP version
    guint32 version;
    // IP transport dissectors
    guint8 proto;
    // IP header size
    guint32 hl;
    // Fragment offset
    guint16 off;
    // IP content len
    guint32 len;
    // Fragmentation flag
    guint16 frag;
    // Fragmentation identifier
    guint32 id;
    // Fragmentation offset
    guint16 frag_off;
    //! More fragments expected
    guint16 more;
    //! Packets with this frame data
    Packet *packet;
    //! Fragment contents
    GByteArray *data;
};

struct _DissectorIpData
{
    GList *assembly;
};

/**
 * @brief Create a IP parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_ip_new();

#endif
