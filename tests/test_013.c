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
 * @file test_013.c
 * @author Marco Sinibaldi <marco.sinibaldi@hpe.com>
 *
 * Unit tests for the IPsec ESP NULL-encryption inner-transport decoder
 * (capture_esp_null_parse). The decoder parses the ESP trailer to recover the
 * exact inner payload for both UDP and TCP, auto-detecting the ICV length.
 */
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include "../src/capture_esp.h"

//! Store a 16-bit value in network (big-endian) byte order
static void
put16(u_char *p, uint16_t v)
{
    p[0] = (u_char)(v >> 8);
    p[1] = (u_char)(v & 0xff);
}

/**
 * @brief Assemble an ESP-NULL packet in @p buf
 *
 * Layout: [ ESP hdr(8) ][ transport(tlen) ][ pad(pad_len) ][ pad_len(1) ]
 *         [ next_header(1) ][ ICV(icv_len) ]
 *
 * @return total ESP length written
 */
static uint32_t
build_esp(u_char *buf, uint8_t next_header, const u_char *transport,
          uint32_t tlen, uint32_t pad_len, uint32_t icv_len)
{
    uint32_t i;
    uint32_t off;

    /* ESP header (SPI + Sequence Number), arbitrary */
    put16(buf + 0, 0xdead);
    put16(buf + 2, 0xbeef);
    put16(buf + 4, 0x0000);
    put16(buf + 6, 0x0001);

    memcpy(buf + 8, transport, tlen);

    off = 8 + tlen;
    for (i = 0; i < pad_len; i++)
        buf[off + i] = (u_char)(i + 1); /* self-describing padding 1,2,3,... */
    buf[off + pad_len] = (u_char)pad_len;
    buf[off + pad_len + 1] = next_header;
    memset(buf + off + pad_len + 2, 0x5A, icv_len); /* fake ICV */

    return 8 + tlen + pad_len + 2 + icv_len;
}

/**
 * Inner UDP datagram inside ESP-NULL with a 12-byte ICV.
 */
static void
test_udp(void)
{
    const char *sip = "OPTIONS sip:test@example.com SIP/2.0\r\n\r\n";
    uint16_t siplen = (uint16_t)strlen(sip);
    u_char udp[8 + 256];
    u_char buf[512];
    uint32_t tlen = 8 + siplen;
    uint32_t esp_len;
    uint8_t proto;
    uint16_t sport, dport;
    uint32_t poff, plen;

    put16(udp + 0, 5060);           /* source port */
    put16(udp + 2, 5061);           /* destination port */
    put16(udp + 4, (uint16_t)tlen); /* UDP length = header + payload */
    put16(udp + 6, 0x0000);         /* checksum */
    memcpy(udp + 8, sip, siplen);

    esp_len = build_esp(buf, IPPROTO_UDP, udp, tlen, 2, 12);

    assert(capture_esp_null_parse(buf, esp_len, &proto, &sport, &dport,
                                  &poff, &plen) == 0);
    assert(proto == IPPROTO_UDP);
    assert(sport == 5060);
    assert(dport == 5061);
    assert(poff == 8 + 8);
    assert(plen == siplen); /* trailer + ICV stripped exactly */
    assert(memcmp(buf + poff, sip, siplen) == 0);

    printf("test_udp: OK\n");
}

/**
 * Inner TCP segment inside ESP-NULL with a 12-byte ICV. The trailer must be
 * stripped exactly so the payload length excludes the ICV.
 */
