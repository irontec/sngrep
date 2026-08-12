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
 * @file test_014.c
 * @brief Unit tests for the eBPF in-kernel SIP/WebSocket prefilter
 *
 * The prefilter runs in BPF context to discard non-SIP TLS traffic before it
 * costs a ring buffer round trip. It is compiled from the same header the BPF
 * program uses, so this test also guards against the two drifting apart.
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../src/bpf/sngrep_tls.h"

//! Convenience wrapper so cases read as plain strings
static int
match(const char *s)
{
    return sngrep_tls_looks_like_sip((const unsigned char *) s,
                                     (unsigned int) strlen(s));
}

static void
test_requests(void)
{
    assert(match("INVITE sip:bob@example.net SIP/2.0\r\n"));
    assert(match("REGISTER sip:example.net SIP/2.0\r\n"));
    assert(match("OPTIONS sip:example.net SIP/2.0\r\n"));
    assert(match("SUBSCRIBE sip:bob@example.net SIP/2.0\r\n"));
    assert(match("MESSAGE sip:bob@example.net SIP/2.0\r\n"));
    assert(match("PUBLISH sip:bob@example.net SIP/2.0\r\n"));
    assert(match("CANCEL sip:bob@example.net SIP/2.0\r\n"));
    assert(match("NOTIFY sip:bob@example.net SIP/2.0\r\n"));
    assert(match("UPDATE sip:bob@example.net SIP/2.0\r\n"));
    assert(match("PRACK sip:bob@example.net SIP/2.0\r\n"));
    assert(match("REFER sip:bob@example.net SIP/2.0\r\n"));
    assert(match("INFO sip:bob@example.net SIP/2.0\r\n"));
    assert(match("BYE sip:bob@example.net SIP/2.0\r\n"));
    assert(match("ACK sip:bob@example.net SIP/2.0\r\n"));
    printf("test_requests: OK\n");
}

static void
test_responses(void)
{
    assert(match("SIP/2.0 200 OK\r\n"));
    assert(match("SIP/2.0 100 Trying\r\n"));
    assert(match("SIP/2.0 486 Busy Here\r\n"));
    printf("test_responses: OK\n");
}

static void
test_websocket(void)
{
    /* SIP over WSS: the plaintext is a WebSocket frame, not SIP text.
     * FIN + text opcode, and FIN + binary opcode. */
    const unsigned char text[]   = { 0x81, 0x05, 'h', 'e', 'l', 'l', 'o' };
    const unsigned char binary[] = { 0x82, 0x02, 0x00, 0x01 };

    assert(sngrep_tls_looks_like_sip(text, sizeof(text)));
    assert(sngrep_tls_looks_like_sip(binary, sizeof(binary)));
    printf("test_websocket: OK\n");
}

static void
test_reject(void)
{
    /* Plain HTTP over TLS, the dominant source of noise */
    assert(!match("GET /index.html HTTP/1.1\r\n"));
    assert(!match("POST /api/v1 HTTP/1.1\r\n"));
    assert(!match("HTTP/1.1 200 OK\r\n"));

    /* Prefixes of SIP methods that are not actually SIP */
    assert(!match("INVITED"));
    assert(!match("ACKNOWLEDGE"));
    assert(!match("SIP/1.0 200 OK\r\n"));

    /* Too short to classify */
    assert(!match(""));
    assert(!match("IN"));
    assert(!match("BYE"));

    /* Binary junk, such as a TLS record header leaking through */
    const unsigned char junk[] = { 0x17, 0x03, 0x03, 0x00, 0x40 };
    assert(!sngrep_tls_looks_like_sip(junk, sizeof(junk)));
    printf("test_reject: OK\n");
}

int main(void)
{
    test_requests();
    test_responses();
    test_websocket();
    test_reject();
    printf("All prefilter tests passed\n");
    return 0;
}
