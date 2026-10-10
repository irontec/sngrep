/*
 * sngrep - SIP Messages flow viewer
 * Copyright (C) 2026 contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "config.h"
#include "media_inspector.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>

/*
 * Header-only, bounded-memory RTP monitoring.  Unlike the SIP/SDP-derived
 * streams in rtp.c, these entries do not require a SIP dialog.  No RTP audio
 * payload is copied or retained.
 */
static media_inspector_flow_t flows[MEDIA_INSPECTOR_MAX_FLOWS];
static size_t flow_count;
static int enabled;

static uint16_t
read16(const unsigned char *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t
read32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static uint64_t
time_us(struct timeval tv)
{
    return (uint64_t)tv.tv_sec * 1000000u + (uint64_t)tv.tv_usec;
}

static uint32_t
static_clock(uint8_t pt)
{
    /* Dynamic mappings require SDP/rtpengine metadata: unknown => no ms. */
    switch (pt) {
    case 0: case 3: case 4: case 5: case 6: case 7:
    case 8: case 12: case 13: case 15: case 18:
        return 8000;
    case 9: return 8000; /* G.722 RTP clock, not codec sample rate */
    case 10: case 11: return 44100;
    case 14: case 25: case 26: case 28:
    case 31: case 32: case 33: case 34: return 90000;
    case 16: return 11025;
    case 17: return 22050;
    default: return 0;
    }
}

void
media_inspector_set_enabled(int value)
{
    enabled = !!value;
}

int
media_inspector_enabled(void)
{
    return enabled;
}

void
media_inspector_reset(void)
{
    memset(flows, 0, sizeof(flows));
    flow_count = 0;
}

size_t
media_inspector_flow_count(void)
{
    return flow_count;
}

uint64_t
media_inspector_estimated_loss(const media_inspector_flow_t *f)
{
    uint64_t expected;
    if (!f || !f->initialized)
        return 0;
    expected = (uint64_t)f->max_seq - f->first_seq + 1;
    return expected > f->unique ? expected - f->unique : 0;
}

size_t
media_inspector_snapshot(media_inspector_flow_t *dst, size_t capacity)
{
    size_t n = flow_count < capacity ? flow_count : capacity;
    if (dst && n)
        memcpy(dst, flows, n * sizeof(*dst));
    return n;
}

static media_inspector_flow_t *
find_flow(packet_t *packet, uint32_t ssrc, uint64_t now)
{
    media_inspector_flow_t *f;
    size_t i, oldest = 0;

    for (i = 0; i < flow_count; i++) {
        f = &flows[i];
        if (f->ssrc == ssrc &&
            addressport_equals(f->src, packet->src) &&
            addressport_equals(f->dst, packet->dst))
            return f;
        if (flows[i].last_us < flows[oldest].last_us)
            oldest = i;
    }

    /* Avoid unbounded memory growth on busy media-only hosts. */
    if (flow_count < MEDIA_INSPECTOR_MAX_FLOWS)
        f = &flows[flow_count++];
    else
        f = &flows[oldest];

    memset(f, 0, sizeof(*f));
    f->src = packet->src;
    f->dst = packet->dst;
    f->ssrc = ssrc;
    f->first_us = f->last_us = now;
    return f;
}

int
media_inspector_ingest(packet_t *packet)
{
    const unsigned char *p;
    uint32_t len, header_len, ssrc, clock;
    uint16_t seq;
    uint8_t pt, cc;
    uint64_t now;
    media_inspector_flow_t *f;
    struct timeval stamp;
    int16_t delta;
    double transit, difference;

    if (!enabled || !packet || packet->proto != IPPROTO_UDP)
        return 0;

    len = packet_payloadlen(packet);
    p = packet_payload(packet);
    if (!p || len < 12 || (p[0] >> 6) != 2)
        return 0;

    /* RTCP and ambiguous payloads are excluded from RTP-only estimates. */
    if (p[1] >= 200 && p[1] <= 211 &&
        len >= 4 && ((uint32_t)read16(p + 2) + 1) * 4 <= len)
        return 0;

    cc = p[0] & 0x0f;
    header_len = 12u + (uint32_t)cc * 4u;
    if (header_len > len)
        return 0;
    if (p[0] & 0x10) {
        uint32_t ext_len;
        if (len - header_len < 4)
            return 0;
        ext_len = 4u + (uint32_t)read16(p + header_len + 2) * 4u;
        if (ext_len > len - header_len)
            return 0;
        header_len += ext_len;
    }
    if (p[0] & 0x20) {
        uint8_t pad = p[len - 1];
        if (!pad || pad > len - header_len)
            return 0;
    }

    seq = read16(p + 2);
    ssrc = read32(p + 8);
    pt = p[1] & 0x7f;
    stamp = packet_time(packet);
    now = time_us(stamp);
    f = find_flow(packet, ssrc, now);
    clock = static_clock(pt);
    f->last_us = now;
    f->packets++;
    f->bytes += len;
    f->payload_type = pt;

    if (!f->initialized) {
        f->initialized = 1;
        f->first_seq = f->max_seq = seq;
        f->seen_recent = 1;
        f->unique = 1;
        f->clock_rate = clock;
        f->jitter_available = clock != 0;
        if (clock)
            f->previous_transit = (double)now * clock / 1000000.0 -
                                  read32(p + 4);
        return 1;
    }

    /* Recent 64-packet bitmap handles duplicates, reordering and wrap. */
    delta = (int16_t)(seq - (uint16_t)f->max_seq);
    if (delta > 0) {
        f->seen_recent = delta >= 64 ? 1 : (f->seen_recent << delta) | 1;
        f->max_seq += (uint16_t)delta;
        f->unique++;
    } else if (-delta < 64 && (f->seen_recent & (UINT64_C(1) << -delta))) {
        f->duplicates++;
        return 1;
    } else {
        f->out_of_order++;
        if (-delta < 64) {
            f->seen_recent |= UINT64_C(1) << -delta;
            f->unique++;
        }
        /* Late samples outside the recent window are not counted as unique. */
    }

    /* RFC 3550 interarrival jitter; units can be converted only for known PTs. */
    if (f->clock_rate != clock || !clock) {
        f->jitter_available = 0;
    } else {
        transit = (double)now * clock / 1000000.0 - read32(p + 4);
        difference = transit - f->previous_transit;
        if (difference < 0)
            difference = -difference;
        f->jitter_units += (difference - f->jitter_units) / 16.0;
        f->jitter_ms = f->jitter_units * 1000.0 / clock;
        f->previous_transit = transit;
    }
    return 1;
}
