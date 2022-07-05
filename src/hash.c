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
 * @file hash.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in hash.h
 *
 */
#include "hash.h"
#include <string.h>
#include <stdlib.h>

htable_t *
htable_create(size_t size)
{
    htable_t *h;

    // Allocate memory for this table data
    if (!(h = malloc(sizeof(htable_t))))
        return NULL;

    h->size = size;

    // Allocate memory for this table buckets
    if (!(h->buckets = malloc(sizeof(hentry_t) * size))) {
        free(h);
        return NULL;
    }

    // Initialize allocated memory
    memset(h->buckets, 0, sizeof(hentry_t) * size);

    // Return allocated table
    return h;
}

void
htable_destroy(htable_t *table)
{
    free(table->buckets);
    free(table);
}

int
htable_insert(htable_t *table, const char *key, void *data)
{
    // Get hash position for given entry
    size_t pos = htable_hash(table, key);

    // Create a new entry for given key
    hentry_t *entry;
    if (!(entry = malloc(sizeof(hentry_t))))
        return -1;

    entry->key = key;
    entry->data = data;
    entry->next = 0;

    // Check if the hash position is in use
    hentry_t *exists = table->buckets[pos];

    if (!exists) {
        table->buckets[pos] = entry;
    } else {
        while (exists->next) {
            exists = exists->next;
        }
        exists->next = entry;
    }
    return 0;
}

void
htable_remove(htable_t *table, const char *key)
{
    // Get hash position for given entry
    size_t pos = htable_hash(table, key);

    // Check if the hash position is in use
    hentry_t *entry, *prev = NULL;
    for (entry = table->buckets[pos]; entry; prev = entry, entry = entry->next) {
        if (!strcmp(entry->key, key)) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->buckets[pos] = entry->next;
            }
            // Remove item memory
            free(entry);
            return;
        }
    }
}

void *
htable_find(htable_t *table, const char *key)
{
    // Get hash position for given entry
    size_t pos = htable_hash(table, key);

    // Check if the hash position is in use
    hentry_t *entry;
    for (entry = table->buckets[pos]; entry; entry = entry->next) {
        if (!strcmp(entry->key, key)) {
            //! Found
            return entry->data;
        }
    }

    // Not found
    return NULL;
}

size_t
htable_hash(htable_t *table, const char *key)
{
    // dbj2 - http://www.cse.yorku.ca/~oz/hash.html
    size_t hash = 5381;
    while (*key++) {
        hash = ((hash << 5) + hash) ^ *key;
    }
    return hash & (table->size - 1);
}