static void
test_tcp(void)
{
    const char *sip = "INVITE sip:test@example.com SIP/2.0\r\n\r\n";
    uint16_t siplen = (uint16_t)strlen(sip);
    u_char tcp[20 + 256];
    u_char buf[512];
    uint32_t tlen = 20 + siplen;
    uint32_t esp_len;
    uint8_t proto;
    uint16_t sport, dport;
    uint32_t poff, plen;

    memset(tcp, 0, sizeof(tcp));
    put16(tcp + 0, 5064); /* source port */
    put16(tcp + 2, 6060); /* destination port */
    tcp[12] = (5 << 4);   /* data offset = 5 words (20 bytes) */
    memcpy(tcp + 20, sip, siplen);

    esp_len = build_esp(buf, IPPROTO_TCP, tcp, tlen, 2, 12);

    assert(capture_esp_null_parse(buf, esp_len, &proto, &sport, &dport,
                                  &poff, &plen) == 0);
    assert(proto == IPPROTO_TCP);
    assert(sport == 5064);
    assert(dport == 6060);
    assert(poff == 8 + 20);
    assert(plen == siplen); /* ICV must NOT leak into the TCP stream */
    assert(memcmp(buf + poff, sip, siplen) == 0);

    printf("test_tcp: OK\n");
}

/**
 * ICV-length auto-detection: a 16-byte ICV with zero padding must still be
 * decoded correctly.
 */
static void
test_icv_autodetect(void)
{
    const char *sip = "BYE sip:test@example.com SIP/2.0\r\n\r\n";
    uint16_t siplen = (uint16_t)strlen(sip);
    u_char tcp[20 + 256];
    u_char buf[512];
    uint32_t tlen = 20 + siplen;
    uint32_t esp_len;
    uint8_t proto;
    uint16_t sport, dport;
    uint32_t poff, plen;

    memset(tcp, 0, sizeof(tcp));
    put16(tcp + 0, 5062);
    put16(tcp + 2, 6080);
    tcp[12] = (5 << 4);
    memcpy(tcp + 20, sip, siplen);

    esp_len = build_esp(buf, IPPROTO_TCP, tcp, tlen, 0, 16); /* pad 0, ICV 16 */

    assert(capture_esp_null_parse(buf, esp_len, &proto, &sport, &dport,
                                  &poff, &plen) == 0);
    assert(proto == IPPROTO_TCP);
    assert(sport == 5062);
    assert(dport == 6080);
    assert(poff == 8 + 20);
    assert(plen == siplen);

    printf("test_icv_autodetect: OK\n");
}

/**
 * Rejection cases: too-short buffers, and payloads whose trailer does not
 * validate (no plausible next header / broken padding pattern).
 */
static void
test_reject(void)
{
    u_char buf[128];
    u_char tcp[64];
    uint32_t esp_len;
    uint8_t proto;
    uint16_t sport, dport;
    uint32_t poff, plen;

    /* NULL buffer */
    assert(capture_esp_null_parse(NULL, 64, &proto, &sport, &dport,
                                  &poff, &plen) == -1);

    /* Too short: not even room for ESP header + UDP header */
    memset(buf, 0, sizeof(buf));
    assert(capture_esp_null_parse(buf, 8 + 4, &proto, &sport, &dport,
                                  &poff, &plen) == -1);

    /* All-zero payload: no valid next header (0) at any ICV offset */
    memset(buf, 0, sizeof(buf));
    assert(capture_esp_null_parse(buf, sizeof(buf), &proto, &sport, &dport,
                                  &poff, &plen) == -1);

    /* Valid TCP header + next_header but broken padding pattern */
    memset(tcp, 0, sizeof(tcp));
    put16(tcp + 0, 5064);
    put16(tcp + 2, 6060);
    tcp[12] = (5 << 4);
    esp_len = build_esp(buf, IPPROTO_TCP, tcp, 20 + 8, 3, 12);
    buf[8 + 20 + 8] = 0x77; /* corrupt first padding byte (should be 0x01) */
    assert(capture_esp_null_parse(buf, esp_len, &proto, &sport, &dport,
                                  &poff, &plen) == -1);

    printf("test_reject: OK\n");
}

int main(void)
{
    test_udp();
    test_tcp();
    test_icv_autodetect();
    test_reject();
    printf("All ESP-NULL decoder tests passed\n");
    return 0;
}
