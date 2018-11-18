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
 * @file glib-utils.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper function for glib containers
 *
 */

#ifndef SNGREP_GLIB_UTILS_H
#define SNGREP_GLIB_UTILS_H

#include <glib.h>

#define g_ptr_array_len(array) (array->len)
#define g_ptr_array_first(array) g_ptr_array_index(array, 0)
#define g_ptr_array_last(array)  g_ptr_array_index(array, array->len-1)

#define g_sequence_first(sequence) g_sequence_nth(sequence, 0)
#define g_sequence_last(sequence) g_sequence_nth(sequence, g_sequence_get_length(sequence))

gpointer
g_sequence_nth(GSequence *sequence, guint index);

gint
g_sequence_iter_length(GSequenceIter *iter);

void
g_sequence_iter_set_pos(GSequenceIter **iter, gint pos);

gint
g_sequence_index(GSequence *sequence, gconstpointer item);

void
g_sequence_remove_data(GSequence *sequence, gconstpointer item);

void
g_sequence_remove_all(GSequence *sequence);

void
g_sequence_append_sequence(GSequence *sequence, GSequence *items);

GSequence *
g_sequence_copy(GSequence *sequence, GEqualFunc filter_func, gpointer filter_data);

GPtrArray *
g_ptr_array_copy(GPtrArray *origin);

GPtrArray *
g_ptr_array_copy_filtered(GPtrArray *origin, GEqualFunc filter_func, gpointer filter_data);

gint
g_ptr_array_data_index(GPtrArray *array, gconstpointer data);

gpointer
g_ptr_array_next(GPtrArray *array, gconstpointer data);

gpointer
g_ptr_array_prev(GPtrArray *array, gconstpointer data);

void
g_ptr_array_add_array(GPtrArray *array, GPtrArray *items);

void
g_ptr_array_remove_array(GPtrArray *array, GPtrArray *items);

#if !GLIB_CHECK_VERSION(2,54,0)
gboolean
g_ptr_array_find(GPtrArray *haystack, gconstpointer needle, guint *index);

gboolean
g_ptr_array_find_with_equal_func(GPtrArray *haystack, gconstpointer needle, GEqualFunc equal_func, guint *index);
#endif

#endif //SNGREP_GLIB_UTILS_H
