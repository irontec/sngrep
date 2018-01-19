/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
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
 * @file hash.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage hash tables
 */

#ifndef __SNGREP_HASH_H_
#define __SNGREP_HASH_H_

#include "config.h"
#include <stdio.h>

//! Shorter declaration of hash structures
typedef struct htable htable_t;
typedef struct hentry hentry_t;

/**
 *  Structure to hold a Hash table entry
 */
struct hentry {
    //! Key of the hash entry
    const char *key;
    //! Pointer to has entry data
    void *data;
    //! Next entry sharing the same hash value
    hentry_t *next;
};

struct htable {
    //! Fixed hash table limit
    size_t size;
    // Hash table entries
    hentry_t **buckets;
};

htable_t *
htable_create(size_t size);

void
htable_destroy(htable_t *table);

int
htable_insert(htable_t *table, const char *key, void *data);

void
htable_remove(htable_t *table, const char *key);

void *
htable_find(htable_t *table, const char *key);

size_t
htable_hash(htable_t *table, const char *key);

#endif /* __SNGREP_HASH_H_ */
