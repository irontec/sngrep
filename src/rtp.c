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
#include <stddef.h>
#include <time.h>
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
stream_create(sdp_media_t *media, address_t dst, int type)
{
    rtp_stream_t *stream;

    // Allocate memory for this stream structure
    if (!(stream = sng_malloc(sizeof(rtp_stream_t))))
        return NULL;

    // Initialize all fields
    stream->type = type;
    stream->media = media;
    stream->dst = dst;

    return stream;
}

rtp_stream_t *
stream_complete(rtp_stream_t *stream, address_t src)
{
    stream->src = src;
    return stream;
}

void
stream_set_format(rtp_stream_t *stream, uint32_t format)
{
    stream->rtpinfo.fmtcode = format;
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
    if ((fmt = rtp_get_standard_format(stream->rtpinfo.fmtcode)))
        return fmt;

    // Try to get format form SDP payload
    if ((fmt = media_get_format(stream->media, stream->rtpinfo.fmtcode)))
        return fmt;

    // Not found format for this code
    return NULL;
}

const char *
rtp_get_standard_format(uint32_t code)
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
rtp_check_packet(packet_t *packet)
{
    address_t src, dst;
    rtp_stream_t *stream;
    rtp_stream_t *reverse;
    u_char format = 0;
    u_char *payload;
    uint32_t size, bsize;
    uint16_t len;
    struct rtcp_hdr_generic hdr;
    struct rtcp_hdr_sr hdr_sr;
    struct rtcp_hdr_xr hdr_xr;
    struct rtcp_blk_xr blk_xr;
    struct rtcp_blk_xr_voip blk_xr_voip;

    // Get packet data
    payload = packet_payload(packet);
    size = packet_payloadlen(packet);

    // Get Addresses from packet
    src = packet->src;
    dst = packet->dst;

    if (data_is_rtp(payload, size) == 0) {

        // Get RTP payload type
        format = RTP_PAYLOAD_TYPE(*(payload + 1));

        // Find the matching stream
        stream = rtp_find_stream_format(src, dst, format);

        // Check if a valid stream has been found
        if (!stream)
            return NULL;

        // We have found a stream, but with different format
        if (stream_is_complete(stream) && stream->rtpinfo.fmtcode != format) {
            // Create a new stream for this new format
            stream = stream_create(stream->media, dst, PACKET_RTP);
            stream_complete(stream, src);
            stream_set_format(stream, format);
            call_add_stream(msg_get_call(stream->media->msg), stream);
        }

        // First packet for this stream, set source data
        if (!(stream_is_complete(stream))) {
            stream_complete(stream, src);
            stream_set_format(stream, format);

            /**
             * TODO This is a mess. Rework required
             *
             * This logic tries to handle a common problem when SDP address and RTP address
             * doesn't match. In some cases one endpoint waits until RTP data is sent to its
             * configured port in SDP and replies its RTP to the source ignoring what the other
             * endpoint has configured in its SDP.
             *
             * For such cases, we create streams 'on the fly', when a stream is completed (the
             * first time its source address is filled), a new stream is created with the
             * opposite src and dst.
             *
             * BUT, there are some cases when this 'reverse' stream should not be created:
             *  - When there already exists a stream with that setup
             *  - When there exists an incomplete stream with that destination (and still no source)
             *  - ...
             *
             */

            // Check if an stream in the opposite direction exists
            if (!(reverse = rtp_find_call_stream(stream->media->msg->call, stream->dst, stream->src))) {
                reverse = stream_create(stream->media, stream->src, PACKET_RTP);
                stream_complete(reverse, stream->dst);
                stream_set_format(reverse, format);
                call_add_stream(msg_get_call(stream->media->msg), reverse);
            } else {
                // If the reverse stream has other source configured
                if (reverse->src.port && !addressport_equals(stream->src, reverse->src)) {
                    if (!(reverse = rtp_find_call_exact_stream(stream->media->msg->call, stream->dst, stream->src))) {
                        // Create a new reverse stream
                        reverse = stream_create(stream->media, stream->src, PACKET_RTP);
                        stream_complete(reverse, stream->dst);
                        stream_set_format(reverse, format);
                        call_add_stream(msg_get_call(stream->media->msg), reverse);
                    }
                }
            }
        }

        // Add packet to stream
        stream_add_packet(stream, packet);
    } else if (data_is_rtcp(payload, size) == 0) {
        // Find the matching stream
        if ((stream = rtp_find_rtcp_stream(src, dst))) {

            // Parse all packet payload headers
            while ((int32_t) size > 0) {

                // Check we have at least rtcp generic info
                if (size < sizeof(struct rtcp_hdr_generic))
                    break;

                memcpy(&hdr, payload, sizeof(hdr));

                // Check RTP version
                if (RTP_VERSION(hdr.version) != RTP_VERSION_RFC1889)
                    break;

                // Header length
                if ((len = ntohs(hdr.len) * 4 + 4) > size)
                    break;

                // Check RTCP packet header typ
                switch (hdr.type) {
                    case RTCP_HDR_SR:
                        // Get Sender Report header
                        memcpy(&hdr_sr, payload, sizeof(hdr_sr));
                        stream->rtcpinfo.spc = ntohl(hdr_sr.spc);
                        break;
                    case RTCP_HDR_RR:
                    case RTCP_HDR_SDES:
                    case RTCP_HDR_BYE:
                    case RTCP_HDR_APP:
                    case RTCP_RTPFB:
                    case RTCP_PSFB:
                        break;
                    case RTCP_XR:
                        // Get Sender Report Extended header
                        memcpy(&hdr_xr, payload, sizeof(hdr_xr));
                        bsize = sizeof(hdr_xr);

                        // Read all report blocks
                        while (bsize < ntohs(hdr_xr.len) * 4 + 4) {
                            // Read block header
                            memcpy(&blk_xr, payload + bsize, sizeof(blk_xr));
                            // Check block type
                            switch (blk_xr.type) {
                                case RTCP_XR_VOIP_METRCS:
                                    memcpy(&blk_xr_voip, payload + sizeof(hdr_xr), sizeof(blk_xr_voip));
                                    stream->rtcpinfo.fdiscard = blk_xr_voip.drate;
                                    stream->rtcpinfo.flost = blk_xr_voip.lrate;
                                    stream->rtcpinfo.mosl = blk_xr_voip.moslq;
                                    stream->rtcpinfo.mosc = blk_xr_voip.moscq;
                                    break;
                                default: break;
                            }
                            bsize += ntohs(blk_xr.len) * 4 + 4;
                        }
                        break;
                    case RTCP_AVB:
                    case RTCP_RSI:
                    case RTCP_TOKEN:
                    default:
                        // Not handled headers. Skip the rest of this packet
                        size = 0;
                        break;
                }
                payload += len;
                size -= len;
            }

            // Add packet to stream
            stream_complete(stream, src);
            stream_add_packet(stream, packet);
        }
    } else {
      return NULL;
    }

    return stream;
}

