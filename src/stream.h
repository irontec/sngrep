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
 * @file stream.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage rtp streams
 *
 * @note RTP_VERSION and RTP_PAYLOAD_TYPE macros has been taken from wireshark
 *       source code: packet-rtp.c
 */

#ifndef __SNGREP_STREAM_H
#define __SNGREP_STREAM_H

#include "config.h"
#include "sip_msg.h"
#include "capture/capture_pcap.h"
#include "packet/dissectors/packet_sdp.h"
#include "packet/dissectors/packet_rtp.h"
#include "packet/dissectors/packet_rtcp.h"

//! Shorter declaration of rtp_stream structure
typedef struct _RtpStream RtpStream;

enum RtpStreamType
{
    STREAM_RTP      = 0,
    STREAM_RTCP
};

struct _RtpStream
{
    //! Determine stream type
    enum RtpStreamType type;
    //! Source address
    Address src;
    //! Destination address
    Address dst;
    //! SDP media that setup this stream
    PacketSdpMedia *media;
    //! SIP message that setup this stream
    SipMsg *msg;
    //! Unix timestamp of last received packet
    gint64 lasttm;
    //! Format of first received packet of stre
    guint8 fmtcode;
    //! List of stream packets
    GPtrArray *packets;
};

RtpStream *
stream_new(enum RtpStreamType type, SipMsg *msg, PacketSdpMedia *media);

void
stream_free(RtpStream *stream);

void
stream_set_src(RtpStream *stream, Address src);

void
stream_set_dst(RtpStream *stream, Address dst);

void
stream_set_data(RtpStream *stream, Address src, Address dst);

void
stream_set_format(RtpStream *stream, guint8 format);

void
stream_add_packet(RtpStream *stream, Packet *packet);

guint
stream_get_count(RtpStream *stream);

const char *
stream_get_format(RtpStream *stream);

GTimeVal
stream_time(RtpStream *stream);

/**
 * @brief Determine if a stream is still active
 *
 * This simply checks the timestamp of the last received packet of the stream
 * marking as inactive if it was before STREAM_INACTIVE_SECS ago
 *
 * @return 1 if stream is active
 * @return 0 if stream is inactive
 */
gboolean
stream_is_active(RtpStream *stream);

#endif /* __SNGREP_STREAM_H */
