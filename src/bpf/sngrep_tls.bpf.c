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
 * @file sngrep_tls.bpf.c
 * @author Jurijs Ivolga <jurijs.ivolga@gmail.com>
 *
 * @brief eBPF programs capturing SIP over TLS plaintext
 *
 * uprobes on OpenSSL's read and write entry points recover plaintext before
 * it is encrypted and after it is decrypted, so no private key or certificate
 * is needed.
 *
 * Connection addresses do not come from the SSL object, because applications
 * that give OpenSSL a memory BIO -- Kamailio among them -- keep no socket
 * there at all. They are read from struct sock in tcp_sendmsg and tcp_recvmsg
 * instead, and matched to the plaintext per thread. That works for both
 * styles of application, because of when the send happens relative to the
 * SSL_write call:
 *
 *   BIO native (Asterisk)      tcp_sendmsg runs inside SSL_write
 *   memory BIO (Kamailio)      tcp_sendmsg runs just after SSL_write returns
 *
 * Either way it is the next tcp_sendmsg on the same thread, so egress is
 * emitted from there. For ingress the tcp_recvmsg always precedes SSL_read
 * returning, so the last socket seen on the thread is the right one.
 *
 */

/*
 * struct pt_regs, which bpf_tracing.h needs to reach probe arguments, normally
 * comes from a generated vmlinux.h. This build deliberately avoids that file
 * so the build host does not need BTF, and takes the UAPI definition instead.
 */
#include <linux/ptrace.h>
#include <linux/bpf.h>
#include <linux/types.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_endian.h>

//! The verifier requires the shared prefilter to be fully inlined
#define SNGREP_TLS_INLINE static __always_inline
#include "sngrep_tls.h"

char LICENSE[] SEC("license") = "GPL";

#define AF_INET  2
#define AF_INET6 10

/*
 * Set from userspace before the programs are loaded. Turning the prefilter
 * off sends every TLS payload on the machine to sngrep, which is only useful
 * when working out why an expected message was not captured.
 */
const volatile __u8 sngrep_filter_enabled = 1;

/*
 * Only the handful of kernel fields actually read are declared here, rather
 * than pulling in a generated vmlinux.h. preserve_access_index makes CO-RE
 * relocate the offsets against the running kernel, so BTF is required at run
 * time but not at build time.
 */
struct in6_addr {
    union {
        __u8 u6_addr8[16];
    } in6_u;
} __attribute__((preserve_access_index));

struct sock_common {
    union {
        __be32 skc_daddr;
    };
    union {
        __be32 skc_rcv_saddr;
    };
    unsigned short skc_family;
    union {
        struct {
            __be16 skc_dport;
            __u16  skc_num;
        };
    };
    struct in6_addr skc_v6_daddr;
    struct in6_addr skc_v6_rcv_saddr;
} __attribute__((preserve_access_index));

struct sock {
    struct sock_common __sk_common;
} __attribute__((preserve_access_index));

//! One direction of a TCP connection as the kernel sees it
struct conn_tuple {
    //! 4 or 6
    __u8  family;
    //! Local end
    __u8  saddr[16];
    //! Peer end
    __u8  daddr[16];
    __u16 sport;
    __u16 dport;
};

//! Plaintext waiting for the send that will reveal where it is going
struct pending_write {
    __u32 len;
    __u8  data[SNGREP_TLS_MAX_MSG];
};

//! Buffer handed to SSL_read, bridged from probe entry to probe return
struct active_read {
    //! User space address of the caller's buffer
    __u64 buf;
    //! Where SSL_read_ex reports the length, zero for plain SSL_read
    __u64 readbytes_ptr;
};

//! Reads in flight, keyed by thread
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 512);
    __type(key, __u64);
    __type(value, struct active_read);
} active_reads SEC(".maps");

//! Last socket each thread touched
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 4096);
    __type(key, __u64);
    __type(value, struct conn_tuple);
} tid_tuple SEC(".maps");

