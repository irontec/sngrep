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
 * @file packet_udp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage UDP protocol
 */

#ifndef __SNGREP_PACKET_UDP_H__
#define __SNGREP_PACKET_UDP_H__

#include <glib.h>

G_BEGIN_DECLS

#define PACKET_DISSECTOR_TYPE_UDP packet_dissector_udp_get_type()
G_DECLARE_FINAL_TYPE(PacketDissectorUdp, packet_dissector_udp, PACKET_DISSECTOR, UDP, PacketDissector)

typedef struct _PacketUdpData PacketUdpData;

struct _PacketDissectorUdp
{
    //! Parent structure
    PacketDissector parent;
};

struct _PacketUdpData
{
    //! Source Port
    guint16 sport;
    //! Destination Port
    guint16 dport;
};

/**
 * @brief Retrieve packet UDP protocol specific data
 * @param packet Packet pointer to get data
 * @return Pointer to PacketUdpData | NULL
 */
PacketUdpData *
packet_udp_data(const Packet *packet);

/**
 * @brief Create an UDP parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_dissector_udp_new();

G_END_DECLS

#endif  /* __SNGREP_PACKET_UDP_H__ */
