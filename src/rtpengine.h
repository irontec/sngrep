/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
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
 * @file rtpengine.h
 * @author Mohammad Bagher Amini <darwvin@hotmail.com>
 *
 * @brief Read-only RTPengine NG control client
 *
 * Polls "list" and "query" once per second over the NG control socket and
 * keeps per-leg packet/byte counters for the Media Inspector.
 */
#ifndef SNGREP_RTPENGINE_H
#define SNGREP_RTPENGINE_H

#include <stddef.h>
#include <stdint.h>

//! Default NG control address (RTPengine --listen-ng)
#define RTPENGINE_DEFAULT_ADDR "127.0.0.1:2223"
//! One query per call each second; raise with a batched API if hosts carry more
#define RTPENGINE_MAX_CALLS 200
//! Forked calls have more tags; only the first ones are shown
#define RTPENGINE_MAX_LEGS 2
//! Maximum nesting accepted from a bencoded reply
#define BENC_MAX_DEPTH 32
//! Time to wait for a reply before giving up on the socket
#define RTPENGINE_TIMEOUT_US 500000

//! Shorter declaration of rtpengine structures
typedef struct rtpengine_leg rtpengine_leg_t;
typedef struct rtpengine_call rtpengine_call_t;

/**
 * @brief One SIP party as RTPengine sees it
 *
 * Counters are the media received from that endpoint.
 */
struct rtpengine_leg
{
    //! RTPengine tag (SIP from-tag/to-tag)
    char tag[64];
    //! Endpoint address of the first RTP stream
    char address[64];
    //! Endpoint port of the first RTP stream
    unsigned port;
    //! Counters of RTP streams, RTCP excluded
    uint64_t packets, bytes, errors;
    //! Bitrate received from this leg since the previous poll
    double kbps;
    //! Media is forwarded by the kernel module
    int kernelized;
};

/**
 * @brief One RTPengine call with its first legs
 */
struct rtpengine_call
{
    //! SIP Call-ID
    char callid[128];
    //! Call legs in the order reported by RTPengine, offerer first
    rtpengine_leg_t legs[RTPENGINE_MAX_LEGS];
    //! Number of legs filled in
    size_t nlegs;
};

/**
 * @brief Status of the NG control socket
 */
typedef enum rtpengine_status
{
    RTPENGINE_IDLE = 0,
    RTPENGINE_CONNECTING,
    RTPENGINE_UNREACHABLE,
    RTPENGINE_CONNECTED,
} rtpengine_status_t;

/**
 * @brief Set the NG control address
 *
 * Must be called before rtpengine_start.
 *
 * @param addr "host:port" or "[v6]:port"
 */
void
rtpengine_set_address(const char *addr);

/**
 * @brief Get the NG control address
 *
 * @return address string as given to rtpengine_set_address or the default
 */
const char *
rtpengine_address(void);

/**
 * @brief Start the background poller once
 *
 * Later calls are no-ops.
 */
void
rtpengine_start(void);

/**
 * @brief Get the current status of the NG control socket
 *
 * @return RTPENGINE_IDLE until rtpengine_start is called
 */
rtpengine_status_t
rtpengine_status(void);

/**
 * @brief Copy the last polled sessions
 *
 * @param out destination array
 * @param capacity number of entries available in out
 * @return number of entries written
 */
size_t
rtpengine_snapshot(rtpengine_call_t *out, size_t capacity);

/**
 * @brief Parse a bencoded NG "query" response dictionary
 *
 * @param buf response body, not NUL terminated
 * @param len response length in bytes
 * @param call structure filled with the call legs
 * @return 0 on success, -1 on error replies or malformed input
 */
int
rtpengine_parse_query(const char *buf, size_t len, rtpengine_call_t *call);

#endif /* SNGREP_RTPENGINE_H */
