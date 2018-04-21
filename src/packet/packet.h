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
#include <time.h>
#include <sys/types.h>
#include "packet/old_packet.h"
#include "packet/address.h"

/**
 * @brief Packet protocols
 *
 * Note that packet types are stored as flags and a packet have more than
 * one type.
 */
enum packet_proto {
    PACKET_LINK = 0,
    PACKET_IP   = 1,
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

/**
 * @brief Packet capture data.
 *
 * One packet can contain more than one frame after assembly. We assume than
 * one SIP message has one packet (maybe in multiple frames) and that one
 * packet can only contain one SIP message.
 */
struct _Packet {
//    //! Packet types (bit flags array)
//    guint16 types;
//    //! Source address and port data
//    Address src;
//    //! Destination address and port data
//    Address dst;
//    //! Assembled/Decoded full packet payload
//    GByteArray *payload;
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
struct _PacketFrame {
    //! PCAP Frame Header data
    struct pcap_pkthdr *header;
    //! PCAP Frame content
    guchar *data;
};

Packet *
packet_new();

void
packet_free(Packet *packet);

///**
// * @brief Create a new frame from libpcap data
// *
// */
//PacketFrame *
//packet_frame_create(const struct pcap_pkthdr *header, const guchar *packet);
//
///**
// * @brief Add a new frame to the given packet
// */
//void
//packet_add_frame(Packet *pkt, PacketFrame *frame);

gboolean
packet_has_type(const Packet *packet, enum packet_proto type);

Address
packet_src_address(const Packet *packet);

Address
packet_dst_address(const Packet *packet);

//const char *
//packet_transport(Packet *packet);

packet_t *
packet_to_oldpkt(Packet *packet);

#endif /* __SNGREP_PACKET_H */
