/*
 * sngrep - SIP Messages flow viewer
 * Copyright (C) 2026 contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Read-only RTPengine NG control client: polls "list" and "query" once per
 * second and keeps per-leg packet/byte counters for the Media Inspector.
 */
#include "config.h"
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "rtpengine.h"

#define BENC_MAX_DEPTH 32
#define RTPENGINE_TIMEOUT_US 500000

static struct {
    pthread_mutex_t lock;
    char addr[128];
    int started;
    rtpengine_status_t status;
    rtpengine_call_t calls[RTPENGINE_MAX_CALLS];
    size_t count;
} rtpe = { .lock = PTHREAD_MUTEX_INITIALIZER, .addr = RTPENGINE_DEFAULT_ADDR };

/* Reads a bencoded string at p; returns the position after it or NULL. */
static const char *
benc_str(const char *p, const char *end, const char **s, size_t *n)
{
    size_t len = 0;
    if (!p || p >= end || *p < '0' || *p > '9')
        return NULL;
    while (p < end && *p >= '0' && *p <= '9') {
        if (len > (size_t)(end - p))
            return NULL;
        len = len * 10 + (size_t)(*p++ - '0');
    }
    if (p >= end || *p != ':' || len > (size_t)(end - p - 1))
        return NULL;
    *s = p + 1;
    *n = len;
    return p + 1 + len;
}

static int
benc_int(const char *p, const char *end, uint64_t *v)
{
    uint64_t x = 0;
    if (!p || p >= end || *p != 'i')
        return -1;
    for (p++; p < end && *p >= '0' && *p <= '9'; p++)
        x = x * 10 + (uint64_t)(*p - '0');
    if (p >= end || *p != 'e')
        return -1;
    *v = x;
    return 0;
}

/* Returns the position after the value at p, or NULL when malformed. */
static const char *
benc_skip(const char *p, const char *end, int depth)
{
    const char *s;
    size_t n;
    if (!p || p >= end || depth > BENC_MAX_DEPTH)
        return NULL;
    if (*p == 'i') {
        p = memchr(p, 'e', (size_t)(end - p));
        return p ? p + 1 : NULL;
    }
    if (*p == 'l' || *p == 'd') {
        for (p++; p && p < end && *p != 'e'; )
            p = benc_skip(p, end, depth + 1);
        return p && p < end ? p + 1 : NULL;
    }
    return benc_str(p, end, &s, &n);
}

/* Returns the value stored under key in the dictionary at p, or NULL. */
static const char *
benc_get(const char *p, const char *end, const char *key)
{
    const char *k;
    size_t n, keylen = strlen(key);
    if (!p || p >= end || *p != 'd')
        return NULL;
    for (p++; p && p < end && *p != 'e'; p = benc_skip(p, end, 0)) {
        if (!(p = benc_str(p, end, &k, &n)))
            return NULL;
        if (n == keylen && !memcmp(k, key, n))
            return p;
    }
    return NULL;
}

static int
benc_is(const char *p, const char *end, const char *text)
{
    const char *s;
    size_t n;
    return benc_str(p, end, &s, &n) && n == strlen(text) && !memcmp(s, text, n);
}

static void
copy_str(char *dst, size_t size, const char *s, size_t n)
{
    if (n >= size)
        n = size - 1;
    memcpy(dst, s, n);
    dst[n] = '\0';
}

static void
parse_stream(const char *st, const char *end, rtpengine_leg_t *leg)
{
    const char *flags = benc_get(st, end, "flags");
    const char *stats = benc_get(st, end, "stats");
    const char *ep = benc_get(st, end, "endpoint");
    const char *f, *s;
    size_t n;
    uint64_t x;
    int rtp = 0, kernel = 0, no_kernel = 0;

    if (flags && *flags == 'l') {
        for (f = flags + 1; f && f < end && *f != 'e'; f = benc_skip(f, end, 0)) {
            rtp |= benc_is(f, end, "RTP");
            kernel |= benc_is(f, end, "kernelized");
            no_kernel |= benc_is(f, end, "no kernel support");
        }
    }
    // RTPengine marks streams "kernelized" even when the module is missing
    leg->kernelized |= kernel && !no_kernel;
    // RTCP-only streams would double count packets of the same party
    if (!rtp)
        return;
    if (!benc_int(benc_get(stats, end, "packets"), end, &x))
        leg->packets += x;
    if (!benc_int(benc_get(stats, end, "bytes"), end, &x))
        leg->bytes += x;
    if (!benc_int(benc_get(stats, end, "errors"), end, &x))
        leg->errors += x;
    if (!leg->address[0] && benc_str(benc_get(ep, end, "address"), end, &s, &n)) {
        copy_str(leg->address, sizeof(leg->address), s, n);
        if (!benc_int(benc_get(ep, end, "port"), end, &x))
            leg->port = (unsigned)x;
    }
}

