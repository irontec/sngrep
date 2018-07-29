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
#include "glib-utils.h"
#include "stream.h"
#include "storage.h"

RtpStream *
stream_create(G_GNUC_UNUSED Packet *packet, PacketSdpMedia *media)
{
    RtpStream *stream = g_malloc0(sizeof(RtpStream));

    // Initialize all fields
    stream->type = media->type;
    stream->media = media;
    stream->dst = media->address;
    stream->packets = g_ptr_array_sized_new(500);
    return stream;
}

RtpStream *
stream_complete(RtpStream *stream, Address src)
{
    stream->src = src;
    return stream;
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

RtpStream *
stream_find_by_format(Address src, Address dst, uint32_t format)
{
    // Structure for RTP packet streams
    RtpStream *stream;
    // Check if this is a RTP packet from active calls
    SipCall *call;
    // Iterator for active calls
    GSequenceIter *calls;
    // Candiate stream
    RtpStream *candidate = NULL;

    // Get active calls (during conversation)
    calls = g_sequence_get_end_iter(storage_calls_vector());

    while (!g_sequence_iter_is_begin(calls)) {
        calls = g_sequence_iter_prev(calls);
        call = g_sequence_get(calls);
        for (guint i = 0; i < g_ptr_array_len(call->streams); i++) {
            stream = g_ptr_array_index(call->streams, i);
            // Only look RTP packets
            if (stream->type != PACKET_RTP)
                continue;

            // Stream complete, check source, dst
            if (stream_is_complete(stream)) {
                if (addressport_equals(stream->src, src) &&
                    addressport_equals(stream->dst, dst)) {
                    // Exact searched stream format
                    if (stream->fmtcode == format) {
                        return stream;
                    } else {
                        // Matching addresses but different format
                        candidate = stream;
                    }
                }
            } else {
                // Incomplete stream, if dst match is enough
                if (addressport_equals(stream->dst, dst)) {
                    return stream;
                }
            }
        }
    }

    return candidate;
}

RtpStream *
stream_find(Address src, Address dst)
{
    // Structure for RTP packet streams
    RtpStream *stream;
    // Check if this is a RTP packet from active calls
    SipCall *call;
    // Iterator for active calls
    GSequenceIter *calls;

    // Get active calls (during conversation)
    calls = g_sequence_get_end_iter(storage_calls_vector());

    while (!g_sequence_iter_is_begin(calls)) {
        calls = g_sequence_iter_prev(calls);
        call = g_sequence_get(calls);
        // Check if this call has an RTP stream for current packet data
        if ((stream = call_find_stream(call, src, dst))) {
            return stream;
        }
    }

    return NULL;
}

GTimeVal
stream_time(RtpStream *stream)
{
    return packet_time(g_ptr_array_index(stream->packets, 0));
}

gint
stream_is_older(RtpStream *one, RtpStream *two)
{
    return timeval_is_older(
            stream_time(one),
            stream_time(two)
    );
}

gboolean
stream_is_complete(RtpStream *stream)
{
    return stream->packets->len > 0;
}

gboolean
stream_is_active(RtpStream *stream)
{
    return g_get_monotonic_time() - stream->lasttm <= STREAM_INACTIVE_SECS;
}
