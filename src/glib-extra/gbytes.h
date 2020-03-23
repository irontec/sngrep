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
 * @brief Helper function for GBytes and GByteArray containers
 *
 */

#ifndef __SNGREP_GLIB_GBYTES_H
#define __SNGREP_GLIB_GBYTES_H

#include <glib.h>

G_BEGIN_DECLS

#define G_BYTES_PER_KILOBYTE    1024
#define G_BYTES_PER_MEGABYTE    G_BYTES_PER_KILOBYTE * 1024
#define G_BYTES_PER_GIGABYTE    G_BYTES_PER_MEGABYTE * 1024

#define g_byte_array_len(array) (array->len)

GByteArray *
g_byte_array_copy(GByteArray *array);

GByteArray *
g_byte_array_offset(GByteArray *array, guint offset);

GBytes *
g_bytes_offset(GBytes *bytes, gsize offset);

GBytes *
g_bytes_set_size(GBytes *bytes, gsize count);

G_END_DECLS

#endif//__SNGREP_GLIB_GBYTES_H
