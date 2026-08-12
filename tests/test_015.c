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
 * @file test_015.c
 * @brief Unit tests for eBPF libssl discovery
 *
 * Covers /proc/<pid>/maps line classification and deduplication of libraries
 * reached through more than one path.
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/capture_bpf_util.h"
#include "../src/vector.h"

static void
test_maps_line_accepts(void)
{
    char path[PATH_MAX];

    assert(capture_bpf_maps_line(
        "7f8c1a000000-7f8c1a02c000 r-xp 00000000 08:01 1234 "
        "/usr/lib/x86_64-linux-gnu/libssl.so.3\n", path, sizeof(path)));
    assert(strcmp(path, "/usr/lib/x86_64-linux-gnu/libssl.so.3") == 0);

    /* OpenSSL 1.1 naming */
    assert(capture_bpf_maps_line(
        "7f00-7f01 r-xp 00000000 08:01 99 /lib/libssl.so.1.1\n",
        path, sizeof(path)));
    assert(strcmp(path, "/lib/libssl.so.1.1") == 0);

    /* musl / Alpine layout */
    assert(capture_bpf_maps_line(
        "7f00-7f01 r-xp 00000000 08:01 99 /usr/lib/libssl.so.3\n",
        path, sizeof(path)));
    assert(strcmp(path, "/usr/lib/libssl.so.3") == 0);

    /* Unversioned development symlink */
    assert(capture_bpf_maps_line(
        "7f00-7f01 r-xp 00000000 08:01 99 /usr/local/lib/libssl.so\n",
        path, sizeof(path)));
    assert(strcmp(path, "/usr/local/lib/libssl.so") == 0);

    printf("test_maps_line_accepts: OK\n");
}

static void
test_maps_line_rejects(void)
{
    char path[PATH_MAX];

    /* Anonymous mapping, no path field at all */
    assert(!capture_bpf_maps_line(
        "7f8c1a000000-7f8c1a02c000 rw-p 00000000 00:00 0 \n",
        path, sizeof(path)));

    /* Special regions */
    assert(!capture_bpf_maps_line(
        "7ffd0000-7ffd1000 rw-p 00000000 00:00 0 [stack]\n",
        path, sizeof(path)));

    /* libcrypto is not libssl; sshd maps it and exports no SSL_write */
    assert(!capture_bpf_maps_line(
        "7f00-7f01 r-xp 00000000 08:01 99 /usr/lib/libcrypto.so.3\n",
        path, sizeof(path)));

    /* A library merely named after libssl must not match on substring */
    assert(!capture_bpf_maps_line(
        "7f00-7f01 r-xp 00000000 08:01 99 /usr/lib/mylibssl_helper.so\n",
        path, sizeof(path)));

    /* An unlinked file cannot carry a uprobe */
    assert(!capture_bpf_maps_line(
        "7f00-7f01 r-xp 00000000 08:01 99 /usr/lib/libssl.so.3 (deleted)\n",
        path, sizeof(path)));

    printf("test_maps_line_rejects: OK\n");
}

static void
test_dedup(void)
{
    vector_t *paths = vector_create(0, 4);

    /* A real file that certainly exists, added once */
    assert(capture_bpf_paths_add(paths, "/bin/sh") == 1);
    /* The same inode reached again must not produce a second attachment */
    assert(capture_bpf_paths_add(paths, "/bin/sh") == 0);
    assert(vector_count(paths) == 1);

    /* A path that does not exist cannot be attached to */
    assert(capture_bpf_paths_add(paths, "/nonexistent/libssl.so.3") == 0);
    assert(vector_count(paths) == 1);

    vector_destroy(paths);
    printf("test_dedup: OK\n");
}

int main(void)
{
    test_maps_line_accepts();
    test_maps_line_rejects();
    test_dedup();
    printf("All libssl discovery tests passed\n");
    return 0;
}
