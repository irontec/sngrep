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
 * @file rtp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage rtp captured packets
 *
 * @note RTP_VERSION and RTP_PAYLOAD_TYPE macros has been taken from wireshark
 *       source code: packet-rtp.c
 */

#ifndef __SNGREP_RTP_H
#define __SNGREP_RTP_H

#include "config.h"
#include "sip_msg.h"
#include "capture/capture_pcap.h"
#include "packet/dissectors/packet_sdp.h"
#include "packet/dissectors/packet_rtp.h"
#include "packet/dissectors/packet_rtcp.h"

//! Shorter declaration of rtp_stream structure
typedef struct rtp_stream rtp_stream_t;

struct rtp_stream {
    //! Determine stream type
    uint32_t type;
    //! Source address
    Address src;
    //! Destination address
    Address dst;
    //! SDP media that setup this stream
    PacketSdpMedia *media;
    //! SIP message that setup this stream
    sip_msg_t *msg;
    //! Packet count for this stream
    uint32_t pktcnt;
    //! Time of first received packet of stream
    struct timeval time;
    //! Unix timestamp of last received packet
    int lasttm;

    // Stream information (depending on type)
    union {
        struct {
            //! Format of first received packet of stre
            uint32_t fmtcode;
        } rtpinfo;
        struct {
            //! Sender packet count
            uint32_t spc;
            //! Fraction lost x/256
            uint8_t flost;
            //! uint8_t discarded x/256
            uint8_t fdiscard;
            //! MOS - listening Quality
            uint8_t mosl;
            //! MOS - Conversational Quality
            uint8_t mosc;
        } rtcpinfo;
    };
};

rtp_stream_t *
stream_create(Packet *packet, PacketSdpMedia *media);

rtp_stream_t *
stream_complete(rtp_stream_t *stream, Address src);

void
stream_set_format(rtp_stream_t *stream, uint32_t format);

void
stream_add_packet(rtp_stream_t *stream, packet_t *packet);

uint32_t
stream_get_count(rtp_stream_t *stream);

struct sip_call *
stream_get_call(rtp_stream_t *stream);

const char *
stream_get_format(rtp_stream_t *stream);

rtp_stream_t *
rtp_check_packet(packet_t *packet);

rtp_stream_t *
rtp_find_stream_format(Address src, Address dst, uint32_t format);

rtp_stream_t *
rtp_find_stream(Address src, Address dst);

rtp_stream_t *
rtp_find_call_stream(struct sip_call *call, Address src, Address dst);

rtp_stream_t *
rtp_find_call_exact_stream(struct sip_call *call, Address src, Address dst);

/**
 * @brief Check if a message is older than other
 *
 * @param one rtp stream pointer
 * @param two rtp stream pointer
 * @return 1 if one is older than two
 * @return 0 if equal or two is older than one
 */
int
stream_is_older(rtp_stream_t *one, rtp_stream_t *two);

int
stream_is_complete(rtp_stream_t *stream);

/**
 * @brief Determine if a stream is still active
 *
 * This simply checks the timestamp of the last received packet of the stream
 * marking as inactive if it was before STREAM_INACTIVE_SECS ago
 *
 * @return 1 if stream is active
 * @return 0 if stream is inactive
 */
int
stream_is_active(rtp_stream_t *stream);

/**
 * @brief Check if the data is a RTP packet
 * RFC 5761 Section 4.  Distinguishable RTP and RTCP Packets
 * RFC 5764 Section 5.1.2.  Reception (packet demultiplexing)
 */
int
data_is_rtp(u_char *data, uint32_t len);

/**
 * @brief Check if the data is a RTCP packet
 * RFC 5761 Section 4.  Distinguishable RTP and RTCP Packets
 * RFC 5764 Section 5.1.2.  Reception (packet demultiplexing)
 */
int
data_is_rtcp(u_char *data, uint32_t len);

#endif /* __SNGREP_RTP_H */
