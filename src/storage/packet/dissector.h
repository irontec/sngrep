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
 * @file dissector.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage dissectors
 *
 */

#ifndef __SNGREP_DISSECTOR_H__
#define __SNGREP_DISSECTOR_H__

#include "config.h"
#include <glib.h>
#include "packet.h"

G_BEGIN_DECLS

#define PACKET_TYPE_DISSECTOR packet_dissector_get_type()
G_DECLARE_DERIVABLE_TYPE(PacketDissector, packet_dissector, PACKET, DISSECTOR, GObject)

/**
 * @brief Packet dissector interface
 *
 * A packet handler is able to check raw captured data from the wire
 * and convert it into Packets to be stored.
 */
struct _PacketDissectorClass
{
    //! Parent class
    GObjectClass parent;
    //! Protocol packet dissector function
    GByteArray *(*dissect)(PacketDissector *self, Packet *packet, GByteArray *data);
    //! Packet data free function
    void (*free_data)(Packet *package);
};

void
packet_dissector_set_protocol(PacketDissector *self, PacketProtocolId id);

void
packet_dissector_add_subdissector(PacketDissector *self, PacketProtocolId id);

GByteArray *
packet_dissector_dissect(PacketDissector *self, Packet *packet, GByteArray *data);

void
packet_dissector_free_data(PacketDissector *self, Packet *packet);

GByteArray *
packet_dissector_next_proto(PacketProtocolId id, Packet *packet, GByteArray *data);

GByteArray *
packet_dissector_next(PacketDissector *current, Packet *packet, GByteArray *data);

G_END_DECLS

#endif /* __SNGREP_DISSECTOR_H__ */
