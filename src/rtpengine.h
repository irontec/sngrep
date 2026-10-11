/*
 * sngrep - SIP Messages flow viewer
 * Copyright (C) 2026 contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SNGREP_RTPENGINE_H
#define SNGREP_RTPENGINE_H

#include <stddef.h>
#include <stdint.h>

#define RTPENGINE_DEFAULT_ADDR "127.0.0.1:2223"
// ponytail: one query per call each second; raise with a batched API if hosts carry more
#define RTPENGINE_MAX_CALLS 200
#define RTPENGINE_MAX_LEGS 2

/* One SIP party as RTPengine sees it: media received from that endpoint. */
typedef struct rtpengine_leg {
    char tag[64];
    char address[64];
    unsigned port;
    uint64_t packets, bytes, errors;
    double kbps;
    int kernelized;
} rtpengine_leg_t;

typedef struct rtpengine_call {
    char callid[128];
    rtpengine_leg_t legs[RTPENGINE_MAX_LEGS];
    size_t nlegs;
} rtpengine_call_t;

typedef enum rtpengine_status {
    RTPENGINE_IDLE = 0,
    RTPENGINE_CONNECTING,
    RTPENGINE_UNREACHABLE,
    RTPENGINE_CONNECTED,
} rtpengine_status_t;

/* Sets the NG control address ("host:port" or "[v6]:port"); before start only. */
void rtpengine_set_address(const char *addr);
const char *rtpengine_address(void);
/* Starts the background poller once; later calls are no-ops. */
void rtpengine_start(void);
rtpengine_status_t rtpengine_status(void);
/* Copies the last polled sessions; returns how many were written. */
size_t rtpengine_snapshot(rtpengine_call_t *out, size_t capacity);

/* Parses a bencoded NG "query" response dictionary into call. 0 on success. */
int rtpengine_parse_query(const char *buf, size_t len, rtpengine_call_t *call);

#endif
