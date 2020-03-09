/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
 * @file gptrarray.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper function for GPtrArray containers
 *
 */

#ifndef __SNGREP_GLIB_GPTRARRAY_H
#define __SNGREP_GLIB_GPTRARRAY_H

#include <glib.h>

#define g_ptr_array_len(array) (array->len)
#define g_ptr_array_empty(array) (array->len == 0)
#define g_ptr_array_first(array) g_ptr_array_index(array, 0)
#define g_ptr_array_last(array)  g_ptr_array_index(array, array->len-1)
#define g_ptr_array_set(array, index, item)  (array->pdata[index] = item)

GPtrArray *
g_ptr_array_deep_copy(GPtrArray *origin);

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

void
g_ptr_array_remove_all(GPtrArray *array);

void
g_ptr_array_foreach_idx(GPtrArray *array, GFunc func, gpointer user_data);

#if !GLIB_CHECK_VERSION(2, 54, 0)
gboolean
g_ptr_array_find(GPtrArray *haystack, gconstpointer needle, guint *index);

gboolean
g_ptr_array_find_with_equal_func(GPtrArray *haystack, gconstpointer needle, GEqualFunc equal_func, guint *index);
#endif

#endif //__SNGREP_GLIB_GPTRARRAY_H
