/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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

#include "rtp.h"
#include "sip.h"
#include "vector.h"

/**
 * @brief Known RTP encodings
 */
rtp_encoding_t encodings[] = {
    { 0, "PCMU/8000", "g711u" },
    { 3, "GSM/8000", "gsm" },
    { 4, "G723/8000", "g723" },
    { 5, "DVI4/8000", "dvi" },
    { 6, "DVI4/16000", "dvi" },
    { 7, "LPC/8000", "lpc" },
    { 8, "PCMA/8000", "g711a" },
    { 9, "G722/8000", "g722" },
    { 10, "L16/44100", "l16" },
    { 11, "L16/44100", "l16" },
    { 12, "QCELP/8000", "qcelp" },
    { 13, "CN/8000", "cn" },
    { 14, "MPA/90000", "mpa" },
    { 15, "G728/8000", "g728" },
    { 16, "DVI4/11025", "dvi" },
    { 17, "DVI4/22050", "dvi" },
    { 18, "G729/8000", "g729" },
    { 25, "CelB/90000", "celb" },
    { 26, "JPEG/90000", "jpeg" },
    { 28, "nv/90000", "nv" },
    { 31, "H261/90000", "h261" },
    { 32, "MPV/90000", "mpv" },
    { 33, "MP2T/90000", "mp2t" },
    { 34, "H263/90000", "h263" },
    { 0, NULL, NULL }
};

rtp_stream_t *
stream_create(sdp_media_t *media, const char *dst, u_short dport)
{
    rtp_stream_t *stream;

    // Allocate memory for this stream structure
    if (!(stream = sng_malloc(sizeof(rtp_stream_t))))
        return NULL;

    // Initialize all fields
    stream->media = media;
    strcpy(stream->ip_dst, dst);
    stream->dport = dport;

    return stream;
}

rtp_stream_t *
stream_complete(rtp_stream_t *stream, const char *src, u_short sport, u_int format)
{
    strcpy(stream->ip_src, src);
    stream->sport = sport;
    stream->fmtcode = format;
    return stream;
}

void
stream_add_packet(rtp_stream_t *stream, const struct pcap_pkthdr *header)
{
    if (stream->pktcnt == 0)
        stream->time = header->ts;

    stream->pktcnt++;
}

int
stream_get_count(rtp_stream_t *stream)
{
    return stream->pktcnt;
}

struct sip_call *
stream_get_call(rtp_stream_t *stream) {
    if (stream && stream->media && stream->media->msg)
        return stream->media->msg->call;
    return NULL;
}

const char *
stream_get_format(rtp_stream_t *stream)
{

    const char *fmt;

    // Get format for media payload
    if (!stream || !stream->media)
        return NULL;

    // Try to get standard format form code
    if ((fmt = rtp_get_standard_format(stream->fmtcode)))
        return fmt;

    // Try to get format form SDP payload
    if ((fmt = media_get_format(stream->media, stream->fmtcode)))
        return fmt;

    // Not found format for this code
    return NULL;
}

const char *
rtp_get_standard_format(u_int code)
{
    int i;

    // Format from RTP codec id
    for (i = 0; encodings[i].format; i++) {
        if (encodings[i].id == code)
            return encodings[i].format;
    }

    return NULL;
}

rtp_stream_t *
rtp_check_stream(capture_packet_t *packet, const char *src, u_short sport, const char* dst, u_short dport)
{
    // Media structure for RTP packets
    rtp_stream_t *stream;
    rtp_stream_t *reverse;
    // RTP payload data
    u_int format;
    u_char *payload = capture_packet_get_payload(packet);
    uint32_t size = capture_packet_get_payload_len(packet);

    // Check if we have at least RTP type
    if (size < 2)
        return NULL;

    // Check RTP version
    if (RTP_VERSION(*payload) != RTP_VERSION_RFC1889)
        return NULL;

    // Get RTP payload type
    format = RTP_PAYLOAD_TYPE(*(++payload));

    // Find the matching stream
    stream = rtp_find_stream(src, sport, dst, dport, format);

    // A valid stream has been found
    if (stream) {
        // We have found a stream, but with different format
        if (stream->pktcnt && stream->fmtcode != format) {
            // Create a new stream for this new format
            stream = stream_create(stream->media, dst, dport);
            stream_complete(stream, src, sport, format);
            call_add_stream(msg_get_call(stream->media->msg), stream);
        }

        // First packet for this stream, set source data
        if (stream->pktcnt == 0) {
            stream_complete(stream, src, sport, format);
            // Check if an stream in the opposite direction exists
            if (!(reverse = rtp_find_call_stream(stream->media->msg->call, stream->ip_dst, stream->dport,  stream->ip_src,  stream->sport, stream->fmtcode))) {
                reverse = stream_create(stream->media, stream->ip_src, stream->sport);
                stream_complete(reverse, stream->ip_dst, stream->dport, format);
                call_add_stream(msg_get_call(stream->media->msg), reverse);
            }
        }

        // Add packet to stream
        // TODO Multiframe packet support
        stream_add_packet(stream, ((capture_frame_t *)vector_first(packet->frames))->header);
    }

    return stream;
}

rtp_stream_t *
rtp_find_stream(const char *src, u_short sport, const char *dst, u_short dport, u_int format)
{
    // Structure for RTP packet streams
    rtp_stream_t *stream;
    // Check if this is a RTP packet from active calls
    sip_call_t *call;
    // Iterator for active calls
    vector_iter_t calls;

    // Get active calls (during conversation)
    calls = sip_calls_iterator();
    vector_iterator_set_filter(&calls, call_is_active);

    while ((call = vector_iterator_next(&calls))) {
        // Check if this call has an RTP stream for current packet data
        if ((stream = rtp_find_call_stream(call, src, sport, dst, dport, format))) {
            return stream;
        }
    }

    return NULL;
}

rtp_stream_t *
rtp_find_call_stream(struct sip_call *call, const char *ip_src, u_short sport, const char *ip_dst, u_short dport, u_int format)
{
    rtp_stream_t *stream, *ret = NULL;
    vector_iter_t it;

    it = vector_iterator(call->streams);

    // Try to look for a complete stream with this format
    while ((stream = vector_iterator_next(&it))) {
        if (!strcmp(ip_src, stream->ip_src) && sport == stream->sport &&
            !strcmp(ip_dst, stream->ip_dst) && dport == stream->dport &&
            stream->fmtcode == format && stream->pktcnt) {
            ret = stream;
        }
    }

    // Try to look for a complete stream with any format
    if (!ret) {
        vector_iterator_reset(&it);
        while ((stream = vector_iterator_next(&it))) {
            if (!strcmp(ip_src, stream->ip_src) && sport == stream->sport &&
                !strcmp(ip_dst, stream->ip_dst) && dport == stream->dport) {
                ret = stream;
            }
        }
    }

    // Try to look for an incomplete stream with this destination
    if (!ret) {
        vector_iterator_reset(&it);
        while ((stream = vector_iterator_next(&it))) {
            if (!strcmp(ip_dst, stream->ip_dst) && dport == stream->dport && !stream->pktcnt) {
                ret = stream;
            }
        }
    }

    return ret;
}
