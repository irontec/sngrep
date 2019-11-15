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
 * @file glib-extra.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper function for glib containers
 *
 */
#include "config.h"
#include <sys/sysinfo.h>
#include "glib-extra.h"

GList *
g_list_concat_deep(GList *dst, GList *src)
{
    for (GList *l = src; l != NULL; l = l->next) {
        dst = g_list_append(dst, l->data);
    }

    return dst;
}

void
g_list_item_free(gpointer item, G_GNUC_UNUSED gpointer user_data)
{
    g_free(item);
}

GPtrArray *
g_ptr_array_deep_copy(GPtrArray *origin)
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

    if (g_ptr_array_empty(array))
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

    if (g_ptr_array_empty(array))
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

void
g_ptr_array_remove_all(GPtrArray *array)
{
    if (!g_ptr_array_empty(array)) {
        g_ptr_array_remove_range(array, 0, g_ptr_array_len(array));
    }
}

void
g_ptr_array_foreach_idx(GPtrArray *array, GFunc func, gpointer user_data)
{
    g_return_if_fail (array);

    for (guint i = 0; i < array->len; i++)
        (*func)(GINT_TO_POINTER(i), user_data);
}

#if !GLIB_CHECK_VERSION(2, 54, 0)
gboolean
g_ptr_array_find(GPtrArray *haystack, gconstpointer needle, guint *index_)
{
    return g_ptr_array_find_with_equal_func(haystack, needle, NULL, index_);
}

gboolean
g_ptr_array_find_with_equal_func(GPtrArray *haystack, gconstpointer needle,
        GEqualFunc equal_func, guint *index_)
{
    guint i;

    g_return_val_if_fail (haystack != NULL, FALSE);

    if (equal_func == NULL)
        equal_func = g_direct_equal;

    for (i = 0; i < haystack->len; i++) {
        if (equal_func(g_ptr_array_index (haystack, i), needle)) {
            if (index_ != NULL)
                *index_ = i;
            return TRUE;
        }
    }

    return FALSE;
}
#endif

GDateTime *
g_date_time_new_from_timeval(gint64 sec,  gint64 usec)
{
    g_autoptr(GDateTime) dt = g_date_time_new_from_unix_local(sec);
    return g_date_time_add(dt, usec);
}

GDateTime *
g_date_time_new_from_unix_usec(gint64 usec)
{
    GDateTime *dt = g_date_time_new_from_unix_local(usec / G_USEC_PER_SEC);
    return g_date_time_add(dt, usec - (usec / G_USEC_PER_SEC * G_USEC_PER_SEC));
}

gint
g_atoi(const gchar *number)
{
    g_return_val_if_fail(number != NULL, 0);
    gint64 number64 = g_ascii_strtoll(number, NULL, 10);
    if (number64 > G_MAXINT) {
        return G_MAXINT;
    } else if (number64 < G_MININT) {
        return G_MININT;
    } else {
        return  (gint) number64;
    }
}

guint64
g_format_size_to_bytes(const gchar *size)
{
    g_return_val_if_fail(size != NULL, 0);
    if (g_str_has_suffix(size, "K")) {
        return g_atoi(size) * G_BYTES_PER_KILOBYTE;
    } else if (g_str_has_suffix(size, "M")) {
        return g_atoi(size) * G_BYTES_PER_MEGABYTE;
    } else if (g_str_has_suffix(size, "G")) {
        return g_atoi(size) * G_BYTES_PER_GIGABYTE;
    } else if (g_str_has_suffix(size, "%")) {
        struct sysinfo info;
        return sysinfo(&info) == 0 ? info.totalram * g_atoi(size) / 100 : 0;
    } else {
        return g_atoi(size);
    }
}
