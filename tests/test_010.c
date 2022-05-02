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
 * @file test_vector.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * Basic testing of vector structures
 */

#include "config.h"
#include <assert.h>
#include <string.h>
#include "../src/hash.h"

int main ()
{
    htable_t *table;
    table = htable_create(10);
    assert(table);

    // Check a key is not found
    assert(htable_find(table, "notfoud") == NULL);

    // Check a key can be added
    htable_insert(table, "key", "data");
    const char *data = htable_find(table, "key");
    // And found
    assert(strcmp(data, "data") == 0);

    // Try filling all the buckets
    htable_insert(table, "key1", "data1");
    htable_insert(table, "key2", "data2");
    htable_insert(table, "key3", "data3");
    htable_insert(table, "key4", "data4");
    htable_insert(table, "key5", "data5");
    htable_insert(table, "key6", "data6");
    htable_insert(table, "key7", "data7");
    htable_insert(table, "key8", "data8");
    htable_insert(table, "key9", "data9");
    htable_insert(table, "key10", "data10");
    htable_insert(table, "key11", "data11");
    htable_insert(table, "key12", "data12");
    htable_insert(table, "key13", "data13");
    htable_insert(table, "key14", "data14");
    htable_insert(table, "key15", "data15");

    // Find one entry
    const char *data7 = htable_find(table, "key7");
    assert(strcmp(data7, "data7") == 0);

    // Remove one entry
    htable_remove(table, "key7");
    assert(htable_find(table, "key7") == NULL);

    // Find another entries
    const char *data5 = htable_find(table, "key5");
    assert(strcmp(data5, "data5") == 0);
    const char *data10 = htable_find(table, "key10");
    assert(strcmp(data10, "data10") == 0);

    // Remove all entries
    htable_remove(table, "key1");
    htable_remove(table, "key2");
    htable_remove(table, "key3");
    htable_remove(table, "key4");
    htable_remove(table, "key5");
    htable_remove(table, "key6");
    htable_remove(table, "key7");
    htable_remove(table, "key8");
    htable_remove(table, "key9");
    htable_remove(table, "key10");
    htable_remove(table, "key11");
    htable_remove(table, "key12");
    htable_remove(table, "key13");
    htable_remove(table, "key14");
    htable_remove(table, "key15");

    // Search a not found entry
    assert(htable_find(table, "key7") == NULL);

    // Destroy the table
    htable_destroy(table);

    return 0;
}
