/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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

//! Shorter declaration of vector structure
typedef struct vector vector_t;
//! Shorter declaration of iterator structure
typedef struct vector_iter vector_iter_t;

/**
 * @brief Structure to hold a list of pointers
 */
struct vector
{
    //! Number of elements in list
    int count;
    //! Total space in list (available + elements)
    int limit;
    //! Number of new spaces to be reallocated
    int step;
    //! Elements of the vector
    void **list;
};

struct vector_iter
{
    //! Last requested position
    int current;
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
 * @brief Remove itemn from vector
 *
 * TODO Not yet implemented
 */
void
vector_remove(vector_t *vector, void *item);

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
 * @brief Return first item of the vector
 */
void *
vector_first(vector_t *vector);

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
 *
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
