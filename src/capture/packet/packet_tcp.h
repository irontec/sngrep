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
 * @file packet_tcp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage TCP protocol
 *
 *
 */
#ifndef __SNGREP_PACKET_TCP_H
#define __SNGREP_PACKET_TCP_H

#include <netinet/tcp.h>
#include <glib.h>
#include "capture/address.h"
#include "capture/parser.h"

//! Ignore too segmented TCP packets
#define TCP_MAX_SEGMENTS    5

typedef struct _PacketTcpStream PacketTcpStream;
typedef struct _PacketTcpSegment PacketTcpSegment;
typedef struct _PacketTcpData PacketTcpData;
typedef struct _DissectorTcpData DissectorTcpData;

struct _PacketTcpStream
{
    //! Packet IP addresses
    Address src, dst;
    //! TCP Segment list
    GPtrArray *segments;
};

struct _PacketTcpSegment
{
    GByteArray *data;
    Packet *packet;
};

struct _PacketTcpData
{
    Address src;
    Address dst;
    guint16 off;
    guint16 syn;
    guint16 ack;
    guint32 seq;
    guint16 psh;
};

struct _DissectorTcpData
{
    GHashTable *assembly;
};

/**
 * @brief Create a TCP parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_tcp_new();

#endif
