//
// Created by kaian on 9/04/18.
//

#include <glib.h>
#include "glib-utils.h"

gpointer
g_sequence_nth(GSequence *sequence, guint index)
{
    GSequenceIter *pos = g_sequence_get_iter_at_pos(sequence, index);
    return (g_sequence_iter_is_end(pos)) ? NULL : g_sequence_get(pos);
}


gint
g_sequence_iter_length(GSequenceIter *iter)
{
    return g_sequence_get_length(g_sequence_iter_get_sequence(iter));
}

void
g_sequence_iter_set_pos(GSequenceIter **iter, gint pos)
{
    *iter = g_sequence_get_iter_at_pos(g_sequence_iter_get_sequence(*iter), pos);
}

gint
g_sequence_index(GSequence *sequence, gconstpointer item)
{
    GSequenceIter *it = g_sequence_get_begin_iter(sequence);

    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        if (g_sequence_get(it) == item) {
            return g_sequence_iter_get_position(it);
        }
    }

    return -1;
}

void
g_sequence_remove_data(GSequence *sequence, gconstpointer item)
{
    GSequenceIter *it = g_sequence_get_begin_iter(sequence);

    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        if (g_sequence_get(it) == item) {
            g_sequence_remove(g_sequence_iter_prev(it));
            break;
        }
    }
}

void
g_sequence_remove_all(GSequence *sequence)
{
    g_sequence_remove_range(
        g_sequence_get_begin_iter(sequence),
        g_sequence_get_end_iter(sequence)
    );
}

void
g_sequence_append_sequence(GSequence *sequence, GSequence *items)
{
    GSequenceIter *it = g_sequence_get_begin_iter(items);
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        g_sequence_append(sequence, g_sequence_get(it));
    }
}

GSequence *
g_sequence_copy(GSequence *sequence, GEqualFunc filter_func, gpointer filter_data)
{
    GSequence *copy = g_sequence_new(NULL);

    GSequenceIter *it = g_sequence_get_begin_iter(sequence);
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {

        if (filter_func && !filter_func(g_sequence_get(it), filter_data))
            continue;

        g_sequence_append(copy, g_sequence_get(it));
    }

    return copy;
}

GPtrArray *
g_ptr_array_copy(GPtrArray *origin)
{
    return g_ptr_array_copy_filtered(origin, NULL, NULL);
}

GPtrArray *
g_ptr_array_copy_filtered(GPtrArray *origin, GEqualFunc filter_func, gpointer filter_data)
{
    GPtrArray *copy = g_ptr_array_new();

    for (guint i = 0; i < g_ptr_array_len(origin); i++) {

        if (filter_func && !filter_func(g_ptr_array_index(origin, i), filter_data))
            continue;

        g_ptr_array_add(copy, g_ptr_array_index(origin, i));
    }

    return copy;
}

gint
g_ptr_array_data_index(GPtrArray *array, gconstpointer data)
{
    guint pos;

    if (g_ptr_array_find(array, data, &pos)) {
        return pos;
    }

    return -1;
}

gpointer
g_ptr_array_next(GPtrArray *array, gconstpointer data)
{
    guint pos;

    if (g_ptr_array_len(array) == 0)
        return NULL;

    if (data == NULL)
        return g_ptr_array_first(array);

    if (g_ptr_array_find(array, data, &pos)) {
        if ((pos + 1) != g_ptr_array_len(array)) {
            return g_ptr_array_index(array, pos + 1);
        }
    }

    return NULL;
}

gpointer
g_ptr_array_prev(GPtrArray *array, gconstpointer data)
{
    guint pos;

    if (g_ptr_array_len(array) == 0)
        return NULL;

    if (data == NULL)
        return g_ptr_array_last(array);

    if (g_ptr_array_find(array, data, &pos)) {
        if (pos > 0) {
            return g_ptr_array_index(array, pos - 1);
        }
    }

    return NULL;
}

static void
g_ptr_array_add_cb(gpointer item, GPtrArray *array)
{
    if (!g_ptr_array_find(array, item, NULL)) {
        g_ptr_array_add(array, item);
    }
}

void
g_ptr_array_add_array(GPtrArray *array, GPtrArray *items)
{
    g_ptr_array_foreach(items, (GFunc) g_ptr_array_add_cb, array);
}

static void
g_ptr_array_remove_cb(gpointer item, GPtrArray *array)
{
    g_ptr_array_remove(array, item);
}

void
g_ptr_array_remove_array(GPtrArray *array, GPtrArray *items)
{
    g_ptr_array_foreach(items, (GFunc) g_ptr_array_remove_cb, array);
}

/**
 * g_ptr_array_find: (skip)
 * @haystack: pointer array to be searched
 * @needle: pointer to look for
 * @index_: (optional) (out caller-allocates): return location for the index of
 *    the element, if found
 *
 * Checks whether @needle exists in @haystack. If the element is found, %TRUE is
 * returned and the element’s index is returned in @index_ (if non-%NULL).
 * Otherwise, %FALSE is returned and @index_ is undefined. If @needle exists
 * multiple times in @haystack, the index of the first instance is returned.
 *
 * This does pointer comparisons only. If you want to use more complex equality
 * checks, such as string comparisons, use g_ptr_array_find_with_equal_func().
 *
 * Returns: %TRUE if @needle is one of the elements of @haystack
 * Since: 2.54
 */
gboolean
g_ptr_array_find (GPtrArray     *haystack,
                  gconstpointer  needle,
                  guint         *index_)
{
    return g_ptr_array_find_with_equal_func (haystack, needle, NULL, index_);
}

/**
 * g_ptr_array_find_with_equal_func: (skip)
 * @haystack: pointer array to be searched
 * @needle: pointer to look for
 * @equal_func: (nullable): the function to call for each element, which should
 *    return %TRUE when the desired element is found; or %NULL to use pointer
 *    equality
 * @index_: (optional) (out caller-allocates): return location for the index of
 *    the element, if found
 *
 * Checks whether @needle exists in @haystack, using the given @equal_func.
 * If the element is found, %TRUE is returned and the element’s index is
 * returned in @index_ (if non-%NULL). Otherwise, %FALSE is returned and @index_
 * is undefined. If @needle exists multiple times in @haystack, the index of
 * the first instance is returned.
 *
 * @equal_func is called with the element from the array as its first parameter,
 * and @needle as its second parameter. If @equal_func is %NULL, pointer
 * equality is used.
 *
 * Returns: %TRUE if @needle is one of the elements of @haystack
 * Since: 2.54
 */
gboolean
g_ptr_array_find_with_equal_func (GPtrArray     *haystack,
                                  gconstpointer  needle,
                                  GEqualFunc     equal_func,
                                  guint         *index_)
{
    guint i;

    g_return_val_if_fail (haystack != NULL, FALSE);

    if (equal_func == NULL)
        equal_func = g_direct_equal;

    for (i = 0; i < haystack->len; i++)
    {
        if (equal_func (g_ptr_array_index (haystack, i), needle))
        {
            if (index_ != NULL)
                *index_ = i;
            return TRUE;
        }
    }

    return FALSE;
}