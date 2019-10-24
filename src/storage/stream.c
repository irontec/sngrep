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
#include "glib/glib-extra.h"
#include "stream.h"
#include "storage/storage.h"

Stream *
stream_new(StreamType type, Message *msg, PacketSdpMedia *media)
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
    if (stream->firsttv != NULL) {
        g_date_time_unref(stream->firsttv);
    }

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
stream_set_ssrc(Stream *stream, guint32 ssrc)
{
    stream->ssrc = ssrc;
}

static void
stream_rtp_analyze(Stream *stream, Packet *packet)
{
    PacketRtpEncoding *encoding = packet_rtp_standard_codec(stream->fmtcode);
    if (encoding == NULL) {
        // Non standard codec, impossible to analyze
        return;
    }

    PacketRtpData *rtp = packet_rtp_data(packet);

    // Packet capture timestamp in ms
    gdouble pkt_time = date_time_to_unix_ms(packet_time(packet));

    // Store first packet information for later comparison
    if (stream->packet_count == 1) {
        stream->stats.pkt_time = pkt_time;
        stream->stats.ts = rtp->ts;
        stream->stats.seq_num = rtp->seq;
        stream->stats.first_seq_num = rtp->seq;
        return;
    }

    // current packet has correct sequence number
    if (stream->stats.seq_num + 1 == rtp->seq) {
        stream->stats.seq_num = rtp->seq;
        // current packet wraps rtp sequence number
    } else if (stream->stats.seq_num == 65535 && rtp->seq == 0) {
        stream->stats.seq_num = 0;
        stream->stats.cycled += 65536 - stream->stats.first_seq_num;
        stream->stats.first_seq_num = 0;
        // current packet is lower in sequence by a big amount, assume new cycle
    } else if (stream->stats.seq_num - rtp->seq > 0x00F0) {
        stream->stats.seq_num = rtp->seq;
        stream->stats.cycled += 65536 - stream->stats.first_seq_num;
        stream->stats.first_seq_num = 0;
        // current packet is greater than sequence number: we've lost some packets
    } else if (stream->stats.seq_num + 1 < rtp->seq) {
        stream->stats.oos++;
        stream->stats.seq_num = rtp->seq;
        // current packet is from the past in the sequence: duplicate or late
    } else if (stream->stats.seq_num + 1 > rtp->seq) {
        stream->stats.oos++;
        return;
    }

    // Check delta time from the previous message
    gdouble delta = pkt_time - stream->stats.pkt_time;
    if (delta > stream->stats.max_delta) {
        stream->stats.max_delta = delta;
    }

    // Calculate jitter buffer in ms
    // Formulas from wireshark wiki https://wiki.wireshark.org/RTP_statistics  based on RFC3350
    //! D(i,j) = (Rj - Ri) - (Sj - Si) = (Rj - Sj) - (Ri - Si)
    gdouble sample_rate = ((gdouble) 1 / encoding->clock) * G_MSEC_PER_SEC;
    gdouble rj = pkt_time;
    gdouble ri = stream->stats.pkt_time;
    gdouble sj = ((gdouble) rtp->ts) * sample_rate;
    gdouble si = ((gdouble) stream->stats.ts) * sample_rate;
    gdouble dij = (rj - ri) - (sj - si);
    //! J(i) = J(i-1) + (|D(i-1,i)| - J(i-1))/16
    gdouble jitter = stream->stats.jitter + (ABS(dij) - stream->stats.jitter) / 16;

    // Check if current packet increases max jitter value
    if (jitter > stream->stats.max_jitter) {
        stream->stats.max_jitter = jitter;
    }

    // Calculate mean jitter
    stream->stats.mean_jitter =
        (stream->stats.mean_jitter * stream->packet_count + jitter) /
        (stream->packet_count + 1);

    // Update stream stats for next parsed packet
    stream->stats.pkt_time = pkt_time;
    stream->stats.ts = rtp->ts;
    stream->stats.jitter = jitter;
    stream->stats.expected = stream->stats.cycled + (rtp->seq - stream->stats.first_seq_num + 1);
    stream->stats.lost = stream->stats.expected - stream->packet_count;
}

void
stream_add_packet(Stream *stream, Packet *packet)
{
    stream->lasttm = g_get_monotonic_time();
    stream->changed = TRUE;
    stream->packet_count++;
    if (stream->firsttv == NULL) {
        stream->firsttv = g_date_time_new_from_unix_usec(packet_time(packet));
    }

    // Add received packet to stream stats
    stream_rtp_analyze(stream, packet);
}

guint
stream_get_count(Stream *stream)
{
    return stream->packet_count;
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
