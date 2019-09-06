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
 * @file stream.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage rtp streams
 */

#ifndef __SNGREP_STREAM_H
#define __SNGREP_STREAM_H

#include "config.h"
#include "storage/message.h"
#include "capture/capture_pcap.h"
#include "capture/dissectors/packet_sdp.h"
#include "capture/dissectors/packet_rtp.h"
#include "capture/dissectors/packet_rtcp.h"

//! Shorter declaration of rtp_stream structure
typedef struct _Stream Stream;

enum StreamType
{
    STREAM_RTP = 0,
    STREAM_RTCP
};

struct _Stream
{
    //! Determine stream type
    enum StreamType type;
    //! Source address
    Address *src;
    //! Destination address
    Address *dst;
    //! SDP media that setup this stream
    PacketSdpMedia *media;
    //! SIP message that setup this stream
    Message *msg;
    //! First received packet time
    GTimeVal firsttv;
    //! Last time this stream was updated
    gint64 lasttm;
    //! Changed since last checked flag
    gboolean changed;
    //! Format of first received packet of stre
    guint8 fmtcode;
    //! Stream packets (not always stored in packets array)
    guint pkt_count;
    //! List of stream packets
    GPtrArray *packets;
};

Stream *
stream_new(enum StreamType type, Message *msg, PacketSdpMedia *media);

void
stream_free(Stream *stream);

void
stream_set_src(Stream *stream, const Address *src);

void
stream_set_dst(Stream *stream, const Address *dst);

void
stream_set_data(Stream *stream, const Address *src, const Address *dst);

void
stream_set_format(Stream *stream, guint8 format);

void
stream_add_packet(Stream *stream, Packet *packet);

guint
stream_get_count(Stream *stream);

const char *
stream_get_format(Stream *stream);

GTimeVal
stream_time(Stream *stream);

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
stream_is_active(Stream *stream);

#endif /* __SNGREP_STREAM_H */
