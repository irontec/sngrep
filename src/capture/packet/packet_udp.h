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
 * @file packet_udp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage UDP protocol
 *
 *
 */
#ifndef __SNGREP_PACKET_UDP_H
#define __SNGREP_PACKET_UDP_H

#include "capture/parser.h"

typedef struct _PacketUdpData PacketUdpData;

struct _PacketUdpData
{
    //! Source Port
    guint16 sport;
    //! Destination Port
    guint16 dport;
};

/**
 * @brief Create an UDP parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_udp_new();

#endif
