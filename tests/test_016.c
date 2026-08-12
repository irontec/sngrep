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
 * @file test_016.c
 * @brief Unit tests for eBPF synthetic frame construction and TCP sequencing
 *
 * eBPF sourced plaintext is wrapped in a synthetic Ethernet/IP/TCP frame and
 * fed to sngrep's ordinary parse_packet() path, so the frame has to be byte
 * correct and the sequence numbers monotonic per direction for
 * capture_packet_reasm_tcp() to reassemble it.
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include "../src/capture_bpf_util.h"

static const uint8_t V4_SRC[16] = { 10, 0, 0, 1 };
static const uint8_t V4_DST[16] = { 10, 0, 0, 2 };

static void
test_conn_key(void)
{
    char a[CAPTURE_BPF_KEYLEN], b[CAPTURE_BPF_KEYLEN];
    char again[CAPTURE_BPF_KEYLEN];

    capture_bpf_conn_key(a, sizeof(a), 4, V4_SRC, 5061, V4_DST, 39000);
    capture_bpf_conn_key(b, sizeof(b), 4, V4_DST, 39000, V4_SRC, 5061);

    /* Direction matters: the two halves of a connection are distinct keys */
    assert(strcmp(a, b) != 0);

    /* The same inputs must always produce the same key */
    capture_bpf_conn_key(again, sizeof(again), 4, V4_SRC, 5061, V4_DST, 39000);
    assert(strcmp(a, again) == 0);

    printf("test_conn_key: OK\n");
}

static void
test_conn_key_worst_case(void)
{
    /* The longest form inet_ntop actually produces, eight full hex groups.
     * Two connections differing only in the final port must still produce
     * distinct keys, because the port is at the end and a key buffer one byte
     * too short would truncate exactly there and merge them. */
    static const uint8_t a[16] = {
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,255,255,255,255
    };
    static const uint8_t b[16] = {
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,255,255,255,254
    };
    char k1[CAPTURE_BPF_KEYLEN], k2[CAPTURE_BPF_KEYLEN];

    capture_bpf_conn_key(k1, sizeof(k1), 6, a, 65535, b, 65535);
    capture_bpf_conn_key(k2, sizeof(k2), 6, a, 65535, b, 65534);

    /* Nothing was cut off the end */
    assert(strlen(k1) < CAPTURE_BPF_KEYLEN - 1);
    assert(strcmp(k1, k2) != 0);

    printf("test_conn_key_worst_case: OK\n");
}

static void
test_seqtab_production_size(void)
{
    /* sng_malloc refuses allocations over MALLOC_MAX_SIZE, so the table
     * sngrep actually asks for at runtime has to be constructible. */
    capture_bpf_seqtab_t *t = capture_bpf_seqtab_create(CAPTURE_BPF_MAX_CONNS);

    assert(t != NULL);
    capture_bpf_seqtab_destroy(t);
    printf("test_seqtab_production_size: OK\n");
}

static void
test_seq_monotonic(void)
{
    capture_bpf_seqtab_t *t = capture_bpf_seqtab_create(16);
    uint32_t ack = 0;

    /* The first segment of a direction starts at 1 */
    assert(capture_bpf_seqtab_next(t, "a>b", "b>a", 100, &ack) == 1);
    /* and advances by the payload length */
    assert(capture_bpf_seqtab_next(t, "a>b", "b>a", 50, &ack) == 101);
    assert(capture_bpf_seqtab_next(t, "a>b", "b>a", 1, &ack) == 151);

    /* The reverse direction is counted independently */
    assert(capture_bpf_seqtab_next(t, "b>a", "a>b", 10, &ack) == 1);
    /* and acknowledges everything the forward direction has sent */
    assert(ack == 152);

    capture_bpf_seqtab_destroy(t);
    printf("test_seq_monotonic: OK\n");
}

static void
test_seq_eviction(void)
{
    capture_bpf_seqtab_t *t = capture_bpf_seqtab_create(4);
    char key[32];
    uint32_t ack = 0;
    int i;

    /* Overfill well past the cap. The table has to stay bounded rather than
     * growing without limit on a busy proxy. */
    for (i = 0; i < 100; i++) {
        snprintf(key, sizeof(key), "conn%d", i);
        capture_bpf_seqtab_next(t, key, "rev", 10, &ack);
    }
    assert(capture_bpf_seqtab_count(t) <= 4);

    /* A surviving entry still counts correctly */
    assert(capture_bpf_seqtab_next(t, "conn99", "rev", 5, &ack) == 11);

    capture_bpf_seqtab_destroy(t);
    printf("test_seq_eviction: OK\n");
}

