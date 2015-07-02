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
rtp_encoding_t encodings[] =
{
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
  { -1, NULL } };

rtp_stream_t *
stream_create(sdp_media_t *media, const char *dst, u_short dport)
{
    rtp_stream_t *stream;

    // Allocate memory for this stream structure
    if (!(stream = malloc(sizeof(rtp_stream_t))))
        return NULL;

    // Initialize all fields
    memset(stream, 0, sizeof(rtp_stream_t));
    stream->media = media;
    strcpy(stream->ip_dst, dst);
    stream->dport = dport;

    return stream;
}

rtp_stream_t *
stream_complete(rtp_stream_t *stream, const char *src, u_short sport)
{
    strcpy(stream->ip_src, src);
    stream->sport = sport;
    stream->complete = 1;
    return stream;
}

void
stream_add_packet(rtp_stream_t *stream, const char *ip_src, u_short sport, const char *ip_dst,
                  u_short dport, u_char format, struct timeval time)
{
    rtp_stream_t *reverse;

    if (stream->pktcnt) {
        stream->pktcnt++;
        return;
    }
    stream_complete(stream, ip_src, sport);
    stream->format = format;
    stream->time = time;
    stream->pktcnt++;

    if (!(reverse = rtp_find_call_stream(stream->media->msg->call, stream->ip_dst, stream->dport,  stream->ip_src,  stream->sport))) {
        reverse = stream_create(stream->media, stream->ip_src, stream->sport);
        stream_complete(reverse, stream->ip_dst, stream->dport);
        vector_append(stream->media->msg->call->streams, reverse);
    }
}

int
stream_get_count(rtp_stream_t *stream)
{
    return stream->pktcnt;
}

const char *
rtp_get_codec(int code, const char *format)
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

rtp_stream_t *
rtp_check_stream(const struct pcap_pkthdr *header, const char *src, u_short sport, const char* dst,
                 u_short dport, u_char *payload)
{
    // Media structure for RTP packets
    rtp_stream_t *stream;
    // RTP payload data
    u_char format;

    // Check if we have at least RTP type
    if (!payload || !(payload + 1))
        return NULL;

    // Get RTP payload type
    format = *(payload + 1) & RTP_FORMAT_MASK;

    // Find the matching stream
    stream = rtp_find_stream(src, sport, dst, dport);

    // if a valid stream has been found
    if (stream)  {
        //! Add packet to found stream
        stream_add_packet(stream, src, sport, dst, dport, format, header->ts);
    }

    return stream;
}

rtp_stream_t *
rtp_find_stream(const char *src, u_short sport, const char *dst, u_short dport)
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
        if ((stream = rtp_find_call_stream(call, src, sport, dst, dport))) {
            return stream;
        }
    }

    return NULL;
}

rtp_stream_t *
rtp_find_call_stream(struct sip_call *call, const char *ip_src, u_short sport, const char *ip_dst, u_short dport)
{
    rtp_stream_t *stream, *ret = NULL;
    vector_iter_t it;

    it = vector_iterator(call->streams);

    while ((stream = vector_iterator_next(&it))) {
        if (!strcmp(ip_src, stream->ip_src) && sport == stream->sport &&
            !strcmp(ip_dst, stream->ip_dst) && dport == stream->dport) {
            ret = stream;
        }
    }

    if (!ret) {
        vector_iterator_reset(&it);
        while ((stream = vector_iterator_next(&it))) {
            if (!strcmp(ip_dst, stream->ip_dst) && dport == stream->dport && !stream->complete) {
                ret = stream;
            }
        }
    }

    return ret;
}
