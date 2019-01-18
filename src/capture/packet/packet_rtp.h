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
 * @file packet_rtp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage RTP packets
 *
 * @note RTP_VERSION and RTP_PAYLOAD_TYPE macros has been taken from wireshark
 *       source code: packet-rtp.c
 */

#ifndef __SNGREP_PACKET_RTP_H
#define __SNGREP_PACKET_RTP_H

#include <glib.h>

// Version is the first 2 bits of the first octet
#define RTP_VERSION(octet) ((octet) >> 6)
// Payload type is the last 7 bits
#define RTP_PAYLOAD_TYPE(octet) (guint8)((octet) & 0x7F)

// Handled RTP versions
#define RTP_VERSION_RFC1889 2

// RTP header length
#define RTP_HDR_LENGTH 12

// RTCP common header length
#define RTCP_HDR_LENGTH 4

// If stream does not receive a packet in this seconds, we consider it inactive
#define STREAM_INACTIVE_SECS 3

#define RTP_CODEC_G711A 8
#define RTP_CODEC_G729  18


typedef struct _PacketRtpData PacketRtpData;
typedef struct _PacketRtpEncoding PacketRtpEncoding;

struct _PacketRtpData
{
    PacketRtpEncoding *encoding;
    GByteArray *payload;
};

struct _PacketRtpEncoding
{
    guint8 id;
    const gchar *name;
    const gchar *format;
};

PacketRtpEncoding *
packet_rtp_standard_codec(guint8 code);

PacketDissector *
packet_rtp_new();

#endif /* __SNGREP_PACKET_RTP_H */
