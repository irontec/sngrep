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
 * @file vector.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage lists of pointers
 *
 *
 */

#ifndef __SNGREP_VECTOR_H_
#define __SNGREP_VECTOR_H_

#include "config.h"
#include <stdint.h>

//! Shorter declaration of vector structure
typedef struct vector vector_t;
//! Shorter declaration of iterator structure
typedef struct vector_iter vector_iter_t;

/**
 * @brief Structure to hold a list of pointers
 */
struct vector {
    //! Number of elements in list
    uint32_t count;
    //! Total space in list (available + elements)
    uint32_t limit;
    //! Number of new spaces to be reallocated
    uint8_t step;
    //! Elements of the vector
    void **list;
    //! Function to destroy one item
    void (*destroyer) (void *item);
    //! Function to sort each appended/inserted item
    void (*sorter) (vector_t *vector, void *item);
};

struct vector_iter {
    //! Last requested position
    int current;
    //! Last vector position
    int current_vector;
    //! Vector that's being iterated
    vector_t *vector;
    //! Filter iterator results using this func
    int (*filter) (void *item);
};

/**
 * @brief Create a new vector
 *
 * Create a new vector with initial size and
 * step increase settings.
 */
vector_t *
vector_create(int limit, int step);

/**
 * @brief Free vector memory
 */
void
vector_destroy(vector_t *vector);

/**
 * @brief Free all vector items and memory
 */
void
vector_destroy_items(vector_t *vector);

/**
 * @brief Clone a vector container
 *
 * The cloned vector will have its own storage of pointers
 * **but the items in its list will be shared with the
 * original vector**
 *
 * Use with caution, specially with vector that have sorter
 * or destroyer functions.
 */
vector_t *
vector_clone(vector_t *original);

/**
 * @brief Copy filtered elements to a new vector
 *
 */
vector_t *
vector_copy_if(vector_t *original, int (*filter)(void *item));

/**
 * @brief Remove all items of vector
 *
 */
void
vector_clear(vector_t *vector);

/**
 * @brief Append an item to vector
 *
 * Item will be added at the end of the
 * items list.
 *
 * @return index of the appended item
 */
int
vector_append(vector_t *vector, void *item);

/**
 * @brief Append a vector to another vector
 * @param dst Vector that will append the new items
 * @param src Vector that contain the items to append
 * @return 0 in case of success, 1 otherwise
 */
int
vector_append_vector(vector_t *dst, vector_t *src);

/**
 * @brief Insert an item in a given vector position
 *
 * @return count of elements in vector
 */
int
vector_insert(vector_t *vector, void *item, int pos);

/**
 * @brief Remove itemn from vector
 */
void
vector_remove(vector_t *vector, void *item);

/**
 * @brief Set the vector destroyer
 *
 * A destroyer is a function that will be invoked
 * for each item when the vector is destroyed or an
 * item is removed.
 */
void
vector_set_destroyer(vector_t *vector, void (*destroyer) (void *item));

/**
 * @brief Set the vector sorter
 *
 * The sorter function will be invoked every time a new
 * item is appended into the vector.
 *
 */
void
vector_set_sorter(vector_t *vector, void (*sorter) (vector_t *vector, void *item));

/**
 * @brief A generic item destroyer
 *
 * Generic memory deallocator for those items that only
 * require a simple 'free'
 */
void
vector_generic_destroyer(void *item);

/**
 * @brief Get an item from vector
 *
 * Return the item at given index, or NULL
 * if index is out of the vector bounds
 *
 */
void *
vector_item(vector_t *vector, int index);

/**
 * @brief Set an item in a given index
 *
 * This funtion will set an item in a given index.
 * The index MUST be in the already allocated memory
 * of the vector. This can be used to replace a vector
 * item. If position is already in use, destroyer won't
 * be call for existing item.
 */
void
vector_set_item(vector_t *vector, int index, void *item);


/**
 * @brief Return first item of the vector
 */
void *
vector_first(vector_t *vector);

/**
 * @brief Return last item of the vector
 */
void *
vector_last(vector_t *vector);

/**
 * @brief Get the index of an item
 *
 * Return the index of item in vector or -1 if
 * the item is not found
 */
int
vector_index(vector_t *vector, void *item);

/**
 * @brief Return the number of items of vector
 */
int
vector_count(vector_t *vector);

/**
 * @brief Return a new iterator for given vector
 */
vector_iter_t
vector_iterator(vector_t *vector);

/**
 * @brief Return the vector of this iterator
 */
vector_t *
vector_iterator_vector(vector_iter_t *it);

/**
 * @brief Return the number of items of iterator
 */
int
vector_iterator_count(vector_iter_t *it);

/**
 * @brief Return next element of iterator
 */
void *
vector_iterator_next(vector_iter_t *it);

/**
 * @brief Return prev element of iterator
 */
void *
vector_iterator_prev(vector_iter_t *it);

/**
 * @brief Set iterator filter funcion
 *
 * Filter iterator results using given function
 */
void
vector_iterator_set_filter(vector_iter_t *it, int (*filter) (void *item));

/**
 * @brief Set current iterator position
 */
void
vector_iterator_set_current(vector_iter_t *it, int current);

/**
 * @brief Set iterator position to the last element
 */
void
vector_iterator_set_last(vector_iter_t *it);

/**
 * @brief Return current iterator position
 */
int
vector_iterator_current(vector_iter_t *it);

/**
 * @brief Reset iterator position to initial
 */
void
vector_iterator_reset(vector_iter_t *it);

#endif /* __SNGREP_VECTOR_H_ */
