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
 * @file packet_parser.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage captured packet parsers
 *
 */

#ifndef __SNGREP_PACKET_PARSER_H
#define __SNGREP_PACKET_PARSER_H

#include "packet.h"

//! Shorter declaration of packet parser structure
typedef struct _PacketParser PacketParser;
//! Forward declaration of Capture input structure
typedef struct _CaptureInput CaptureInput;
//! Forward declaration of packet parser structure
typedef struct _PacketDissector PacketDissector;

/**
 * @brief Packet parser interface
 *
 * A packet parser stores the information of the protocol
 * parser for a capture input
 */
struct _PacketParser
{
    //! Capture input owner of this parser
    CaptureInput *input;
    //! Protocol lists handled by this parser
    GPtrArray *dissectors;
    //! Protocol dissection tree
    GNode *dissector_tree;
    //! Protocl node actually parsing
    GNode *current;
};



/**
 * @brief Create a new packet parser for given Capture Input
 *
 * @return allocated PacketParser structure pointer
 */
PacketParser *
packet_parser_new(CaptureInput *input);

/**
 * @brief Free parser and associated protocols memory
 *
 * @note This function will free all parser protocols memory
 *
 * @param parser
 */
void
packet_parser_free(PacketParser *parser);

/**
 * @brief Add a new protocol handler to the parser
 *
 * @note This function will initialize protocol memory
 *
 * @param parser PacketParser holding the protocols
 * @param proto Protocol id to be added
 * @return
 */
PacketDissector *
packet_parser_add_proto(PacketParser *parser, GNode *parent, enum packet_proto id);

/**
 * @brief Send packet data to current dissector children
 *
 * @param parser PacketParser holding the protocols
 * @param packet
 * @param data
 * @return
 */
GByteArray *
packet_parser_next_dissector(PacketParser *parser, Packet *packet, GByteArray *data);


#endif