static int
parse_leg(const char *tag, const char *end, rtpengine_leg_t *leg)
{
    const char *medias = benc_get(tag, end, "medias"), *m, *streams, *st;
    if (!medias || *medias != 'l')
        return 0;
    for (m = medias + 1; m && m < end && *m != 'e'; m = benc_skip(m, end, 0)) {
        streams = benc_get(m, end, "streams");
        if (!streams || *streams != 'l')
            continue;
        for (st = streams + 1; st && st < end && *st != 'e'; st = benc_skip(st, end, 0))
            parse_stream(st, end, leg);
        if (!st)
            return -1;
    }
    return m ? 0 : -1;
}

int
rtpengine_parse_query(const char *buf, size_t len, rtpengine_call_t *call)
{
    const char *end = buf + len, *tags, *p, *k;
    size_t n;

    if (!benc_is(benc_get(buf, end, "result"), end, "ok"))
        return -1;
    if (!(tags = benc_get(buf, end, "tags")) || *tags != 'd')
        return -1;
    call->nlegs = 0;
    // ponytail: forked calls have more than two tags; only the first two are shown
    for (p = tags + 1; p < end && *p != 'e' && call->nlegs < RTPENGINE_MAX_LEGS; ) {
        rtpengine_leg_t *leg = &call->legs[call->nlegs];
        if (!(p = benc_str(p, end, &k, &n)))
            return -1;
        memset(leg, 0, sizeof(*leg));
        copy_str(leg->tag, sizeof(leg->tag), k, n);
        if (parse_leg(p, end, leg))
            return -1;
        call->nlegs++;
        if (!(p = benc_skip(p, end, 0)))
            return -1;
    }
    return 0;
}

static int
ng_connect(const char *addr)
{
    char host[128], *port, *name = host, *bracket;
    struct addrinfo hints, *res, *ai;
    struct timeval tv = { 0, RTPENGINE_TIMEOUT_US };
    int fd = -1;

    snprintf(host, sizeof(host), "%s", addr);
    if (!(port = strrchr(host, ':')))
        return -1;
    *port++ = '\0';
    if (*name == '[' && (bracket = strchr(++name, ']')))
        *bracket = '\0';
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(name, port, &hints, &res))
        return -1;
    for (ai = res; ai && fd < 0; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd >= 0 && connect(fd, ai->ai_addr, ai->ai_addrlen)) {
            close(fd);
            fd = -1;
        }
    }
    freeaddrinfo(res);
    if (fd >= 0)
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return fd;
}

/* Sends one NG command; on success *dict points at the response body. */
static ssize_t
ng_request(int fd, const char *body, char *buf, size_t size, const char **dict)
{
    static unsigned seq;
    static time_t started;
    char msg[512];
    int clen, len;
    ssize_t n;

    // RTPengine replays cached replies for a repeated cookie, so make it unique per run
    if (!started)
        started = time(NULL);
    clen = snprintf(msg, sizeof(msg), "%d_%ld_%u ", (int)getpid(), (long)started, ++seq);
    len = snprintf(msg + clen, sizeof(msg) - (size_t)clen, "%s", body) + clen;
    if (len >= (int)sizeof(msg) || send(fd, msg, (size_t)len, 0) < 0)
        return -1;
    // Late replies to earlier requests carry another cookie and are dropped
    while ((n = recv(fd, buf, size, 0)) >= 0) {
        if (n > clen && !memcmp(buf, msg, (size_t)clen)) {
            *dict = buf + clen;
            return n - clen;
        }
    }
    return -1;
}

