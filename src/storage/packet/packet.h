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

#ifndef __SNGREP_PACKET_H__
#define __SNGREP_PACKET_H__

#include <glib.h>
#include <glib-object.h>
#include <time.h>
#include <sys/types.h>
#include "storage/address.h"

G_BEGIN_DECLS

#define CAPTURE_TYPE_PACKET packet_get_type()

G_DECLARE_FINAL_TYPE(Packet, packet, SNGREP, PACKET, GObject)

/**
 * @brief Packet protocols
 *
 * Note that packet types are stored as flags and a packet have more than
 * one type.
 */
typedef enum
{
    PACKET_PROTO_LINK = 0,
    PACKET_PROTO_IP = 1,
    PACKET_PROTO_UDP,
    PACKET_PROTO_TCP,
    PACKET_PROTO_TLS,
    PACKET_PROTO_WS,
    PACKET_PROTO_SIP,
    PACKET_PROTO_SDP,
    PACKET_PROTO_RTP,
    PACKET_PROTO_RTCP,
    PACKET_PROTO_HEP,
    PACKET_PROTO_COUNT
} PacketProtocol;

//! Shorter declaration of packet structure
typedef struct _Packet Packet;
//! Shorter declaration of frame structure
typedef struct _PacketFrame PacketFrame;
//! Forward declaration for CaptureInput
typedef struct _CaptureInput CaptureInput;

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
    //! Capture input that generated this packet
    CaptureInput *input;
    //! Packet Source Address
    Address *src;
    //! Packet Destination Adddress
    Address *dst;
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
    //! Capture length (effective)
    guint32 len;
    //! Capture length (from wire)
    guint32 caplen;
    //! PCAP Frame content
    GByteArray *data;
};

Packet *
packet_new(CaptureInput *input);

Packet *
packet_ref(Packet *packet);

void
packet_unref(Packet *packet);

void
packet_set_protocol_data(Packet *packet, PacketProtocol proto, gpointer data);

gpointer
packet_get_protocol_data(const Packet *packet, PacketProtocol proto);

gboolean
packet_has_protocol(const Packet *packet, PacketProtocol proto);

Address *
packet_src_address(Packet *packet);

Address *
packet_dst_address(Packet *packet);

const char *
packet_transport(Packet *packet);

CaptureInput *
packet_get_input(Packet *packet);

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
 * @brief Return first packet captured frame
 */
const PacketFrame *
packet_first_frame(const Packet *packet);

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

#endif  /* __SNGREP_PACKET_H__ */
