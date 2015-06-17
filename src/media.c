/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2015 Irontec SL. All rights reserved.
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
 * @file media.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in media.h
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "media.h"

/**
 * @brief Known RTP encodings
 */
rtp_encoding_t encodings[] = {
      { 0,  "PCMU/8000",    "g711u" },
      { 3,  "GSM/8000",     "gsm" },
      { 4,  "G723/8000",    "g723" },
      { 5,  "DVI4/8000",    "dvi" },
      { 6,  "DVI4/16000",   "dvi" },
      { 7,  "LPC/8000",     "lpc" },
      { 8,  "PCMA/8000",    "g711a" },
      { 9,  "G722/8000",    "g722" },
      { 10, "L16/44100",    "l16" },
      { 11, "L16/44100",    "l16" },
      { 12, "QCELP/8000",   "qcelp" },
      { 13, "CN/8000",      "cn" },
      { 14, "MPA/90000",    "mpa" },
      { 15, "G728/8000",    "g728" },
      { 16, "DVI4/11025",   "dvi" },
      { 17, "DVI4/22050",   "dvi" },
      { 18, "G729/8000",    "g729" },
      { 25, "CelB/90000",   "celb" },
      { 26, "JPEG/90000",   "jpeg" },
      { 28, "nv/90000",     "nv" },
      { 31, "H261/90000",   "h261" },
      { 32, "MPV/90000",    "mpv" },
      { 33, "MP2T/90000",   "mp2t" },
      { 34, "H263/90000",   "h263" },
      { -1, NULL }
};

sdp_media_t *
media_create(struct sip_msg *msg)
{
    sdp_media_t *media;;

    // Allocate memory for this media structure
    if (!(media = malloc(sizeof(sdp_media_t))))
        return NULL;

    // Initialize all fields
    memset(media, 0, sizeof(sdp_media_t));
    media->msg = msg;
    return media;
}


rtp_stream_t *
stream_create(sdp_media_t *media)
{
    rtp_stream_t *stream;;

    // Allocate memory for this stream structure
    if (!(stream = malloc(sizeof(rtp_stream_t))))
        return NULL;

    // Initialize all fields
    memset(stream, 0, sizeof(rtp_stream_t));
    stream->media = media;
    return stream;
}

void
media_set_port(sdp_media_t *media, u_short port)
{
    media->port = port;
}

void
media_set_type(sdp_media_t *media, const char *type)
{
    strcpy(media->type, type);
}

void
media_set_address(sdp_media_t *media, const char *address)
{
    strcpy(media->address, address);
}

void
media_set_format(sdp_media_t *media, const char *format)
{
    strcpy(media->format, format);
}

void
media_set_format_code(sdp_media_t *media, int code)
{
    media->fmtcode = code;
}

void
stream_add_packet(rtp_stream_t *stream, const char *ip_src, u_short sport, const char *ip_dst, u_short dport, int format, struct timeval time)
{
    if (stream->pktcnt) {
        stream->pktcnt++;
        return;
    }

    stream->format = format;
    stream->time = time;
    stream->pktcnt++;

}

const char *
media_get_address(sdp_media_t *media)
{
    return media->address;
}

u_short
media_get_port(sdp_media_t *media)
{
    return media->port;
}

const char *
media_get_type(sdp_media_t *media)
{
    return media->type;
}

const char *
media_get_format(sdp_media_t *media)
{
    return media_codec_from_encoding(media->fmtcode, media->format);
}

int
media_get_format_code(sdp_media_t *media)
{
    return media->fmtcode;
}

int
stream_get_count(rtp_stream_t *stream)
{
    return stream->pktcnt;
}

const char *
media_codec_from_encoding(int code, const char *format)
{
    int i;

    // Format from RTP codec id
    for (i = 0; encodings[i].id >= 0; i++) {
        if (encodings[i].id == code)
            return encodings[i].format;
    }

    if (format && strlen(format)) {
        // Format from RTP codec name
        for (i = 0; encodings[i].id >= 0; i++) {
            if (!strcmp(encodings[i].name, format))
                return encodings[i].format;
        }
    }

    return format;
}

