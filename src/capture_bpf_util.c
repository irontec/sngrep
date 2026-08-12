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
 * @file capture_bpf_util.c
 * @author Jurijs Ivolga <jurijs.ivolga@gmail.com>
 *
 * @brief Pure helpers for eBPF TLS capture
 *
 */
#include "config.h"

// struct tcphdr member names below are the BSD spelling
#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif

#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "capture_bpf_util.h"
#include "util.h"

int
capture_bpf_maps_line(const char *line, char *path, size_t pathlen)
{
    const char *p, *base;
    size_t len;

    if (!line || !path || pathlen == 0)
        return 0;

    // The path is the sixth field, and is absent for anonymous mappings.
    // Special regions such as [stack] contain no slash, so the first slash
    // reliably starts a real path.
    if (!(p = strchr(line, '/')))
        return 0;

    // Trim the trailing newline
    len = strcspn(p, "\n");
    if (len == 0 || len >= pathlen)
        return 0;

    // A mapping of an unlinked file can not be probed
    if (len > 10 && strncmp(p + len - 10, " (deleted)", 10) == 0)
        return 0;

    if (!(base = strrchr(p, '/')))
        return 0;
    base++;

    if (strncmp(base, "libssl.so", 9) != 0)
        return 0;

    memcpy(path, p, len);
    path[len] = '\0';
    return 1;
}

int
capture_bpf_paths_add(vector_t *paths, const char *path)
{
    struct stat st, other;
    vector_iter_t it;
    char *copy;

    if (stat(path, &st) != 0)
        return 0;

    // Reject anything already present, even under a different name
    it = vector_iterator(paths);
    while ((copy = vector_iterator_next(&it))) {
        if (stat(copy, &other) == 0
            && other.st_dev == st.st_dev && other.st_ino == st.st_ino)
            return 0;
    }

    if (!(copy = sng_malloc(strlen(path) + 1)))
        return 0;
    strcpy(copy, path);
    vector_append(paths, copy);

    return 1;
}

vector_t *
capture_bpf_find_libssl(const char *procroot)
{
    vector_t *paths;
    DIR *proc;
    struct dirent *ent;
    char mapspath[PATH_MAX], line[PATH_MAX + 128];
    char libpath[PATH_MAX], rooted[PATH_MAX];
    pid_t self = getpid();

    paths = vector_create(0, 8);
    vector_set_destroyer(paths, vector_generic_destroyer);

    if (!(proc = opendir(procroot)))
        return paths;

    while ((ent = readdir(proc))) {
        FILE *f;
        long pid;
        char *end;

        // Only numeric entries are processes
        pid = strtol(ent->d_name, &end, 10);
        if (*end != '\0' || pid <= 0 || (pid_t) pid == self)
            continue;

        snprintf(mapspath, sizeof(mapspath), "%s/%ld/maps", procroot, pid);
        if (!(f = fopen(mapspath, "r")))
            continue;

        while (fgets(line, sizeof(line), f)) {
            if (!capture_bpf_maps_line(line, libpath, sizeof(libpath)))
                continue;

            // Resolve through the process root so libraries inside container
            // mount namespaces are reachable from ours. Fall back to the bare
            // path when the process is in our own namespace.
            snprintf(rooted, sizeof(rooted), "%s/%ld/root%s",
                     procroot, pid, libpath);

            if (!capture_bpf_paths_add(paths, rooted))
                capture_bpf_paths_add(paths, libpath);
        }
        fclose(f);
    }

    closedir(proc);
    return paths;
}

//! Sequence counter for one direction of one connection
struct capture_bpf_seq {
    //! Direction key as built by capture_bpf_conn_key()
    char     key[CAPTURE_BPF_KEYLEN];
    //! Sequence number the next segment will carry
    uint32_t next;
};

//! Bounded, insertion ordered table of sequence counters
struct capture_bpf_seqtab {
    struct capture_bpf_seq *ent;
    size_t count;
    size_t max;
};

void
capture_bpf_conn_key(char *buf, size_t buflen, uint8_t family,
                     const uint8_t *saddr, uint16_t sport,
                     const uint8_t *daddr, uint16_t dport)
{
    char s[INET6_ADDRSTRLEN] = { 0 }, d[INET6_ADDRSTRLEN] = { 0 };
    int af = (family == 6) ? AF_INET6 : AF_INET;

    inet_ntop(af, saddr, s, sizeof(s));
    inet_ntop(af, daddr, d, sizeof(d));
    snprintf(buf, buflen, "%s:%u>%s:%u", s, sport, d, dport);
}

capture_bpf_seqtab_t *
capture_bpf_seqtab_create(size_t max_entries)
{
    capture_bpf_seqtab_t *t;

    if (max_entries == 0)
        return NULL;

    if (!(t = sng_malloc(sizeof(capture_bpf_seqtab_t))))
        return NULL;

    if (!(t->ent = sng_malloc(sizeof(struct capture_bpf_seq) * max_entries))) {
        sng_free(t);
        return NULL;
    }

    t->count = 0;
    t->max = max_entries;
    return t;
}

