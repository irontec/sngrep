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
 * @file stream.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage RTP captured packets
 *
 * This file contains the functions and structures to manage the RTP streams
 *
 */
#include "config.h"
#include <glib.h>
#include "stream.h"
#include "storage/storage.h"

Stream *
stream_new(enum StreamType type, Message *msg, PacketSdpMedia *media)
{
    Stream *stream = g_malloc0(sizeof(Stream));

    // Initialize all fields
    stream->type = type;
    stream->msg = msg;
    stream->media = media;
    stream->packets = g_ptr_array_new_with_free_func((GDestroyNotify) packet_unref);
    return stream;
}

void
stream_free(Stream *stream)
{
    g_date_time_unref(stream->firsttv);
    g_ptr_array_free(stream->packets, TRUE);
    address_free(stream->src);
    address_free(stream->dst);
    g_free(stream);
}

void
stream_set_src(Stream *stream, const Address *src)
{
    g_return_if_fail(stream != NULL);
    g_return_if_fail(src != NULL);

    stream->src = address_clone(src);
}

void
stream_set_dst(Stream *stream, const Address *dst)
{
    g_return_if_fail(stream != NULL);
    g_return_if_fail(dst != NULL);

    stream->dst = address_clone(dst);
}

void
stream_set_data(Stream *stream, const Address *src, const Address *dst)
{
    stream_set_src(stream, src);
    stream_set_dst(stream, dst);
}

void
stream_set_format(Stream *stream, guint8 format)
{
    stream->fmtcode = format;
}

void
stream_add_packet(Stream *stream, Packet *packet)
{
    stream->lasttm = g_get_monotonic_time();
    stream->changed = TRUE;
    stream->pkt_count++;
    if (stream->firsttv == NULL) {
        stream->firsttv = g_date_time_ref(packet_time(packet));
    }
}

guint
stream_get_count(Stream *stream)
{
    return stream->pkt_count;
}

const char *
stream_get_format(Stream *stream)
{
    // Get format for media payload
    g_return_val_if_fail(stream != NULL, NULL);

    // Try to get standard format form code
    PacketRtpEncoding *encoding = packet_rtp_standard_codec(stream->fmtcode);
    if (encoding != NULL)
        return g_strdup(encoding->format);

    // Try to get format form SDP payload
    g_return_val_if_fail(stream->media != NULL, NULL);
    for (guint i = 0; i < g_list_length(stream->media->formats); i++) {
        PacketSdpFormat *format = g_list_nth_data(stream->media->formats, i);
        if (format->id == stream->fmtcode) {
            return g_strdup(format->alias);
        }
    }

    // Not found format for this code
    return g_strdup_printf("unknown-%d", stream->fmtcode);
}

GDateTime *
stream_time(Stream *stream)
{
    return stream->firsttv;
}

gboolean
stream_is_active(Stream *stream)
{
    return g_get_monotonic_time() - stream->lasttm <= STREAM_INACTIVE_SECS;
}
