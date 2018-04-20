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
#include <stddef.h>
#include <time.h>
#include "glib-utils.h"
#include "stream.h"
#include "storage.h"

rtp_stream_t *
stream_create(Packet *packet, PacketSdpMedia *media)
{
    rtp_stream_t *stream;

    // Allocate memory for this stream structure
    if (!(stream = sng_malloc(sizeof(rtp_stream_t))))
        return NULL;

    // Initialize all fields
    stream->type = media->type;
    stream->media = media;
    stream->dst = media->address;

    return stream;
}

rtp_stream_t *
stream_complete(rtp_stream_t *stream, Address src)
{
    stream->src = src;
    return stream;
}

void
stream_set_format(rtp_stream_t *stream, uint32_t format)
{
    stream->fmtcode = format;
}

void
stream_add_packet(rtp_stream_t *stream, packet_t *packet)
{
    if (stream->pktcnt == 0)
        stream->time = packet_time(packet);

    stream->lasttm = (int) time(NULL);
    stream->pktcnt++;
}

uint32_t
stream_get_count(rtp_stream_t *stream)
{
    return stream->pktcnt;
}

const char *
stream_get_format(rtp_stream_t *stream)
{

    const char *fmt;

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

rtp_stream_t *
stream_find_by_format(Address src, Address dst, uint32_t format)
{
    // Structure for RTP packet streams
    rtp_stream_t *stream;
    // Check if this is a RTP packet from active calls
    sip_call_t *call;
    // Iterator for active calls
    GSequenceIter *calls;
    // Iterator for call streams
    GSequenceIter *streams;
    // Candiate stream
    rtp_stream_t *candidate = NULL;

    // Get active calls (during conversation)
    calls = g_sequence_get_end_iter(storage_calls_vector());

    while(!g_sequence_iter_is_begin(calls)) {
        calls = g_sequence_iter_prev(calls);
        call = g_sequence_get(calls);
        streams = g_sequence_get_end_iter(call->streams);
        while(!g_sequence_iter_is_begin(streams)) {
            streams = g_sequence_iter_prev(streams);
            stream = g_sequence_get(streams);
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

rtp_stream_t *
stream_find(Address src, Address dst)
{
    // Structure for RTP packet streams
    rtp_stream_t *stream;
    // Check if this is a RTP packet from active calls
    sip_call_t *call;
    // Iterator for active calls
    GSequenceIter *calls;

    // Get active calls (during conversation)
    calls = g_sequence_get_end_iter(storage_calls_vector());

    while(!g_sequence_iter_is_begin(calls)) {
        calls = g_sequence_iter_prev(calls);
        call = g_sequence_get(calls);
        // Check if this call has an RTP stream for current packet data
        if ((stream = call_find_stream(call, src, dst))) {
            return stream;
        }
    }

    return NULL;
}



int
stream_is_older(rtp_stream_t *one, rtp_stream_t *two)
{
    // Yes, you are older than nothing
    if (!two)
        return 1;

    // No, you are not older than yourself
    if (one == two)
        return 0;

    // Otherwise
    return timeval_is_older(one->time, two->time);
}

int
stream_is_complete(rtp_stream_t *stream)
{
    return (stream->pktcnt != 0);
}

int
stream_is_active(rtp_stream_t *stream)
{
    return ((int) time(NULL) - stream->lasttm <= STREAM_INACTIVE_SECS);
}
