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
 * @file packet.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage captured packet
 *
 * Capture packet contains the information about a one or more packets captured
 * from network interface or readed from a .PCAP file.
 *
 * The binary content of the packet can be stored in one or more frames (if
 * packet has been reassembled).
 *
 */

#ifndef __SNGREP_PACKET_H
#define __SNGREP_PACKET_H

#include <glib.h>
#include <glib-object.h>
#include <time.h>
#include <sys/types.h>
#include "address.h"

G_BEGIN_DECLS

//! Protocol type macros
#define packet_add_type(packet, type, data) (g_ptr_array_set(packet->proto, type, data))
#define packet_has_type(packet, type)       (g_ptr_array_index(packet->proto, type) != NULL)


/**
 * @brief Packet protocols
 *
 * Note that packet types are stored as flags and a packet have more than
 * one type.
 */
enum PacketProtoId
{
    PACKET_LINK = 0,
    PACKET_IP = 1,
    PACKET_UDP,
    PACKET_TCP,
    PACKET_TLS,
    PACKET_WS,
    PACKET_SIP,
    PACKET_SDP,
    PACKET_RTP,
    PACKET_RTCP,
    PACKET_HEP,
    PACKET_PROTO_COUNT
};

//! Shorter declaration of packet structure
typedef struct _Packet Packet;
//! Shorter declaration of frame structure
typedef struct _PacketFrame PacketFrame;

// Forward declarations
struct _PacketParser;
typedef struct _PacketParser PacketParser;

/**
 * @brief Packet capture data.
 *
 * One packet can contain more than one frame after assembly. We assume than
 * one SIP message has one packet (maybe in multiple frames) and that one
 * packet can only contain one SIP message.
 */
struct _Packet
{
    //! Parent class
    GObject parent;
    //! Packet Source Address
    Address *src;
    //! Packet Destination Adddress
    Address *dst;
    //! Parser who processed this packet
    PacketParser *parser;
    //! Each packet protocol information
    GPtrArray *proto;
    //! Packet frame list (frame_t)
    GList *frames;
};

/**
 *  @brief Capture frame.
 *
 *  One packet can contain multiple frames. This structure is designed to store
 *  the required information to save a packet into a PCAP file.
 */
struct _PacketFrame
{
    //! Frame received time (microseconds)
    guint64 ts;
    //! Capture lenght (effective)
    guint32 len;
    //! Capture lenght (from wire)
    guint32 caplen;
    //! PCAP Frame content
    GByteArray *data;
};

#define CAPTURE_TYPE_PACKET packet_get_type()

G_DECLARE_FINAL_TYPE(Packet, packet, SNGREP, PACKET, GObject)

Packet *
packet_new(PacketParser *parser);

void
packet_free(Packet *packet);

Packet *
packet_ref(Packet *packet);

void
packet_unref(Packet *packet);

Address *
packet_src_address(Packet *packet);

Address *
packet_dst_address(Packet *packet);

const char *
packet_transport(Packet *packet);

/**
 * @brief Get The timestamp for a packet.
 */
guint64
packet_time(const Packet *packet);

/**
 * @brief Sorter by time for captured packets
 */
gint
packet_time_sorter(const Packet **a, const Packet **b);

/**
 * @brief Return frame received unix timestamp seconds
 */
guint64
packet_frame_seconds(const PacketFrame *frame);

/**
 * @brief Return frame received timestamp microseconds
 */
guint64
packet_frame_microseconds(const PacketFrame *frame);

/**
 * @brief Free allocated memory in Packet frame
 * @param frame Frame pointer to be free'd
 */
void
packet_frame_free(PacketFrame *frame);

/**
 * @brief Create a new packet frame to store captured data
 * @return pointer to a new allocated frame structure | NULL
 */
PacketFrame *
packet_frame_new();

G_END_DECLS

#endif /* __SNGREP_PACKET_H */
