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
 * @file sngrep_tls.h
 *
 * @brief Data shared between the eBPF programs and sngrep
 *
 * This header is included both by the BPF program, compiled by clang for the
 * bpf target, and by the sngrep binary. It must therefore avoid libbpf, kernel
 * internals and anything else that only one of the two can parse.
 *
 */
#ifndef __SNGREP_TLS_BPF_H
#define __SNGREP_TLS_BPF_H

/*
 * Only fixed width integers are used below, so this header stays portable and
 * its unit test can build on platforms that have no eBPF support at all.
 */
#include <stdint.h>
#include <stddef.h>

//! Maximum plaintext carried by a single event; longer writes are chunked
#define SNGREP_TLS_MAX_CHUNK 8192

//! Chunks one message may be split into
#define SNGREP_TLS_MAX_CHUNKS 2

/*
 * Largest message captured. Two chunks is not a compromise: sngrep discards
 * SIP payloads over MAX_SIP_PAYLOAD, which is 10240 bytes, so capturing more
 * than this could not produce a visible call anyway.
 */
#define SNGREP_TLS_MAX_MSG (SNGREP_TLS_MAX_CHUNK * SNGREP_TLS_MAX_CHUNKS)
//! Length of the task comm field, matching the kernel TASK_COMM_LEN
#define SNGREP_TLS_COMM_LEN  16

//! SSL_write: local -> peer
#define SNGREP_TLS_EGRESS    0
//! SSL_read: peer -> local
#define SNGREP_TLS_INGRESS   1

/**
 * @brief One chunk of TLS plaintext with its connection tuple
 *
 * Emitted with bpf_ringbuf_output() using only SNGREP_TLS_EVENT_HDRLEN + len
 * bytes, so the unused tail of @data is never copied.
 */
struct tls_event {
    //! Timestamp from bpf_ktime_get_boot_ns()
    uint64_t ts_ns;
    //! Process that produced this data
    uint32_t pid;
    //! Thread that produced this data
    uint32_t tid;
    //! IP version, 4 or 6
    uint8_t  family;
    //! SNGREP_TLS_EGRESS or SNGREP_TLS_INGRESS
    uint8_t  direction;
    //! Non-zero when a continuation chunk follows
    uint8_t  more;
    //! Unused, keeps the addresses aligned
    uint8_t  pad;
    //! Addresses in network byte order; only the first 4 bytes used for IPv4
    uint8_t  saddr[16];
    uint8_t  daddr[16];
    //! Ports in host byte order
    uint16_t sport;
    uint16_t dport;
    //! Valid bytes in @data
    uint32_t len;
    //! Owning process name, for diagnostics
    char     comm[SNGREP_TLS_COMM_LEN];
    //! Plaintext
    uint8_t  data[SNGREP_TLS_MAX_CHUNK];
};

//! Bytes of struct tls_event preceding the payload
#define SNGREP_TLS_EVENT_HDRLEN offsetof(struct tls_event, data)

//! Plaintext dropped because the ring buffer was full
#define SNGREP_TLS_STAT_RINGBUF_FULL 0
//! Plaintext dropped because its memory could not be read
#define SNGREP_TLS_STAT_READ_FAILED  1
//! Number of counters in the stats map
#define SNGREP_TLS_STAT_NR           2

/*
 * The prefilter below is compiled into both the BPF program and the userspace
 * unit test, so the two can not drift apart. The BPF build defines
 * SNGREP_TLS_INLINE to "static __always_inline" before including this header,
 * because the verifier requires it to be fully inlined.
 */
#ifndef SNGREP_TLS_INLINE
#define SNGREP_TLS_INLINE static inline
#endif

//! Longest prefix the filter has to examine ("SUBSCRIBE ")
#define SNGREP_TLS_PFX_MAX 10

/**
 * @brief Compare the first @n bytes of @d against @p
 *
 * The loop is bounded by SNGREP_TLS_PFX_MAX so the BPF verifier can prove it
 * terminates.
 *
 * @return 1 on match, 0 otherwise
 */
SNGREP_TLS_INLINE int
sngrep_tls_prefix(const unsigned char *d, unsigned int len,
                  const char *p, unsigned int n)
{
    unsigned int i;

    if (len < n)
        return 0;

    for (i = 0; i < SNGREP_TLS_PFX_MAX; i++) {
        if (i >= n)
            break;
        if (d[i] != (unsigned char) p[i])
            return 0;
    }
    return 1;
}

/**
 * @brief Cheap test for data worth sending to userspace
 *
 * Matches SIP request start lines, SIP responses, and WebSocket text or binary
 * frames, because SIP over WSS carries WebSocket framing rather than SIP text.
 * Everything else, overwhelmingly HTTPS from unrelated processes, is dropped
 * in kernel space.
 *
 * The trailing space in every method prefix matters: without it "INVITED"
 * would be captured as an INVITE.
 *
 * @return 1 if the buffer should be captured, 0 otherwise
 */
SNGREP_TLS_INLINE int
sngrep_tls_looks_like_sip(const unsigned char *d, unsigned int len)
{
    if (len < 4)
        return 0;

    // WebSocket frame: FIN set with a text (0x1) or binary (0x2) opcode
    if (d[0] == 0x81 || d[0] == 0x82)
        return 1;

    switch (d[0]) {
        case 'A': return sngrep_tls_prefix(d, len, "ACK ", 4);
        case 'B': return sngrep_tls_prefix(d, len, "BYE ", 4);
        case 'C': return sngrep_tls_prefix(d, len, "CANCEL ", 7);
        case 'I': return sngrep_tls_prefix(d, len, "INVITE ", 7)
                      || sngrep_tls_prefix(d, len, "INFO ", 5);
        case 'M': return sngrep_tls_prefix(d, len, "MESSAGE ", 8);
        case 'N': return sngrep_tls_prefix(d, len, "NOTIFY ", 7);
        case 'O': return sngrep_tls_prefix(d, len, "OPTIONS ", 8);
        case 'P': return sngrep_tls_prefix(d, len, "PRACK ", 6)
                      || sngrep_tls_prefix(d, len, "PUBLISH ", 8);
        case 'R': return sngrep_tls_prefix(d, len, "REGISTER ", 9)
                      || sngrep_tls_prefix(d, len, "REFER ", 6);
        case 'S': return sngrep_tls_prefix(d, len, "SIP/2.0", 7)
                      || sngrep_tls_prefix(d, len, "SUBSCRIBE ", 10);
        case 'U': return sngrep_tls_prefix(d, len, "UPDATE ", 7);
        default:  return 0;
    }
}

#endif /* __SNGREP_TLS_BPF_H */
