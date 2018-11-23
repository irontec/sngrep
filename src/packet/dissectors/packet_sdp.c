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
 * @file packet_sdp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Funtions to manage SDP protocol
 */
#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include "glib-utils.h"
#include "packet_sdp.h"

/**
 * @brief Known RTP encodings
 *
 * This values has been interpreted from:
 * https://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml
 * https://tools.ietf.org/html/rfc3551#section-6
 *
 * Alias names for each RTP encoding name are sngrep developers personal
 * preferrence and may or may not match reality.
 *
 */
PacketSdpFormat formats[] = {
    { 0,    "PCMU/8000",    "g711u"     },
    { 3,    "GSM/8000",     "gsm"       },
    { 4,    "G723/8000",    "g723"      },
    { 5,    "DVI4/8000",    "dvi"       },
    { 6,    "DVI4/16000",   "dvi"       },
    { 7,    "LPC/8000",     "lpc"       },
    { 8,    "PCMA/8000",    "g711a"     },
    { 9,    "G722/8000",    "g722"      },
    { 10,   "L16/44100",    "l16"       },
    { 11,   "L16/44100",    "l16"       },
    { 12,   "QCELP/8000",   "qcelp"     },
    { 13,   "CN/8000",      "cn"        },
    { 14,   "MPA/90000",    "mpa"       },
    { 15,   "G728/8000",    "g728"      },
    { 16,   "DVI4/11025",   "dvi"       },
    { 17,   "DVI4/22050",   "dvi"       },
    { 18,   "G729/8000",    "g729"      },
    { 25,   "CelB/90000",   "celb"      },
    { 26,   "JPEG/90000",   "jpeg"      },
    { 28,   "nv/90000",     "nv"        },
    { 31,   "H261/90000",   "h261"      },
    { 32,   "MPV/90000",    "mpv"       },
    { 33,   "MP2T/90000",   "mp2t"      },
    { 34,   "H263/90000",   "h263"      },
};

const struct {
    const gchar *str;
    enum PacketSdpMediaType type;
} media_types[] = {
    { "audio",          SDP_MEDIA_AUDIO         },
    { "video",          SDP_MEDIA_VIDEO         },
    { "text",           SDP_MEDIA_TEXT          },
    { "application",    SDP_MEDIA_APPLICATION   },
    { "message",        SDP_MEDIA_MESSAGE       },
    { "image",          SDP_MEDIA_IMAGE         }
};

const gchar*
packet_sdp_media_type_str(enum PacketSdpMediaType type)
{
    for (guint i = 0; i < G_N_ELEMENTS(media_types); i++) {
        if (media_types[i].type == type) {
            return media_types[i].str;
        }
    }

    return NULL;
}

static void
packet_sdp_dissect_connection(PacketSdpData *sdp, PacketSdpMedia *media, gchar *line)
{
    // c=<nettype> <addrtype> <connection-address>
    gchar **conn_data = g_strsplit(line, " ", 3);
    if (g_strv_length(conn_data) < 3) return;

    // Get connection address
    PacketSdpConnection *conn = g_malloc(sizeof(PacketSdpConnection));
    g_utf8_strncpy(conn->address, conn_data[SDP_CONN_ADDRESS], ADDRESSLEN);

    // Set media/session connection data
    if (media == NULL) {
        sdp->sconn = conn;
    } else {
        media->sconn = conn;
        g_utf8_strncpy(media->address.ip, conn->address, ADDRESSLEN);
    }
}

static enum PacketSdpMediaType
packet_sdp_media_type(const gchar *media)
{
    g_return_val_if_fail(media != NULL, 0);

    for (guint i = 0; i < G_N_ELEMENTS(media_types); i++) {
        if (g_ascii_strcasecmp(media, media_types[i].str) == 0) {
            return media_types[i].type;
        }
    }

    return SDP_MEDIA_UNKNOWN;
}

