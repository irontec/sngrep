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
 * @file proto.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage captured packet protocols
 *
 */

#ifndef __SNGREP_PACKET_PROTO_H
#define __SNGREP_PACKET_PROTO_H

#include <glib.h>
#include "parser.h"
#include "packet.h"

/**
 * @brief Packet handler interface
 *
 * A packet handler is able to check raw captured data from the wire
 * and convert it into Packets to be stored.
 */
struct _PacketDissector
{
    //! Protocol id
    enum packet_proto id;
    //! SubProtocol children
    GSList *subdissectors;

    //! Protocol initialization funtion
    void (*init)(PacketParser *parser);
    //! Protocol raw packet data parser
    GByteArray* (*dissect)(PacketParser *parser, Packet *packet, GByteArray *data);
    //! Protocol deinitialization funtion
    void (*deinit)(PacketParser *parser);
};

#endif