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
 * @file packet_rtp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_rtp.h
 */

#include "config.h"
#include <glib.h>
#include "glib-extra.h"
#include "storage.h"
#include "capture/parser.h"
#include "capture/packet.h"
#include "packet_ip.h"
#include "packet_udp.h"
#include "packet_rtp.h"

/**
 * @brief Known RTP encodings
 */
PacketRtpEncoding encodings[] = {
    { 0,  "PCMU/8000",  "g711u" },
    { 3,  "GSM/8000",   "gsm" },
    { 4,  "G723/8000",  "g723" },
    { 5,  "DVI4/8000",  "dvi" },
    { 6,  "DVI4/16000", "dvi" },
    { 7,  "LPC/8000",   "lpc" },
    { 8,  "PCMA/8000",  "g711a" },
    { 9,  "G722/8000",  "g722" },
    { 10, "L16/44100",  "l16" },
    { 11, "L16/44100",  "l16" },
    { 12, "QCELP/8000", "qcelp" },
    { 13, "CN/8000",    "cn" },
    { 14, "MPA/90000",  "mpa" },
    { 15, "G728/8000",  "g728" },
    { 16, "DVI4/11025", "dvi" },
    { 17, "DVI4/22050", "dvi" },
    { 18, "G729/8000",  "g729" },
    { 25, "CelB/90000", "celb" },
    { 26, "JPEG/90000", "jpeg" },
    { 28, "nv/90000",   "nv" },
    { 31, "H261/90000", "h261" },
    { 32, "MPV/90000",  "mpv" },
    { 33, "MP2T/90000", "mp2t" },
    { 34, "H263/90000", "h263" },
    { 0, NULL, NULL }
};

PacketRtpEncoding *
packet_rtp_standard_codec(guint8 code)
{
    // Format from RTP codec id
    for (guint i = 0; encodings[i].format; i++) {
        if (encodings[i].id == code)
            return &encodings[i];
    }

    return NULL;
}

/**
 * @brief Check if the data is a RTP packet
 * RFC 5761 Section 4.  Distinguishable RTP and RTCP Packets
 * RFC 5764 Section 5.1.2.  Reception (packet demultiplexing)
 */
static gboolean
packet_rtp_valid(GByteArray *data)
{
    g_return_val_if_fail(data != NULL, FALSE);

    guint8 pt = RTP_PAYLOAD_TYPE(data->data[1]);

    if ((data->len >= RTP_HDR_LENGTH) &&
        (RTP_VERSION(data->data[0]) == RTP_VERSION_RFC1889) &&
        (data->data[0] > 127 && data->data[0] < 192) &&
        (pt <= 64 || pt >= 96)) {
        return TRUE;
    }

    // Not a RTP packet
    return FALSE;
}

static GByteArray *
packet_rtp_parse(G_GNUC_UNUSED PacketParser *parser, Packet *packet, GByteArray *data)
{
    // Not RTP
    if (!packet_rtp_valid(data)) {
        return data;
    }

    guint8 codec = RTP_PAYLOAD_TYPE(data->data[1]);

    PacketRtpData *rtp = g_malloc0(sizeof(PacketRtpData));
    rtp->encoding = packet_rtp_standard_codec(codec);

    // Not standard codec, just set the id and let storage search in SDP rtpmap
    if (rtp->encoding == NULL) {
        rtp->encoding = g_malloc0(sizeof(PacketRtpEncoding));
        rtp->encoding->id = codec;
    }

    // Store RTP payload data
    rtp->payload = g_byte_array_remove_range(data, 0, 12 /* RTP Headers */);

    // Set packet RTP informaiton
    g_ptr_array_set(packet->proto, PACKET_RTP, rtp);

    /** @TODO Backwards compatibility during refactoring */
    storage_add_packet(packet);
    return NULL;
}

static void
packet_rtp_free(G_GNUC_UNUSED PacketParser *parser, Packet *packet)
{
    g_return_if_fail(packet != NULL);
    PacketRtpData *rtp_data = g_ptr_array_index(packet->proto, PACKET_RTP);
    g_return_if_fail(rtp_data != NULL);

    if (packet_rtp_standard_codec(rtp_data->encoding->id) == NULL) {
        g_free(rtp_data->encoding);
    }

    g_byte_array_free(rtp_data->payload, TRUE);
    g_free(rtp_data);
}

PacketDissector *
packet_rtp_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_RTP;
    proto->dissect = packet_rtp_parse;
    proto->free = packet_rtp_free;
    return proto;
}
