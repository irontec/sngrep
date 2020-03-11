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
 * @file glib-extra-extra.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper function for glib containers
 *
 */
#include "config.h"
#include <sys/sysinfo.h>
#include "glib.h"

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
        return (gint) number64;
    }
}

gsize
g_format_size_to_bytes(const gchar *size)
{
    g_return_val_if_fail(size != NULL, 0);

    gchar *units = NULL;
    guint64 number = g_ascii_strtoll(size, &units, 10);
    if (g_str_has_suffix(units, "K")
        || g_str_has_suffix(units, "KB")
        || g_str_has_suffix(units, "KiB")) {
        return number * G_BYTES_PER_KILOBYTE;
    } else if (
        g_str_has_suffix(units, "M")
        || g_str_has_suffix(units, "MB")
        || g_str_has_suffix(units, "MiB")) {
        return number * G_BYTES_PER_MEGABYTE;
    } else if (
        g_str_has_suffix(size, "G")
        || g_str_has_suffix(size, "GB")
        || g_str_has_suffix(size, "GiB")) {
        return number * G_BYTES_PER_GIGABYTE;
    } else if (g_str_has_suffix(size, "%")) {
        struct sysinfo info;
        return sysinfo(&info) == 0 ? info.totalram * number / 100 : 0;
    } else {
        return number;
    }
}

