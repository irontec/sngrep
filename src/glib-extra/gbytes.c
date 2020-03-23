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
 * @file gptrarray.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper function for GPtrArray containers
 *
 */
#include "config.h"
#include "gbytes.h"

GByteArray *
g_byte_array_copy(GByteArray *array)
{
    g_return_val_if_fail(array != NULL, NULL);
    GByteArray *copy = g_byte_array_sized_new(g_byte_array_len(array));
    g_byte_array_append(copy, array->data, array->len);
    return copy;
}

GByteArray *
g_byte_array_offset(GByteArray *array, guint offset)
{
    g_return_val_if_fail(array->len >= offset, NULL);
    array->data += offset;
    array->len -= offset;
    return array;
}

GBytes *
g_bytes_offset(GBytes *bytes, gsize offset)
{
    g_return_val_if_fail(bytes != NULL, bytes);
    g_return_val_if_fail(offset <= g_bytes_get_size(bytes), bytes);

    gsize len = g_bytes_get_size(bytes);
    GBytes *ret = g_bytes_new_from_bytes(bytes, offset, len - offset);
    g_bytes_unref(bytes);
    return ret;
}

GBytes *
g_bytes_set_size(GBytes *bytes, gsize count)
{
    g_return_val_if_fail(bytes != NULL, bytes);
    g_return_val_if_fail(count <= g_bytes_get_size(bytes), bytes);

    GBytes *ret = g_bytes_new_from_bytes(bytes, 0, count);
    g_bytes_unref(bytes);
    return ret;
}