/*
 * Plaintext written but not yet sent, keyed by thread. Only threads with a
 * write in flight hold an entry, so this needs to cover the daemon's worker
 * count rather than its connection count.
 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 128);
    __type(key, __u64);
    __type(value, struct pending_write);
} pending_writes SEC(".maps");

//! Captured plaintext on its way to sngrep
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 8 * 1024 * 1024);
} events SEC(".maps");

//! Event assembly area; struct tls_event is far too large for the BPF stack
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct tls_event);
} scratch SEC(".maps");

//! Staging area for a pending write, for the same reason
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct pending_write);
} write_scratch SEC(".maps");

//! Counters for plaintext that had to be dropped
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, SNGREP_TLS_STAT_NR);
    __type(key, __u32);
    __type(value, __u64);
} stats SEC(".maps");

/**
 * @brief Count one dropped payload
 */
static __always_inline void
sngrep_tls_stat(__u32 idx)
{
    __u64 *v = bpf_map_lookup_elem(&stats, &idx);

    if (v)
        (*v)++;
}

/**
 * @brief Is @a an IPv4 address carried in an IPv6 one, ::ffff:0:0/96
 */
static __always_inline int
sngrep_tls_v4mapped(const __u8 *a)
{
    int i;

    for (i = 0; i < 10; i++)
        if (a[i] != 0)
            return 0;

    return a[10] == 0xff && a[11] == 0xff;
}

/**
 * @brief Read the local and peer addresses out of a struct sock
 *
 * A socket bound to :: accepts IPv4 connections as IPv4 mapped IPv6, which is
 * the default for every common SIP daemon. Those are reported as plain IPv4,
 * so both ends of a connection agree and the addresses match what a capture
 * from the wire would show.
 *
 * @return 1 if the address family is one we handle, 0 otherwise
 */
static __always_inline int
sngrep_tls_tuple(struct sock *sk, struct conn_tuple *t)
{
    __u16 family = BPF_CORE_READ(sk, __sk_common.skc_family);

    __builtin_memset(t, 0, sizeof(*t));

    if (family == AF_INET) {
        t->family = 4;
        BPF_CORE_READ_INTO(&t->saddr, sk, __sk_common.skc_rcv_saddr);
        BPF_CORE_READ_INTO(&t->daddr, sk, __sk_common.skc_daddr);
    } else if (family == AF_INET6) {
        t->family = 6;
        BPF_CORE_READ_INTO(&t->saddr, sk, __sk_common.skc_v6_rcv_saddr);
        BPF_CORE_READ_INTO(&t->daddr, sk, __sk_common.skc_v6_daddr);

        if (sngrep_tls_v4mapped(t->saddr) && sngrep_tls_v4mapped(t->daddr)) {
            t->family = 4;
            __builtin_memcpy(t->saddr, t->saddr + 12, 4);
            __builtin_memcpy(t->daddr, t->daddr + 12, 4);
            __builtin_memset(t->saddr + 4, 0, 12);
            __builtin_memset(t->daddr + 4, 0, 12);
        }
    } else {
        return 0;
    }

    // skc_num is already in host order, skc_dport is not
    t->sport = BPF_CORE_READ(sk, __sk_common.skc_num);
    t->dport = bpf_ntohs(BPF_CORE_READ(sk, __sk_common.skc_dport));
    return 1;
}

/**
 * @brief Publish @len bytes already staged in @e->data
 *
 * For egress the local end is the source, for ingress the peer is, so the
 * call flow in sngrep points the right way.
 */
static __always_inline void
sngrep_tls_emit(struct tls_event *e, __u32 len, __u8 dir, __u8 more,
                const struct conn_tuple *t)
{
    __u64 id = bpf_get_current_pid_tgid();

    if (len == 0 || len > SNGREP_TLS_MAX_CHUNK)
        return;

    e->ts_ns     = bpf_ktime_get_boot_ns();
    e->pid       = id >> 32;
    e->tid       = (__u32) id;
    e->direction = dir;
    e->more      = more;
    e->pad       = 0;
    e->len       = len;
    e->family    = t->family;

    if (dir == SNGREP_TLS_EGRESS) {
        __builtin_memcpy(e->saddr, t->saddr, 16);
        __builtin_memcpy(e->daddr, t->daddr, 16);
        e->sport = t->sport;
        e->dport = t->dport;
    } else {
        __builtin_memcpy(e->saddr, t->daddr, 16);
        __builtin_memcpy(e->daddr, t->saddr, 16);
        e->sport = t->dport;
        e->dport = t->sport;
    }

    bpf_get_current_comm(&e->comm, sizeof(e->comm));

    if (bpf_ringbuf_output(&events, e, SNGREP_TLS_EVENT_HDRLEN + len, 0) != 0)
        sngrep_tls_stat(SNGREP_TLS_STAT_RINGBUF_FULL);
}

