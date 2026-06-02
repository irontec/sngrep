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
 ****************************************************************************/
/**
 * @file test_012.c
 *
 * Basic RTP stream statistics tests.
 */

#include "config.h"
#ifdef WITH_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#endif
#include <assert.h>
#include <string.h>
#include "../src/packet.h"
#include "../src/rtp.h"
#include "../src/sip.h"

vector_iter_t
sip_calls_iterator()
{
    vector_iter_t it;
    memset(&it, 0, sizeof(it));
    return it;
}

struct sip_call *
msg_get_call(const sip_msg_t *msg)
{
    (void) msg;
    return NULL;
}

void
call_add_stream(sip_call_t *call, rtp_stream_t *stream)
{
    (void) call;
    (void) stream;
}

const char *
media_get_format(sdp_media_t *media, uint32_t code)
{
    (void) media;
    (void) code;
    return NULL;
}

#include "../src/rtp.c"

static packet_t *
rtp_packet(uint16_t seq, uint32_t ts, uint32_t usec)
{
    address_t src = { "10.0.0.1", 10000 };
    address_t dst = { "10.0.0.2", 20000 };
    struct pcap_pkthdr header;
    packet_t *packet = packet_create(4, IPPROTO_UDP, src, dst, 1);
    u_char payload[RTP_HDR_LENGTH] = { 0x80, 0x00 };

    memset(&header, 0, sizeof(header));
    header.ts.tv_sec = usec / 1000000;
    header.ts.tv_usec = usec % 1000000;
    header.caplen = sizeof(payload);
    header.len = sizeof(payload);

    payload[2] = seq >> 8;
    payload[3] = seq & 0xff;
    payload[4] = ts >> 24;
    payload[5] = (ts >> 16) & 0xff;
    payload[6] = (ts >> 8) & 0xff;
    payload[7] = ts & 0xff;
    payload[8] = 0x12;
    payload[9] = 0x34;
    payload[10] = 0x56;
    payload[11] = 0x78;

    packet_add_frame(packet, &header, payload);
    packet_set_payload(packet, payload, sizeof(payload));
    return packet;
}

static void
add_rtp(rtp_stream_t *stream, uint16_t seq, uint32_t ts, uint32_t usec)
{
    packet_t *packet = rtp_packet(seq, ts, usec);
    stream_add_packet(stream, packet);
    packet_destroy(packet);
}

int
main()
{
    address_t dst = { "10.0.0.2", 20000 };
    rtp_stream_t *stream;

    stream = stream_create(NULL, dst, PACKET_RTP);
    stream_set_format(stream, 0);
    add_rtp(stream, 10, 0, 0);
    add_rtp(stream, 11, 160, 20000);
    add_rtp(stream, 12, 320, 40000);
    assert(stream_get_expected_count(stream) == 3);
    assert(stream_get_lost_count(stream) == 0);
    assert(stream->rtpstats.duplicates == 0);
    assert(stream->rtpstats.outoforder == 0);
    assert(stream_get_duration_ms(stream) == 40);
    assert(stream_get_clock_rate(stream) == 8000);
    assert(stream->rtpstats.max_delta == 20.0);
    assert(stream->rtpstats.max_jitter == 0.0);
    assert(stream->rtpstats.mean_jitter == 0.0);
    sng_free(stream);

    stream = stream_create(NULL, dst, PACKET_RTP);
    stream_set_format(stream, 0);
    add_rtp(stream, 10, 0, 0);
    add_rtp(stream, 12, 320, 40000);
    assert(stream_get_expected_count(stream) == 3);
    assert(stream_get_lost_count(stream) == 1);
    assert(stream->rtpstats.outoforder == 1);
    add_rtp(stream, 11, 160, 20000);
    assert(stream_get_lost_count(stream) == 0);
    assert(stream->rtpstats.outoforder == 2);
    sng_free(stream);

    stream = stream_create(NULL, dst, PACKET_RTP);
    stream_set_format(stream, 0);
    add_rtp(stream, 10, 0, 0);
    add_rtp(stream, 10, 0, 0);
    assert(stream_get_expected_count(stream) == 1);
    assert(stream_get_lost_count(stream) == 0);
    assert(stream->rtpstats.duplicates == 1);
    assert(stream->rtpstats.outoforder == 1);
    sng_free(stream);

    stream = stream_create(NULL, dst, PACKET_RTP);
    stream_set_format(stream, 0);
    add_rtp(stream, 65534, 0, 0);
    add_rtp(stream, 65535, 160, 20000);
    add_rtp(stream, 0, 320, 40000);
    add_rtp(stream, 65535, 160, 20000);
    assert(stream_get_expected_count(stream) == 3);
    assert(stream_get_lost_count(stream) == 0);
    assert(stream->rtpstats.outoforder == 1);
    sng_free(stream);

    stream = stream_create(NULL, dst, PACKET_RTP);
    stream_set_format(stream, 0);
    add_rtp(stream, 10, 0, 0);
    add_rtp(stream, 11, 160, 30000);
    assert(stream->rtpstats.max_delta == 30.0);
    assert(stream->rtpstats.max_jitter > 0.62);
    assert(stream->rtpstats.max_jitter < 0.63);
    assert(stream->rtpstats.mean_jitter > 0.62);
    assert(stream->rtpstats.mean_jitter < 0.63);
    sng_free(stream);

    return 0;
}
