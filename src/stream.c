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
 * @file rtp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage RTP captured packets
 *
 * This file contains the functions and structures to manage the RTP streams
 *
 */
#include "config.h"
#include <glib.h>
#include "glib-extra.h"
#include "stream.h"
#include "storage.h"

RtpStream *
stream_new(enum RtpStreamType type, Message *msg, PacketSdpMedia *media)
{
    RtpStream *stream = g_malloc0(sizeof(RtpStream));

    // Initialize all fields
    stream->type = type;
    stream->msg = msg;
    stream->media = media;
    stream->packets = g_ptr_array_new_with_free_func((GDestroyNotify) packet_free);
    return stream;
}

void
stream_free(RtpStream *stream)
{
    g_ptr_array_free(stream->packets, TRUE);
    g_free(stream);
}

void
stream_set_src(RtpStream *stream, Address src)
{
    stream->src = src;
}

void
stream_set_dst(RtpStream *stream, Address dst)
{
    stream->dst = dst;
}

void
stream_set_data(RtpStream *stream, Address src, Address dst)
{
    stream->src = src;
    stream->dst = dst;
}

void
stream_set_format(RtpStream *stream, guint8 format)
{
    stream->fmtcode = format;
}

void
stream_add_packet(RtpStream *stream, Packet *packet)
{
    stream->lasttm = g_get_monotonic_time();
    g_ptr_array_add(stream->packets, packet);
}

guint
stream_get_count(RtpStream *stream)
{
    return stream->packets->len;
}

const char *
stream_get_format(RtpStream *stream)
{
    // Get format for media payload
    if (!stream || !stream->media)
        return NULL;

    // Try to get standard format form code
    PacketRtpEncoding *encoding = packet_rtp_standard_codec(stream->fmtcode);
    if (encoding != NULL)
        return encoding->format;

    // Try to get format form SDP payload
    for (guint i = 0; i < g_list_length(stream->media->formats); i++) {
        PacketSdpFormat *format = g_list_nth_data(stream->media->formats, i);
        if (format->id == stream->fmtcode) {
            return format->alias;
        }
    }

    // Not found format for this code
    return NULL;
}

GTimeVal
stream_time(RtpStream *stream)
{
    return packet_time(g_ptr_array_index(stream->packets, 0));
}

gboolean
stream_is_active(RtpStream *stream)
{
    return g_get_monotonic_time() - stream->lasttm <= STREAM_INACTIVE_SECS;
}