/**
 * @brief Publish the plaintext this thread wrote but had not yet sent
 *
 * The copy goes through a helper rather than a builtin so its length can vary
 * without unrolling eight kilobytes of loads and stores.
 */
static __always_inline void
sngrep_tls_flush_pending(__u64 id, const struct conn_tuple *t)
{
    struct pending_write *pw;
    struct tls_event *e;
    __u32 zero = 0, total, off, n;
    int i;

    if (!(pw = bpf_map_lookup_elem(&pending_writes, &id)))
        return;

    total = pw->len;
    if (total > SNGREP_TLS_MAX_MSG)
        total = SNGREP_TLS_MAX_MSG;

    // A message longer than one chunk becomes consecutive events, which
    // userspace turns into consecutive TCP segments for sngrep to reassemble
    #pragma unroll
    for (i = 0; i < SNGREP_TLS_MAX_CHUNKS; i++) {
        off = (__u32) i * SNGREP_TLS_MAX_CHUNK;
        if (off >= total)
            break;

        n = total - off;
        if (n > SNGREP_TLS_MAX_CHUNK)
            n = SNGREP_TLS_MAX_CHUNK;

        if (!(e = bpf_map_lookup_elem(&scratch, &zero)))
            break;
        if (bpf_probe_read_kernel(e->data, n, pw->data + off) != 0) {
            sngrep_tls_stat(SNGREP_TLS_STAT_READ_FAILED);
            break;
        }

        sngrep_tls_emit(e, n, SNGREP_TLS_EGRESS,
                        (off + n < total) ? 1 : 0, t);
    }

    bpf_map_delete_elem(&pending_writes, &id);
}

/**
 * @brief Copy plaintext aside until the kernel reveals its destination
 *
 * The copy happens at SSL_write entry because an application using a memory
 * BIO may reuse the caller's buffer before the socket write happens.
 */
static __always_inline void
sngrep_tls_stash_write(const void *buf, __u32 len)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct pending_write *pw;
    struct conn_tuple *t;
    __u32 zero = 0;

    if (len == 0)
        return;
    if (len > SNGREP_TLS_MAX_MSG)
        len = SNGREP_TLS_MAX_MSG;

    // Two writes before a single send would otherwise lose the first one
    if ((t = bpf_map_lookup_elem(&tid_tuple, &id)))
        sngrep_tls_flush_pending(id, t);

    if (!(pw = bpf_map_lookup_elem(&write_scratch, &zero)))
        return;

    if (bpf_probe_read_user(pw->data, len, buf) != 0) {
        sngrep_tls_stat(SNGREP_TLS_STAT_READ_FAILED);
        return;
    }

    if (sngrep_filter_enabled && !sngrep_tls_looks_like_sip(pw->data, len))
        return;

    pw->len = len;
    bpf_map_update_elem(&pending_writes, &id, pw, BPF_ANY);
}

/**
 * @brief Publish plaintext just returned by a read, if it looks like SIP
 */
