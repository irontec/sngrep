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
#include "datetime.h"

const gchar *
date_time_date_to_str(GDateTime *time, gchar *out)
{
    GDate date;
    g_date_set_time_t(&date, g_date_time_to_unix(time));
    g_date_strftime(out, 11, "%Y/%m/%d", &date);
    return out;
}


const gchar *
date_time_time_to_str(GDateTime *time, gchar *out)
{
    g_sprintf(out, "%s.%06d",
              g_date_time_format(time, "%H:%M:%S"),
              g_date_time_get_microsecond(time)
    );
    return out;
}

const gchar *
date_time_to_duration(GDateTime *start, GDateTime *end, gchar *out)
{
    g_return_val_if_fail(out != NULL, NULL);

    if (start == NULL || end == NULL)
        return NULL;

    // Differnce in secons
    glong seconds = g_date_time_difference(end, start) / G_USEC_PER_SEC;

    // Set Human readable format
    g_sprintf(out, "%lu:%02lu", seconds / 60, seconds % 60);
    return out;
}

const gchar *
date_time_to_delta(GDateTime *start, GDateTime *end, gchar *out)
{
    g_return_val_if_fail(out != NULL, NULL);

    if (start == NULL || end == NULL)
        return NULL;

    glong diff = g_date_time_difference(end, start);

    glong nsec = diff / G_USEC_PER_SEC;
    glong nusec = labs(diff - (nsec * G_USEC_PER_SEC));

    gint sign = (diff >= 0) ? '+' : '-';

    g_sprintf(out, "%c%ld.%06lu", sign, labs(nsec), nusec);
    return out;
}

gdouble
date_time_to_unix_ms(GDateTime *time)
{
    return g_date_time_to_unix(time) * G_MSEC_PER_SEC
           + ((gdouble) g_date_time_get_microsecond(time) / G_MSEC_PER_SEC);
}