static void
test_frame_ipv4(void)
{
    u_char buf[CAPTURE_BPF_MAX_FRAME];
    const char *sip = "SIP/2.0 200 OK\r\n\r\n";
    uint32_t plen = (uint32_t) strlen(sip);
    struct ether_header *eth;
    struct ip *ip;
    struct tcphdr *tcp;
    size_t n;

    n = capture_bpf_build_frame(buf, sizeof(buf), 4,
                                V4_SRC, 5061, V4_DST, 39000,
                                1000, 2000, true,
                                (const u_char *) sip, plen);

    assert(n == sizeof(struct ether_header) + sizeof(struct ip)
                + sizeof(struct tcphdr) + plen);

    eth = (struct ether_header *) buf;
    assert(ntohs(eth->ether_type) == ETHERTYPE_IP);

    ip = (struct ip *) (buf + sizeof(struct ether_header));
    assert(ip->ip_v == 4);
    assert(ip->ip_hl == 5);
    assert(ip->ip_p == IPPROTO_TCP);
    assert(ntohs(ip->ip_len) == sizeof(struct ip) + sizeof(struct tcphdr) + plen);
    assert(memcmp(&ip->ip_src, V4_SRC, 4) == 0);
    assert(memcmp(&ip->ip_dst, V4_DST, 4) == 0);

    tcp = (struct tcphdr *)
        (buf + sizeof(struct ether_header) + sizeof(struct ip));
    assert(ntohs(tcp->th_sport) == 5061);
    assert(ntohs(tcp->th_dport) == 39000);
    assert(ntohl(tcp->th_seq) == 1000);
    assert(ntohl(tcp->th_ack) == 2000);
    assert(tcp->th_off == 5);
    assert(tcp->th_flags & TH_ACK);
    assert(tcp->th_flags & TH_PUSH);

    /* Payload lands immediately after the TCP header, unmodified */
    assert(memcmp(buf + n - plen, sip, plen) == 0);

    printf("test_frame_ipv4: OK\n");
}

static void
test_frame_no_push(void)
{
    u_char buf[CAPTURE_BPF_MAX_FRAME];
    const char *chunk = "INVITE sip:a@b SIP/2.0\r\n";
    uint32_t plen = (uint32_t) strlen(chunk);
    struct tcphdr *tcp;

    capture_bpf_build_frame(buf, sizeof(buf), 4, V4_SRC, 5061, V4_DST, 39000,
                            1, 1, false, (const u_char *) chunk, plen);

    tcp = (struct tcphdr *)
        (buf + sizeof(struct ether_header) + sizeof(struct ip));
    /* Continuation chunks must not set PSH. capture_packet_reasm_tcp()
     * flushes non-SIP payload on PSH, which would truncate the message. */
    assert(!(tcp->th_flags & TH_PUSH));
    assert(tcp->th_flags & TH_ACK);

    printf("test_frame_no_push: OK\n");
}

static void
test_frame_ipv6(void)
{
    u_char buf[CAPTURE_BPF_MAX_FRAME];
    uint8_t s6[16] = { 0x20,0x01,0x0d,0xb8,0,0,0,0,0,0,0,0,0,0,0,0x01 };
    uint8_t d6[16] = { 0x20,0x01,0x0d,0xb8,0,0,0,0,0,0,0,0,0,0,0,0x02 };
    const char *sip = "BYE sip:a@b SIP/2.0\r\n\r\n";
    uint32_t plen = (uint32_t) strlen(sip);
    struct ether_header *eth;
    struct ip6_hdr *ip6;
    size_t n;

    n = capture_bpf_build_frame(buf, sizeof(buf), 6, s6, 5061, d6, 39000,
                                7, 9, true, (const u_char *) sip, plen);

    assert(n == sizeof(struct ether_header) + sizeof(struct ip6_hdr)
                + sizeof(struct tcphdr) + plen);

    eth = (struct ether_header *) buf;
    assert(ntohs(eth->ether_type) == ETHERTYPE_IPV6);

    ip6 = (struct ip6_hdr *) (buf + sizeof(struct ether_header));
    assert((ip6->ip6_vfc >> 4) == 6);
    assert(ip6->ip6_nxt == IPPROTO_TCP);
    assert(ntohs(ip6->ip6_plen) == sizeof(struct tcphdr) + plen);
    assert(memcmp(&ip6->ip6_src, s6, 16) == 0);
    assert(memcmp(&ip6->ip6_dst, d6, 16) == 0);

    printf("test_frame_ipv6: OK\n");
}

static void
test_frame_rejects_overflow(void)
{
    u_char small[64];
    const char *sip = "SIP/2.0 200 OK\r\n\r\n";

    /* Refuses to write past the caller's buffer rather than corrupting it */
    assert(capture_bpf_build_frame(small, sizeof(small), 4,
                                   V4_SRC, 5061, V4_DST, 39000, 1, 1, true,
                                   (const u_char *) sip,
                                   (uint32_t) strlen(sip)) == 0);
    printf("test_frame_rejects_overflow: OK\n");
}

int main(void)
{
    test_conn_key();
    test_conn_key_worst_case();
    test_seqtab_production_size();
    test_seq_monotonic();
    test_seq_eviction();
    test_frame_ipv4();
    test_frame_no_push();
    test_frame_ipv6();
    test_frame_rejects_overflow();
    printf("All frame synthesis tests passed\n");
    return 0;
}