rtp_stream_t *
rtp_find_stream_format(address_t src, address_t dst, uint32_t format)
{
    // Structure for RTP packet streams
    rtp_stream_t *stream;
    // Check if this is a RTP packet from active calls
    sip_call_t *call;
    // Iterator for active calls
    vector_iter_t calls;
    // Iterator for call streams
    vector_iter_t streams;
    // Candiate stream
    rtp_stream_t *candidate = NULL;

    // Get active calls (during conversation)
    calls = sip_calls_iterator();
    vector_iterator_set_current(&calls, vector_iterator_count(&calls));

    while ((call = vector_iterator_prev(&calls))) {
        streams = vector_iterator(call->streams);
        vector_iterator_set_last(&streams);
        while ((stream = vector_iterator_prev(&streams))) {
            // Only look RTP packets
            if (stream->type != PACKET_RTP)
                continue;

            // Stream complete, check source, dst
            if (stream_is_complete(stream)) {
                if (addressport_equals(stream->src, src) &&
                    addressport_equals(stream->dst, dst)) {
                    // Exact searched stream format
                    if (stream->rtpinfo.fmtcode == format) {
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
rtp_find_rtcp_stream(address_t src, address_t dst)
{
    // Structure for RTP packet streams
    rtp_stream_t *stream;
    // Check if this is a RTP packet from active calls
    sip_call_t *call;
    // Iterator for active calls
    vector_iter_t calls;

    // Get active calls (during conversation)
    calls = sip_calls_iterator();
    vector_iterator_set_current(&calls, vector_iterator_count(&calls));

    while ((call = vector_iterator_prev(&calls))) {
        // Check if this call has an RTP stream for current packet data
        if ((stream = rtp_find_call_stream(call, src, dst))) {
            if (stream->type != PACKET_RTCP)
                continue;
            return stream;
        }
    }

    return NULL;
}


rtp_stream_t *
rtp_find_call_stream(struct sip_call *call, address_t src, address_t dst)
{
    rtp_stream_t *stream;
    vector_iter_t it;

    // Create an iterator for call streams
    it = vector_iterator(call->streams);

    // Look for an incomplete stream with this destination
    vector_iterator_set_last(&it);
    while ((stream = vector_iterator_prev(&it))) {
        if (addressport_equals(dst, stream->dst)) {
            if (!src.port) {
                return stream;
            } else {
                if (!stream->pktcnt) {
                    return stream;
                }
            }
        }
    }

    // Try to look for a complete stream with this destination
    if (src.port) {
        return rtp_find_call_exact_stream(call, src, dst);
    }

    // Nothing found
    return NULL;
}

rtp_stream_t *
rtp_find_call_exact_stream(struct sip_call *call, address_t src, address_t dst)
{
    rtp_stream_t *stream;
    vector_iter_t it;

    // Create an iterator for call streams
    it = vector_iterator(call->streams);

    vector_iterator_set_last(&it);
    while ((stream = vector_iterator_prev(&it))) {
        if (addressport_equals(src, stream->src) &&
            addressport_equals(dst, stream->dst)) {
            return stream;
        }
    }

    // Nothing found
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

int
data_is_rtp(u_char *data, uint32_t len)
{
    u_char pt = RTP_PAYLOAD_TYPE(*(data + 1));

    if ((len >= RTP_HDR_LENGTH) &&
        (RTP_VERSION(*data) == RTP_VERSION_RFC1889) &&
        (data[0] > 127 && data[0] < 192) &&
        (pt <= 64 || pt >= 96)) {
        return 0;
    }

    // Not a RTP packet
    return 1;
}

int
data_is_rtcp(u_char *data, uint32_t len)
{
    struct rtcp_hdr_generic *hdr = (struct rtcp_hdr_generic*) data;

    if ((len >= RTCP_HDR_LENGTH) &&
        (RTP_VERSION(*data) == RTP_VERSION_RFC1889) &&
        (data[0] > 127 && data[0] < 192) &&
        (hdr->type >= 192 && hdr->type <= 223)) {
        return 0;
    }

    // Not a RTCP packet
    return 1;
}