static int
poll_calls(int fd, char *buf, size_t size, rtpengine_call_t *calls, size_t *count)
{
    char body[256];
    const char *d, *p, *s, *end;
    ssize_t len;
    size_t n, i, kept = 0, total = 0;

    snprintf(body, sizeof(body), "d7:command4:list5:limiti%dee", RTPENGINE_MAX_CALLS);
    if ((len = ng_request(fd, body, buf, size, &d)) < 0)
        return -1;
    end = d + len;
    if (!(p = benc_get(d, end, "calls")) || *p != 'l')
        return -1;
    for (p++; p && p < end && *p != 'e' && total < RTPENGINE_MAX_CALLS; p = benc_skip(p, end, 0)) {
        if (benc_str(p, end, &s, &n) && n < sizeof(calls[0].callid))
            copy_str(calls[total++].callid, sizeof(calls[0].callid), s, n);
    }

    for (i = 0; i < total; i++) {
        rtpengine_call_t call;
        memset(&call, 0, sizeof(call));
        memcpy(call.callid, calls[i].callid, sizeof(call.callid));
        snprintf(body, sizeof(body), "d7:command5:query7:call-id%zu:%se",
                 strlen(call.callid), call.callid);
        if ((len = ng_request(fd, body, buf, size, &d)) < 0)
            return -1;
        // A call may end between "list" and "query"; that is not an error
        if (!rtpengine_parse_query(d, (size_t)len, &call))
            calls[kept++] = call;
    }
    *count = kept;
    return 0;
}

static const rtpengine_leg_t *
find_leg(const char *callid, const char *tag)
{
    size_t i, j;
    for (i = 0; i < rtpe.count; i++) {
        if (strcmp(rtpe.calls[i].callid, callid))
            continue;
        for (j = 0; j < rtpe.calls[i].nlegs; j++)
            if (!strcmp(rtpe.calls[i].legs[j].tag, tag))
                return &rtpe.calls[i].legs[j];
    }
    return NULL;
}

static void
publish(rtpengine_call_t *calls, size_t count, rtpengine_status_t status, double elapsed)
{
    size_t i, j;
    pthread_mutex_lock(&rtpe.lock);
    for (i = 0; i < count; i++) {
        for (j = 0; j < calls[i].nlegs; j++) {
            rtpengine_leg_t *leg = &calls[i].legs[j];
            const rtpengine_leg_t *old = find_leg(calls[i].callid, leg->tag);
            if (old && elapsed > 0 && leg->bytes >= old->bytes)
                leg->kbps = (double)(leg->bytes - old->bytes) * 8 / 1000 / elapsed;
        }
    }
    memcpy(rtpe.calls, calls, count * sizeof(*calls));
    rtpe.count = count;
    rtpe.status = status;
    pthread_mutex_unlock(&rtpe.lock);
}

static double
now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void *
poller(void *arg)
{
    static char buf[65536];
    static rtpengine_call_t calls[RTPENGINE_MAX_CALLS];
    double last = 0, now;
    size_t count;
    int fd = -1;
    (void)arg;

    for (;;) {
        count = 0;
        if (fd < 0)
            fd = ng_connect(rtpe.addr);
        if (fd >= 0 && poll_calls(fd, buf, sizeof(buf), calls, &count)) {
            close(fd);
            fd = -1;
        }
        now = now_seconds();
        publish(calls, count, fd >= 0 ? RTPENGINE_CONNECTED : RTPENGINE_UNREACHABLE,
                last ? now - last : 0);
        last = now;
        sleep(1);
    }
    return NULL;
}

void
rtpengine_set_address(const char *addr)
{
    snprintf(rtpe.addr, sizeof(rtpe.addr), "%s", addr);
}

const char *
rtpengine_address(void)
{
    return rtpe.addr;
}

void
rtpengine_start(void)
{
    pthread_t thread;
    pthread_mutex_lock(&rtpe.lock);
    if (!rtpe.started) {
        rtpe.started = 1;
        rtpe.status = RTPENGINE_CONNECTING;
        if (pthread_create(&thread, NULL, poller, NULL) == 0)
            pthread_detach(thread);
        else
            rtpe.status = RTPENGINE_UNREACHABLE;
    }
    pthread_mutex_unlock(&rtpe.lock);
}

rtpengine_status_t
rtpengine_status(void)
{
    rtpengine_status_t status;
    pthread_mutex_lock(&rtpe.lock);
    status = rtpe.status;
    pthread_mutex_unlock(&rtpe.lock);
    return status;
}

size_t
rtpengine_snapshot(rtpengine_call_t *out, size_t capacity)
{
    size_t n;
    pthread_mutex_lock(&rtpe.lock);
    n = rtpe.count < capacity ? rtpe.count : capacity;
    memcpy(out, rtpe.calls, n * sizeof(*out));
    pthread_mutex_unlock(&rtpe.lock);
    return n;
}
