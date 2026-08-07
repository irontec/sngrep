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
 * @file capture_esp.c
 * @author Marco Sinibaldi <marco.sinibaldi@hpe.com>
 *
 * @brief Source of functions defined in capture_esp.h
 *
 */

/* Request BSD-style struct member names (uh_sport, th_off, ...) */
#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stddef.h>
#include "capture_esp.h"

/**
 * @brief Candidate ICV (Integrity Check Value) lengths to probe, in bytes.
 *
 * The ICV length is not carried on the wire; it depends on the negotiated
 * integrity algorithm. The most common IMS/IPsec values are tried first:
 *  12 = HMAC-MD5-96 / HMAC-SHA1-96 / AES-XCBC-96 (typical IMS Gm)
 *  16 = HMAC-SHA-256-128 / AES-GMAC-128
 *  24 = HMAC-SHA-384-192
 *  32 = HMAC-SHA-512-256
 *   0 = NULL integrity (no ICV)
 */
static const uint32_t esp_icv_lengths[] = {12, 16, 24, 32, 0};

/**
 * @brief Validate the RFC 4303 self-describing padding pattern (1, 2, 3, ...)
 */
static int
esp_padding_valid(const u_char *pad, uint32_t pad_len)
{
    uint32_t i;

    for (i = 0; i < pad_len; i++)
    {
        if (pad[i] != (u_char)(i + 1))
            return 0;
    }
    return 1;
}

int capture_esp_null_parse(const u_char *esp, uint32_t esp_len,
                           uint8_t *proto, uint16_t *sport, uint16_t *dport,
                           uint32_t *payload_off, uint32_t *payload_len)
{
    const u_char *inner;
    uint32_t inner_len;
    size_t i;

    // Need at least the ESP header plus a minimal transport header
    if (esp == NULL || esp_len < ESP_HEADER_LEN + sizeof(struct udphdr))
        return -1;

    // Inner (clear text) data follows the 8-byte ESP header (SPI + Sequence)
    inner = esp + ESP_HEADER_LEN;
    inner_len = esp_len - ESP_HEADER_LEN;

    // The ESP trailer sits at the end: [ Padding | Pad Length | Next Header | ICV ].
    // The ICV length is unknown, so probe the common values and validate each
    // candidate against the padding pattern and the resulting transport header.
    for (i = 0; i < sizeof(esp_icv_lengths) / sizeof(esp_icv_lengths[0]); i++)
    {
        uint32_t icv_len = esp_icv_lengths[i];
        uint8_t next_header;
        uint32_t pad_len;
        uint32_t tlen;

        // Room for Pad Length + Next Header + ICV
        if (inner_len < icv_len + 2)
            continue;

        next_header = inner[inner_len - icv_len - 1];
        pad_len = inner[inner_len - icv_len - 2];

        // Only interested in inner UDP or TCP
        if (next_header != IPPROTO_UDP && next_header != IPPROTO_TCP)
            continue;

        // Room for the padding itself
        if (2 + pad_len + icv_len > inner_len)
            continue;

        // Length of the inner transport segment (header + payload)
        tlen = inner_len - icv_len - 2 - pad_len;

        // Validate the self-describing padding bytes (1, 2, 3, ...)
        if (!esp_padding_valid(inner + tlen, pad_len))
            continue;

        if (next_header == IPPROTO_TCP)
        {
            const struct tcphdr *tcp = (const struct tcphdr *)inner;
            uint32_t tcp_off;

            if (tlen < sizeof(struct tcphdr))
                continue;

            tcp_off = (uint32_t)tcp->th_off * 4;
            if (tcp_off < sizeof(struct tcphdr) || tcp_off > tlen)
                continue;

            *proto = IPPROTO_TCP;
            *sport = ntohs(tcp->th_sport);
            *dport = ntohs(tcp->th_dport);
            *payload_off = ESP_HEADER_LEN + tcp_off;
            *payload_len = tlen - tcp_off;
            return 0;
        }
        else
        {
            const struct udphdr *udp = (const struct udphdr *)inner;

            if (tlen < sizeof(struct udphdr))
                continue;

            // The UDP length field must match the trailer-derived segment size
            if (ntohs(udp->uh_ulen) != tlen)
                continue;

            *proto = IPPROTO_UDP;
            *sport = ntohs(udp->uh_sport);
            *dport = ntohs(udp->uh_dport);
            *payload_off = ESP_HEADER_LEN + sizeof(struct udphdr);
            *payload_len = tlen - sizeof(struct udphdr);
            return 0;
        }
    }

    // Not a recognizable NULL-encrypted ESP payload
    return -1;
}
