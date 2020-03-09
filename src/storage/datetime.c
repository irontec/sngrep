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
 * @file timeval.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in util.h
 *
 */
#include "config.h"
#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "glib-extra/glib.h"
#include "datetime.h"

const gchar *
date_time_date_to_str(guint64 time, gchar *out)
{
    GDate date;
    g_date_set_time_t(&date, time);
    g_date_strftime(out, 11, "%Y/%m/%d", &date);
    return out;
}


const gchar *
date_time_time_to_str(guint64 ts, gchar *out)
{
    g_autoptr(GDateTime) time = g_date_time_new_from_unix_usec(ts);

    g_sprintf(out, "%s.%06d",
              g_date_time_format(time, "%H:%M:%S"),
              g_date_time_get_microsecond(time)
    );

    return out;
}

const gchar *
date_time_to_duration(guint64 start, guint64 end, gchar *out)
{
    g_return_val_if_fail(out != NULL, NULL);

    if (start == 0 || end == 0)
        return NULL;

    g_autoptr(GDateTime) start_time = g_date_time_new_from_unix_usec(start);
    g_autoptr(GDateTime) end_time = g_date_time_new_from_unix_usec(end);

    // Difference in seconds
    glong seconds = g_date_time_difference(end_time, start_time) / G_USEC_PER_SEC;

    // Set Human readable format
    g_sprintf(out, "%lu:%02lu", seconds / 60, seconds % 60);
    return out;
}

const gchar *
date_time_to_delta(guint64 start, guint64 end, gchar *out)
{
    g_return_val_if_fail(out != NULL, NULL);

    if (start == 0 || end == 0)
        return NULL;

    g_autoptr(GDateTime) start_time = g_date_time_new_from_unix_usec(start);
    g_autoptr(GDateTime) end_time = g_date_time_new_from_unix_usec(end);

    glong diff = g_date_time_difference(end_time, start_time);

    glong nsec = diff / G_USEC_PER_SEC;
    glong nusec = labs(diff - (nsec * G_USEC_PER_SEC));

    gint sign = (diff >= 0) ? '+' : '-';

    g_sprintf(out, "%c%ld.%06lu", sign, labs(nsec), nusec);
    return out;
}

gdouble
date_time_to_unix_ms(guint64 ts)
{
    return ((gdouble) ts / G_MSEC_PER_SEC);
}
