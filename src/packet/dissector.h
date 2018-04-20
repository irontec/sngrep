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
 * @file dissector.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage captured packet protocols handlers
 *
 */

#ifndef __SNGREP_DISSECTOR_H
#define __SNGREP_DISSECTOR_H

#include <glib.h>
#include "parser.h"
#include "packet.h"

//! Dissector type
typedef struct _PacketDissector PacketDissector;

//! Dissector functions types
typedef GByteArray *(*PacketDissectorDissectFunc)(PacketParser *, Packet *, GByteArray *);

typedef void (*PacketDissectorInitFunc)(PacketParser *);

typedef void (*PacketDissectorDeinitFunc)(PacketParser *);

/**
 * @brief Packet dissector interface
 *
 * A packet handler is able to check raw captured data from the wire
 * and convert it into Packets to be stored.
 */
struct _PacketDissector
{
    //! Protocol id
    enum packet_proto id;
    //! SubProtocol children dissectors
    GSList *subdissectors;

    //! Protocol initialization funtion
    PacketDissectorInitFunc init;
    //! Protocol packet dissector function
    PacketDissectorDissectFunc dissect;
    //! Protocol deinitialization funtion
    PacketDissectorDeinitFunc deinit;
};

#endif