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
 * @file glist.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper functions for GList and GSList containers
 */

#ifndef __SNGREP_GLIB_GLIST_H
#define __SNGREP_GLIB_GLIST_H

#include <glib.h>

#define g_list_first_data(list) g_list_first(list)->data
#define g_list_last_data(list) g_list_last(list)->data

#define g_slist_first_data(list) g_slist_nth_data(list, 0)

/**
 * @brief Make a deep concat from one Double-Linked list to another
 *
 * @param dst Destination list. May already have values or be NULL
 * @param src Source list. May be empty or be NULL
 *
 * @return Destination list with source nodes appended
 */
GList *
g_list_concat_deep(GList *dst, GList *src);

/**
 * @brief Wrapper function for freeing list items using g_list_foreach
 */
void
g_list_item_free(gpointer item, gpointer user_data);

#endif //__SNGREP_GLIB_GLIST_H