static PacketSdpFormat *
packet_sdp_standard_format(guint32 code)
{
    for (guint i = 0;i < G_N_ELEMENTS(formats); i++) {
        if (formats[i].id == code) {
            return  &formats[i];
        }
    }

    return NULL;
}

static PacketSdpMedia *
packet_sdp_dissect_media(PacketSdpData *sdp, gchar *line)
{
    // m=<media> <port> <proto> <fmt>
    gchar **media_data = g_strsplit(line, " ", 4);

    // Create a new media container
    PacketSdpMedia *media = g_malloc0(sizeof(PacketSdpMedia));
    media->sconn = sdp->sconn;
    media->rtpport = (guint16)strtoul(media_data[SDP_MEDIA_PORT], NULL, 10);
    media->type = packet_sdp_media_type(media_data[SDP_MEDIA_MEDIA]);

    // @todo For backwards compatibility with stream searching
    if (sdp->sconn) {
        g_utf8_strncpy(media->address.ip, sdp->sconn->address, ADDRESSLEN);
        media->address.port = media->rtpport;
    }

    // Parse SDP preferred codec order
    gchar **formats = g_strsplit(media_data[SDP_MEDIA_FORMAT], " ", -1);
    for (guint i = 0; i < g_strv_length(formats); i++ ) {
        // Check if format code is standard
        guint32 code = (guint32)strtoul(formats[i], NULL, 10);

        PacketSdpFormat *format = packet_sdp_standard_format(code);
        if (format == NULL) {
            // Unknown media format for this code
            format = g_malloc0(sizeof(PacketSdpFormat));
            format->id = code;
        }

        // Add new format for this media
        media->formats = g_list_append(media->formats, format);
    }

    return media;
}

static void
packet_sdp_dissect_attribute(G_GNUC_UNUSED PacketSdpData *sdp, PacketSdpMedia *media, gchar *line)
{
    // a=<attribute>
    // a=<attribute>:<value>
    gchar **rtpattr = g_strsplit_set(line, " :", -1);

    if (g_ascii_strcasecmp(rtpattr[SDP_ATTR_NAME], "rtpmap") == 0) {
        // Check if format code is standard
        guint32 code = (guint32)strtoul(rtpattr[2], NULL, 10);
        PacketSdpFormat *format = packet_sdp_standard_format(code);

        if (format == NULL) {
            for (guint i = 0; i < g_list_length(media->formats); i++) {
                format = g_list_nth_data(media->formats, i);
                if (format->id == code) {
                    format->name = format->alias = rtpattr[3];
                    break;
                }
            }
        }
    } else if (g_ascii_strcasecmp(rtpattr[SDP_ATTR_NAME], "rtcp") == 0) {
        media->rtcpport = (guint16)strtoul(rtpattr[SDP_ATTR_VALUE], NULL, 10);
    }
}

static GByteArray *
packet_sdp_dissect(G_GNUC_UNUSED PacketParser *parser, Packet *packet, GByteArray *data)
{
    GString *payload = g_string_new_len((const gchar *) data->data, data->len);

    PacketSdpData *sdp = g_malloc0(sizeof(PacketSdpData));
    PacketSdpMedia *media = NULL;

    gchar **lines = g_strsplit(payload->str, "\r\n", -1);
    for (guint i = 0; i < g_strv_length(lines); i++ ) {
        gchar *line = lines[i] + 2;
        switch (lines[i][0]) {
            case 'c':
                packet_sdp_dissect_connection(sdp, media, line);
                break;
            case 'm':
                media = packet_sdp_dissect_media(sdp, line);
                sdp->medias = g_list_append(sdp->medias, media);
                break;
            case 'a':
                packet_sdp_dissect_attribute(sdp, media, line);
            default:
                break;
        }
    }

    // Set packet SDP data
    g_ptr_array_set(packet->proto, PACKET_SDP, sdp);

    return data;
}

PacketDissector *
packet_sdp_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_SDP;
    proto->dissect = packet_sdp_dissect;
    return proto;
}

