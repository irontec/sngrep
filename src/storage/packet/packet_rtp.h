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
 * @file packet_rtp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage RTP packets
 *
 * @note RTP_VERSION and RTP_PAYLOAD_TYPE macros has been taken from wireshark
 *       source code: packet-rtp.c
 */

#ifndef __SNGREP_PACKET_RTP_H__
#define __SNGREP_PACKET_RTP_H__

#include <glib.h>

G_BEGIN_DECLS

#define PACKET_DISSECTOR_TYPE_RTP packet_dissector_rtp_get_type()
G_DECLARE_FINAL_TYPE(PacketDissectorRtp, packet_dissector_rtp, PACKET_DISSECTOR, RTP, PacketDissector)

/**
 * From wireshark code rtp_pt.h file
 * RTP Payload types
 * Table B.2 / H.225.0
 * Also RFC 3551, and
 *
 *  http://www.iana.org/assignments/rtp-parameters
 */
#define RTP_PT_PCMU           0
#define RTP_PT_GSM            3
#define RTP_PT_G723           4
#define RTP_PT_DVI4_8000      5
#define RTP_PT_DVI4_16000     6
#define RTP_PT_LPC            7
#define RTP_PT_PCMA           8
#define RTP_PT_G722           9
#define RTP_PT_L16_STEREO    10
#define RTP_PT_L16_MONO      11
#define RTP_PT_QCELP         12
#define RTP_PT_CN            13
#define RTP_PT_MPA           14
#define RTP_PT_G728          15
#define RTP_PT_DVI4_11025    16
#define RTP_PT_DVI4_22050    17
#define RTP_PT_G729          18
#define RTP_PT_CELB          25
#define RTP_PT_JPEG          26
#define RTP_PT_NV            28
#define RTP_PT_H261          31
#define RTP_PT_MPV           32
#define RTP_PT_MP2T          33
#define RTP_PT_H263          34

typedef struct _PacketRtpHdr PacketRtpHdr;
typedef struct _PacketRtpData PacketRtpData;
typedef struct _PacketRtpEncoding PacketRtpEncoding;

struct _PacketDissectorRtp
{
    //! Parent structure
    PacketDissector parent;
};

struct _PacketRtpHdr
{
# if __BYTE_ORDER == __LITTLE_ENDIAN
    guint8 cc:4;
    guint8 ext:1;
    guint8 pad:1;
    guint8 version:2;
    guint8 pt:7;
    guint8 marker:1;
# endif
# if __BYTE_ORDER == __BIG_ENDIAN
    guint8 version:2;
    guint8 pad:1;
    guint8 ext:1;
    guint8 cc:4;
    guint8 marker:1;
    guint8 pt:7;
# endif
    guint16 seq;
    guint32 ts;
    guint32 ssrc;
} __attribute__((packed));

struct _PacketRtpData
{
    //! Protocol information
    PacketProtocol proto;
    //! RTP encoding (@see encodings table)
    PacketRtpEncoding *encoding;
    //! RTP Sequence number
    guint16 seq;
    //! RTP timestamp
    guint32 ts;
    //! RTP Syncronization Source Iden
    guint32 ssrc;
    //! RTP Marker set
    gboolean marker;
    //! RTP payload
    GBytes *payload;
};

struct _PacketRtpEncoding
{
    guint8 id;
    const gchar *name;
    const gchar *format;
    gint clock;
};

PacketRtpEncoding *
packet_rtp_standard_codec(guint8 code);

PacketRtpData *
packet_rtp_data(const Packet *packet);

PacketDissector *
packet_dissector_rtp_new();

G_END_DECLS

#endif  /* __SNGREP_PACKET_RTP_H__ */
