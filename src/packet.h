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

#ifndef __SNGREP_CAPTURE_PACKET_H
#define __SNGREP_CAPTURE_PACKET_H

#include <time.h>
#include <sys/types.h>
#include <pcap.h>
#include "address.h"
#include "vector.h"

//! Stored packet types
enum packet_type {
    PACKET_SIP_UDP = 0,
    PACKET_SIP_TCP,
    PACKET_SIP_TLS,
    PACKET_SIP_WS,
    PACKET_SIP_WSS,
    PACKET_RTP,
    PACKET_RTCP,
};

//! Shorter declaration of packet structure
typedef struct packet packet_t;
//! Shorter declaration of frame structure
typedef struct frame frame_t;

/**
 * @brief Packet capture data.
 *
 * One packet can contain more than one frame after assembly. We assume than
 * one SIP message has one packet (maybe in multiple frames) and that one
 * packet can only contain one SIP message.
 */
struct packet {
    //! IP protocol
    uint8_t ip_version;
    //! Transport protocol
    uint8_t proto;
    //! Packet type as defined in capture_packet_type
    enum packet_type type;
    //! Source
    address_t src;
    //! Destination
    address_t dst;
    //! Packet IP id
    uint16_t ip_id;
    //! Packet IP fragmentation captured data
    uint32_t ip_cap_len;
    //! Packet IP fragmentation expected data
    uint32_t ip_exp_len;
    //! Last TCP sequence frame
    uint32_t tcp_seq;
    //! PCAP Packet payload when it can not be get from data
    u_char *payload;
    //! Payload length
    uint32_t payload_len;
    //! Packet frame list (frame_t)
    vector_t *frames;
};

/**
 *  @brief Capture frame.
 *
 *  One packet can contain multiple frames. This structure is designed to store
 *  the required information to save a packet into a PCAP file.
 */
struct frame {
    //! PCAP Frame Header data
    struct pcap_pkthdr *header;
    //! PCAP Frame content
    u_char *data;
};

/**
 * @brief Allocate memory to store new packet data
 */
packet_t *
packet_create(uint8_t ip_ver, uint8_t proto, address_t src, address_t dst, uint32_t id);

/**
 * @brief Deep clone one packet
 */
packet_t*
packet_clone(packet_t *packet);

/**
 * @brief Set Transport layer information
 */
packet_t *
packet_set_transport_data(packet_t *pkt, uint16_t sport, uint16_t dport);

/**
 * @brief Add a new frame to the given packet
 */
frame_t *
packet_add_frame(packet_t *pkt, const struct pcap_pkthdr *header, const u_char *packet);

/**
 * @brief Deallocate a packet structure memory
 */
void
packet_destroy(packet_t *packet);

/**
 * @brief Destroyer function for packet vectors
 */
void
packet_destroyer(void *packet);

/**
 * @brief Free packet frames data.
 *
 * This can be used to avoid storing packet payload in memory or disk
 */
void
packet_free_frames(packet_t *pkt);

/**
 * @brief Set packet type
 */
void
packet_set_type(packet_t *packet, enum packet_type type);

/**
 * @brief Set packet payload when it can not be get from packet
 */
void
packet_set_payload(packet_t *packet, u_char *payload, uint32_t payload_len);

/**
 * @brief Getter for capture payload size
 */
uint32_t
packet_payloadlen(packet_t *packet);

/**
 * @brief Getter for capture payload pointer
 */
u_char *
packet_payload(packet_t *packet);

/**
 * @brief Get The timestamp for a packet.
 */
struct timeval
packet_time(packet_t *packet);

#endif /* __SNGREP_CAPTURE_PACKET_H */
