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
 * @file vector.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in vector.h
 *
 */
#include "vector.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

vector_t *
vector_create(int limit, int step)
{
    vector_t *v;
    // Allocate memory for this vector data
    if (!(v = malloc(sizeof(vector_t))))
        return NULL;

    v->count = 0;
    v->limit = limit;
    v->step = step;
    v->list = NULL;
    v->sorter = NULL;
    v->destroyer = NULL;

    return v;
}

void
vector_destroy(vector_t *vector)
{
    // Nothing to free. Done.
    if (!vector) return;
    // Remove all items if a destroyer is set
    vector_clear(vector);
    // Deallocate vector list
    sng_free(vector->list);
    // Deallocate vector itself
    sng_free(vector);
}

void
vector_destroy_items(vector_t *vector)
{
    int i;

    // Nothing to free. Done
    if (!vector) return;

    // If vector contains items
    if (vector->count) {
        for (i = 0; i < vector->count; i++) {
            free(vector->list[i]);
        }
        free(vector->list);
    }
    free(vector);
}

vector_t *
vector_clone(vector_t *original)
{
    vector_t *clone;
    vector_iter_t it;
    void *item;

    // Check we have a valid vector pointer
    if (!original)
        return NULL;

    // Create a new vector structure
    clone = vector_create(original->limit, original->step);
    vector_set_destroyer(clone, original->destroyer);
    vector_set_sorter(clone, original->sorter);

    // Fill the clone vector with the same elements
    it = vector_iterator(original);
    while ((item = vector_iterator_next(&it)))
        vector_append(clone, item);

    // Return the cloned vector
    return clone;
}

vector_t *
vector_copy_if(vector_t *original, int (*filter)(void *item))
{
    vector_t *clone;
    vector_iter_t it;
    void *item;

    // Check we have a valid vector pointer
    if (!original)
        return NULL;

    // Create a new vector structure
    clone = vector_create(0, 1);
    // Fill the clone vector with the same elements applying filter
    it = vector_iterator(original);
    vector_iterator_set_filter(&it, filter);
    while ((item = vector_iterator_next(&it))) {
        vector_append(clone, item);
    }
    // Return the cloned vector
    return clone;
}

void
vector_clear(vector_t *vector)
{
    // Remove all items in the vector
    while (vector_first(vector))
        vector_remove(vector, vector_first(vector));
}

int
vector_append(vector_t *vector, void *item)
{
    // Sanity check
    if (!item)
        return vector->count;

    // Check if the vector has been initializated
    if (!vector->list) {
        vector->list = malloc(sizeof(void *) * vector->limit);
        memset(vector->list, 0, sizeof(void *) * vector->limit);
    }

    // Check if we need to increase vector size
    if (vector->count == vector->limit) {
        // Increase vector size
        vector->limit += vector->step;
        // Add more memory to the list
        vector->list = realloc(vector->list, sizeof(void *) * vector->limit);
        // Initialize new allocated memory
        memset(vector->list + vector->limit - vector->step, 0, vector->step);
    }

    // Add item to the end of the list
    vector->list[vector->count++] = item;

    // Check if vector has a sorter
    if (vector->sorter) {
        vector->sorter(vector, item);
    }

    return vector->count - 1;
}

int
vector_append_vector(vector_t *dst, vector_t *src)
{
    if (!dst || !src)
        return 1;

    vector_iter_t it = vector_iterator(src);

    void *item;
    while ((item = vector_iterator_next(&it)))
        vector_append(dst, item);

    return 0;
}

int
vector_insert(vector_t *vector, void *item, int pos)
{
    if (!item)
        return vector->count;

    if (pos < 0 || pos > vector->count - 2)
        return vector->count;

    // If position is already filled with that item, we're done
    if (vector->list[pos] == item)
        return vector->count;

    // If possition is occupied, move the other position
    if (vector->list[pos]) {
        memmove(vector->list + pos + 1, vector->list + pos,
                sizeof(void *) * (vector->count - pos - 1));
    }

    // Set the position
    vector->list[pos] = item;
    return vector->count;
}


void
vector_remove(vector_t *vector, void *item)
{
    // Get item position
    int idx = vector_index(vector, item);
    // Not found in the vector
    if (idx == -1)
        return;

    // Decrease item counter
    vector->count--;
    // Move the rest of the elements one position up
    memmove(vector->list + idx, vector->list + idx + 1, sizeof(void *) * (vector->count - idx));
    // Reset vector last position
    vector->list[vector->count] = NULL;

    // Destroy the item if vector has a destroyer
    if (vector->destroyer) {
        vector->destroyer(item);
    }
}

void
vector_set_destroyer(vector_t *vector, void (*destroyer) (void *item))
{
    vector->destroyer = destroyer;
}

void
vector_set_sorter(vector_t *vector, void (*sorter) (vector_t *vector, void *item))
{
    vector->sorter = sorter;
}

void
vector_generic_destroyer(void *item)
{
    sng_free(item);
}

void *
vector_item(vector_t *vector, int index)
{
    if (!vector || index >= vector->count || index < 0)
        return NULL;
    return vector->list[index];
}

void
vector_set_item(vector_t *vector, int index, void *item)
{
    if (!vector || index >= vector->count || index < 0)
        return;
    vector->list[index] = item;
}

void *
vector_first(vector_t *vector)
{
    return vector_item(vector, 0);
}

void *
vector_last(vector_t *vector)
{
    return vector_item(vector, vector_count(vector) - 1);
}

int
vector_index(vector_t *vector, void *item)
{
    // FIXME Bad perfomance
    int i;
    for (i = 0; i < vector->count; i++) {
        if (vector->list[i] == item)
            return i;
    }
    return -1;
}

int
vector_count(vector_t *vector)
{
    return (vector) ? vector->count : 0;
}

vector_iter_t
vector_iterator(vector_t *vector)
{
    vector_iter_t it;
    memset(&it, 0, sizeof(vector_iter_t));
    it.current = -1;
    it.vector = vector;
    return it;
}

vector_t *
vector_iterator_vector(vector_iter_t *it)
{
    return it->vector;
}

int
vector_iterator_count(vector_iter_t *it)
{
    int count = 0;
    int pos = it->current;

    vector_iterator_reset(it);

    if (!it->filter) {
        count = vector_count(it->vector);
    } else {
        while (vector_iterator_next(it)) {
            count++;
        }
    }

    vector_iterator_set_current(it, pos);

    return count;
}

void *
vector_iterator_next(vector_iter_t *it)
{
    void *item;

    if (!it || it->current >= vector_count(it->vector))
        return NULL;

    while ((item = vector_item(it->vector, ++it->current))) {
        if (it->filter) {
            if (it->filter(item)) {
                return item;
            }
        } else {
            return item;
        }
    }
    return NULL;
}

void *
vector_iterator_prev(vector_iter_t *it)
{
    void *item;

    if (it->current == -1)
        return NULL;

    while ((item = vector_item(it->vector, --it->current))) {
        if (it->filter) {
            if (it->filter(item)) {
                return item;
            }
        } else {
            return item;
        }
    }
    return NULL;
}

void
vector_iterator_set_filter(vector_iter_t *it, int
                           (*filter)(void *item))
{
    it->filter = filter;
}

void
vector_iterator_set_current(vector_iter_t *it, int current)
{
    it->current = current;
}

void
vector_iterator_set_last(vector_iter_t *it)
{
    it->current = vector_count(it->vector);
}

int
vector_iterator_current(vector_iter_t *it)
{
    return it->current;
}

void
vector_iterator_reset(vector_iter_t *it)
{
    vector_iterator_set_current(it, -1);
}