static __always_inline void
sngrep_tls_publish_read(const void *buf, __u32 total)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct conn_tuple *t;
    struct tls_event *e;
    __u32 zero = 0, off, n;
    int i;

    if (total == 0)
        return;
    if (total > SNGREP_TLS_MAX_MSG)
        total = SNGREP_TLS_MAX_MSG;

    // Without a socket for this thread the payload could not be placed
    if (!(t = bpf_map_lookup_elem(&tid_tuple, &id)))
        return;

    #pragma unroll
    for (i = 0; i < SNGREP_TLS_MAX_CHUNKS; i++) {
        off = (__u32) i * SNGREP_TLS_MAX_CHUNK;
        if (off >= total)
            break;

        n = total - off;
        if (n > SNGREP_TLS_MAX_CHUNK)
            n = SNGREP_TLS_MAX_CHUNK;

        if (!(e = bpf_map_lookup_elem(&scratch, &zero)))
            break;
        if (bpf_probe_read_user(e->data, n, (const char *) buf + off) != 0) {
            sngrep_tls_stat(SNGREP_TLS_STAT_READ_FAILED);
            break;
        }

        // Only the start of a message can be recognised as SIP; once it is
        // accepted its continuation has to follow whatever it contains
        if (i == 0 && sngrep_filter_enabled
            && !sngrep_tls_looks_like_sip(e->data, n))
            return;

        sngrep_tls_emit(e, n, SNGREP_TLS_INGRESS,
                        (off + n < total) ? 1 : 0, t);
    }
}

SEC("uprobe/SSL_write")
int BPF_UPROBE(sngrep_ssl_write, void *ssl, const void *buf, int num)
{
    if (num > 0)
        sngrep_tls_stash_write(buf, (__u32) num);
    return 0;
}

SEC("uprobe/SSL_write_ex")
int BPF_UPROBE(sngrep_ssl_write_ex, void *ssl, const void *buf, __u64 num)
{
    if (num > 0)
        sngrep_tls_stash_write(buf, (__u32) num);
    return 0;
}

SEC("uprobe/SSL_read")
int BPF_UPROBE(sngrep_ssl_read, void *ssl, void *buf, int num)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct active_read ar = { .buf = (__u64) buf, .readbytes_ptr = 0 };

    // No data yet, the buffer is only filled by the time the call returns
    bpf_map_update_elem(&active_reads, &id, &ar, BPF_ANY);
    return 0;
}

SEC("uretprobe/SSL_read")
int BPF_URETPROBE(sngrep_ssl_read_ret, int ret)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct active_read *ar;

    if (!(ar = bpf_map_lookup_elem(&active_reads, &id)))
        return 0;

    if (ret > 0)
        sngrep_tls_publish_read((const void *) ar->buf, (__u32) ret);

    bpf_map_delete_elem(&active_reads, &id);
    return 0;
}

SEC("uprobe/SSL_read_ex")
int BPF_UPROBE(sngrep_ssl_read_ex, void *ssl, void *buf, __u64 num,
               __u64 *readbytes)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct active_read ar = {
        .buf = (__u64) buf,
        .readbytes_ptr = (__u64) readbytes,
    };

    bpf_map_update_elem(&active_reads, &id, &ar, BPF_ANY);
    return 0;
}

SEC("uretprobe/SSL_read_ex")
int BPF_URETPROBE(sngrep_ssl_read_ex_ret, int ret)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct active_read *ar;
    __u64 nread = 0;

    if (!(ar = bpf_map_lookup_elem(&active_reads, &id)))
        return 0;

    // SSL_read_ex returns 1 on success and reports the length out of band
    if (ret == 1 && ar->readbytes_ptr
        && bpf_probe_read_user(&nread, sizeof(nread),
                               (const void *) ar->readbytes_ptr) == 0
        && nread > 0)
        sngrep_tls_publish_read((const void *) ar->buf, (__u32) nread);

    bpf_map_delete_elem(&active_reads, &id);
    return 0;
}

SEC("kprobe/tcp_sendmsg")
int BPF_KPROBE(sngrep_tcp_sendmsg, struct sock *sk)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct conn_tuple t;

    if (!sngrep_tls_tuple(sk, &t))
        return 0;

    bpf_map_update_elem(&tid_tuple, &id, &t, BPF_ANY);

    // This send is where the pending SSL_write was heading, whether it
    // happened inside SSL_write or just after it returned
    sngrep_tls_flush_pending(id, &t);
    return 0;
}

SEC("kprobe/tcp_recvmsg")
int BPF_KPROBE(sngrep_tcp_recvmsg, struct sock *sk)
{
    __u64 id = bpf_get_current_pid_tgid();
    struct conn_tuple t;

    if (sngrep_tls_tuple(sk, &t))
        bpf_map_update_elem(&tid_tuple, &id, &t, BPF_ANY);

    return 0;
}
