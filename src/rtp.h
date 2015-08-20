/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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
#include "capture.h"
#include "media.h"

// Version is the first 2 bits of the first octet
#define RTP_VERSION(octet) ((octet) >> 6)
// Payload type is the last 7 bits
#define RTP_PAYLOAD_TYPE(octet) ((octet) & 0x7F)

// Handled RTP versions
#define RTP_VERSION_RFC1889 2

//! Shorter declaration of rtp_encoding structure
typedef struct rtp_encoding rtp_encoding_t;
//! Shorter declaration of rtp_stream structure
typedef struct rtp_stream rtp_stream_t;

struct rtp_encoding {
    u_int id;
    const char *name;
    const char *format;
};

struct rtp_stream {
    //! Source address and port
    char ip_src[ADDRESSLEN];
    u_short sport;
    //! Destination address and port
    char ip_dst[ADDRESSLEN];
    u_short dport;
    //! Format of first received packet of stre
    u_int fmtcode;
    //! Time of first received packet of stream
    struct timeval time;
    //! Packet count for this stream
    int pktcnt;
    //! SDP media that setup this stream
    sdp_media_t *media;
};

rtp_stream_t *
stream_create(sdp_media_t *media, const char *dst, u_short dport);

rtp_stream_t *
stream_complete(rtp_stream_t *stream, const char *src, u_short sport, u_int format);

void
stream_add_packet(rtp_stream_t *stream, const struct pcap_pkthdr *header);

int
stream_get_count(rtp_stream_t *stream);

struct sip_call *
stream_get_call(rtp_stream_t *stream);

const char *
stream_get_format(rtp_stream_t *stream);

const char *
rtp_get_standard_format(u_int code);

rtp_stream_t *
rtp_check_stream(capture_packet_t *packet, const char *src, u_short sport, const char* dst, u_short dport);

rtp_stream_t *
rtp_find_stream(const char *ip_src, u_short sport, const char *ip_dst, u_short dport, u_int format);

rtp_stream_t *
rtp_find_call_stream(struct sip_call *call, const char *ip_src, u_short sport, const char *ip_dst, u_short dport, u_int format);

#endif /* __SNGREP_RTP_H */
