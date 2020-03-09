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
 * @file glib-extra.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper function for glib containers
 *
 */

#ifndef __SNGREP_GLIB_EXTRA_H
#define __SNGREP_GLIB_EXTRA_H

#include <glib.h>
#include "glib-extra/glist.h"
#include "glib-extra/gptrarray.h"
#include "glib-extra/gasyncqueuesource.h"
#include "glib-extra/gdatetime.h"

#define G_OPTION_SENTINEL NULL, 0, 0, 0, NULL, NULL, NULL

#define G_BYTES_PER_KILOBYTE    1024
#define G_BYTES_PER_MEGABYTE    G_BYTES_PER_KILOBYTE * 1024
#define G_BYTES_PER_GIGABYTE    G_BYTES_PER_MEGABYTE * 1024

#define g_byte_array_len(array) (array->len)

gint
g_atoi(const gchar *number);

gsize
g_format_size_to_bytes(const gchar *size);

GByteArray *
g_byte_array_offset(GByteArray *array, guint offset);

#endif //__SNGREP_GLIB_EXTRA_H
