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
 * @file capture_bpf_util.h
 * @author Jurijs Ivolga <jurijs.ivolga@gmail.com>
 *
 * @brief Pure helpers for eBPF TLS capture
 *
 * Everything declared here is free of libbpf and kernel dependencies, so it
 * can be unit tested on any host. The half that does need libbpf lives in
 * capture_bpf.c.
 *
 */
#ifndef __SNGREP_CAPTURE_BPF_UTIL_H
#define __SNGREP_CAPTURE_BPF_UTIL_H

#include "config.h"
#include <limits.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "bpf/sngrep_tls.h"
#include "vector.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

//! Largest synthetic frame: Ethernet + IPv6 + TCP + one full chunk
#define CAPTURE_BPF_MAX_FRAME (14 + 40 + 20 + SNGREP_TLS_MAX_CHUNK)

/*
 * Longest connection key. A textual IPv6 address reaches 45 characters, so
 * the worst case is 45 + ':' + 5 + '>' + 45 + ':' + 5 plus a terminator, and
 * anything shorter would silently alias two different connections.
 */
#define CAPTURE_BPF_KEYLEN 108

/*
 * Connection directions tracked before the oldest is evicted. sng_malloc
 * refuses anything over MALLOC_MAX_SIZE, so the whole table has to fit in
 * 100kB: CAPTURE_BPF_KEYLEN plus a counter, times this count.
 */
#define CAPTURE_BPF_MAX_CONNS 768

/**
 * @brief Classify one line of /proc/<pid>/maps
 *
 * Accepts lines mapping a file whose basename begins with "libssl.so".
 * Rejects anonymous mappings, special regions such as [stack], libcrypto, and
 * mappings marked "(deleted)", because an unlinked file can not carry a
 * uprobe.
 *
 * @param line    One line from a maps file, trailing newline optional
 * @param path    Receives the mapped file path
 * @param pathlen Size of @path
 *
 * @return 1 if this line maps a libssl object, 0 otherwise
 */
int
capture_bpf_maps_line(const char *line, char *path, size_t pathlen);

/**
 * @brief Add @path to @paths unless the same file is already present
 *
 * Deduplicates on (st_dev, st_ino) rather than on the path string, so a
 * library reached through different symlinks or mount namespaces is attached
 * exactly once. Attaching twice would duplicate every captured message.
 *
 * @return 1 if added, 0 if it was a duplicate or could not be stat()ed
 */
int
capture_bpf_paths_add(vector_t *paths, const char *path);

/**
 * @brief Find every distinct libssl object mapped by a running process
 *
 * Paths are resolved through <procroot>/<pid>/root so libraries inside
 * container mount namespaces are reachable from ours.
 *
 * @param procroot Usually "/proc"; a parameter so tests can point elsewhere
 *
 * @return Vector of char* paths, to be released with vector_destroy()
 */
vector_t *
capture_bpf_find_libssl(const char *procroot);

//! Shorter declaration of the sequence number table
typedef struct capture_bpf_seqtab capture_bpf_seqtab_t;

/**
 * @brief Format a direction specific connection key
 *
 * The key identifies one half of a connection, so that each direction gets
 * its own sequence counter.
 */
void
capture_bpf_conn_key(char *buf, size_t buflen, uint8_t family,
                     const uint8_t *saddr, uint16_t sport,
                     const uint8_t *daddr, uint16_t dport);

/**
 * @brief Create a bounded table of per direction TCP sequence counters
 *
 * @param max_entries Hard cap; the oldest entry is evicted once it is reached
 */
capture_bpf_seqtab_t *
capture_bpf_seqtab_create(size_t max_entries);

/**
 * @brief Free a sequence table and everything it holds
 */
void
capture_bpf_seqtab_destroy(capture_bpf_seqtab_t *t);

/**
 * @brief Entries currently held, for tests and diagnostics
 */
size_t
capture_bpf_seqtab_count(capture_bpf_seqtab_t *t);

/**
 * @brief Reserve @len sequence numbers for direction @key
 *
 * @param key    Key of the direction being sent
 * @param revkey Key of the opposite direction, used to compute the ack
 * @param ack    Receives the opposite direction's next expected sequence
 *
 * @return The sequence number this segment should carry
 */
uint32_t
capture_bpf_seqtab_next(capture_bpf_seqtab_t *t, const char *key,
                        const char *revkey, uint32_t len, uint32_t *ack);

/**
 * @brief Build a synthetic Ethernet + IP + TCP frame carrying @payload
 *
 * Checksums are left zero. sngrep never validates them and neither does the
 * pcap writer, matching what the EEP capture source already does. The MAC
 * addresses follow the same convention.
 *
 * @param family 4 or 6
 * @param push   Set the PSH flag, marking the last chunk of a message
 *
 * @return Total frame length, or 0 if it would not fit in @buflen
 */
size_t
capture_bpf_build_frame(u_char *buf, size_t buflen, uint8_t family,
                        const uint8_t *saddr, uint16_t sport,
                        const uint8_t *daddr, uint16_t dport,
                        uint32_t seq, uint32_t ack, bool push,
                        const u_char *payload, uint32_t payload_len);

#endif /* __SNGREP_CAPTURE_BPF_UTIL_H */
