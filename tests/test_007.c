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
#include <stdlib.h>
#include "vector.h"
#include "util.h"

int main ()
{
    vector_t *vector;

    // Basic Vector append/remove test
    vector = vector_create(10, 10);
    assert(vector);
    assert(vector_count(vector) == 0);
    vector_append(vector, 0);
    assert(vector_count(vector) == 0);
    vector_append(vector, sng_malloc(1024));
    assert(vector_count(vector) == 1);
    assert(vector_first(vector) == vector_item(vector, 0));
    vector_remove(vector, vector_first(vector));
    assert(vector_count(vector) == 0);
    assert(vector_first(vector) == vector_item(vector, 0));

    // Vector overflow test
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    // Next append requires memory reallocation
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    vector_append(vector, sng_malloc(32));
    // Expected vector size
    assert(vector_count(vector) == 16);
    // Expected empty position
    assert(vector_item(vector, vector_count(vector)) == 0);
    // Remove position (use generic destroyer)
    vector_set_destroyer(vector, vector_generic_destroyer);
    vector_remove(vector, vector_item(vector, 12));
    assert(vector_count(vector) == 15);

    return 0;
}