void
capture_bpf_seqtab_destroy(capture_bpf_seqtab_t *t)
{
    if (!t)
        return;
    sng_free(t->ent);
    sng_free(t);
}

size_t
capture_bpf_seqtab_count(capture_bpf_seqtab_t *t)
{
    return t ? t->count : 0;
}

/**
 * @brief Locate an entry by key, or NULL
 *
 * A linear scan is fine at these table sizes, and keeps the table compact
 * enough that eviction is a single memmove.
 */
static struct capture_bpf_seq *
seqtab_find(capture_bpf_seqtab_t *t, const char *key)
{
    size_t i;

    for (i = 0; i < t->count; i++)
        if (strcmp(t->ent[i].key, key) == 0)
            return &t->ent[i];

    return NULL;
}

/**
 * @brief Append an entry, evicting the oldest once the table is full
 */
static struct capture_bpf_seq *
seqtab_insert(capture_bpf_seqtab_t *t, const char *key)
{
    struct capture_bpf_seq *e;

    if (t->count == t->max) {
        // Drop the oldest entry and shift the remainder down
        memmove(&t->ent[0], &t->ent[1],
                sizeof(struct capture_bpf_seq) * (t->max - 1));
        t->count--;
    }

    e = &t->ent[t->count++];
    sng_strncpy(e->key, key, sizeof(e->key) - 1);
    e->next = 1;
    return e;
}

uint32_t
capture_bpf_seqtab_next(capture_bpf_seqtab_t *t, const char *key,
                        const char *revkey, uint32_t len, uint32_t *ack)
{
    struct capture_bpf_seq *fwd, *rev;
    uint32_t seq;

    if (!(fwd = seqtab_find(t, key)))
        fwd = seqtab_insert(t, key);

    seq = fwd->next;
    fwd->next += len;

    if (ack) {
        // Acknowledge everything the opposite direction has sent so far
        rev = seqtab_find(t, revkey);
        *ack = rev ? rev->next : 1;
    }

    return seq;
}

size_t
capture_bpf_build_frame(u_char *buf, size_t buflen, uint8_t family,
                        const uint8_t *saddr, uint16_t sport,
                        const uint8_t *daddr, uint16_t dport,
                        uint32_t seq, uint32_t ack, bool push,
                        const u_char *payload, uint32_t payload_len)
{
    size_t iphdr_len = (family == 6) ? sizeof(struct ip6_hdr) : sizeof(struct ip);
    size_t total = sizeof(struct ether_header) + iphdr_len
                   + sizeof(struct tcphdr) + payload_len;
    size_t off = 0;
    struct tcphdr tcp = { 0 };

    if (total > buflen)
        return 0;

    // Ethernet, using the placeholder addresses the EEP source already uses
    struct ether_header eth = {
        .ether_dhost = { 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB },
        .ether_shost = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA },
        .ether_type  = htons((family == 6) ? ETHERTYPE_IPV6 : ETHERTYPE_IP),
    };
    memcpy(buf + off, &eth, sizeof(eth));
    off += sizeof(eth);

    if (family == 6) {
        struct ip6_hdr ip6 = { 0 };
        ip6.ip6_vfc  = 0x60;
        ip6.ip6_plen = htons((uint16_t) (sizeof(struct tcphdr) + payload_len));
        ip6.ip6_nxt  = IPPROTO_TCP;
        ip6.ip6_hlim = 128;
        memcpy(&ip6.ip6_src, saddr, 16);
        memcpy(&ip6.ip6_dst, daddr, 16);
        memcpy(buf + off, &ip6, sizeof(ip6));
        off += sizeof(ip6);
    } else {
        struct ip ip4 = { 0 };
        ip4.ip_v   = 4;
        ip4.ip_hl  = sizeof(struct ip) / 4;
        ip4.ip_len = htons((uint16_t) (sizeof(struct ip)
                                       + sizeof(struct tcphdr) + payload_len));
        ip4.ip_ttl = 128;
        ip4.ip_p   = IPPROTO_TCP;
        memcpy(&ip4.ip_src, saddr, 4);
        memcpy(&ip4.ip_dst, daddr, 4);
        memcpy(buf + off, &ip4, sizeof(ip4));
        off += sizeof(ip4);
    }

    tcp.th_sport = htons(sport);
    tcp.th_dport = htons(dport);
    tcp.th_seq   = htonl(seq);
    tcp.th_ack   = htonl(ack);
    tcp.th_off   = sizeof(struct tcphdr) / 4;
    // PSH marks the final chunk of a message; capture_packet_reasm_tcp()
    // flushes on it, so continuation chunks must leave it clear
    tcp.th_flags = TH_ACK | (push ? TH_PUSH : 0);
    tcp.th_win   = htons(65535);
    memcpy(buf + off, &tcp, sizeof(tcp));
    off += sizeof(tcp);

    if (payload_len)
        memcpy(buf + off, payload, payload_len);

    return total;
}
