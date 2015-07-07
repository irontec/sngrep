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
 */
#ifndef __SNGREP_RTP_H
#define __SNGREP_RTP_H

#include "config.h"
#include "capture.h"
#include "media.h"

#define RTP_FORMAT_MASK     0x7F

//! Shorter declaration of rtp_encoding structure
typedef struct rtp_encoding rtp_encoding_t;
//! Shorter declaration of rtp_stream structure
typedef struct rtp_stream rtp_stream_t;

struct rtp_encoding
{
    u_char id;
    const char *name;
    const char *format;
};

struct rtp_stream
{
    //! Source address and port
    char ip_src[ADDRESSLEN];
    u_short sport;
    //! Destination address and port
    char ip_dst[ADDRESSLEN];
    u_short dport;
    //! Format of first received packet of stre
    u_char format;
    //! Time of first received packet of stream
    struct timeval time;
    //! Packet count for this stream
    int pktcnt;
    //! Mark if address has been bound
    int complete;
    //! SDP media that setup this stream
    sdp_media_t *media;
    //! Next stream in the call
    rtp_stream_t *next;
};

rtp_stream_t *
stream_create(sdp_media_t *media, const char *dst, u_short dport);

rtp_stream_t *
stream_complete(rtp_stream_t *stream, const char *src, u_short sport);

void
stream_add_packet(rtp_stream_t *stream, const char *ip_src, u_short sport, const char *ip_dst,
                  u_short dport, u_char format, struct timeval time);

int
stream_get_count(rtp_stream_t *stream);

struct sip_call *
stream_get_call(rtp_stream_t *stream);

const char *
rtp_get_codec(int code, const char *format);

rtp_stream_t *
rtp_check_stream(const struct pcap_pkthdr *header, const char *src, u_short sport, const char* dst, u_short dport, u_char *payload);

rtp_stream_t *
rtp_find_stream(const char *ip_src, u_short sport, const char *ip_dst, u_short dport);

rtp_stream_t *
rtp_find_call_stream(struct sip_call *call, const char *ip_src, u_short sport, const char *ip_dst, u_short dport);

#endif /* __SNGREP_RTP_H */
