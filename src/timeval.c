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
 * @file timeval.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in util.h
 *
 */
#include "config.h"
#include <glib.h>
#include <ctype.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include "timeval.h"

gint
timeval_is_older(GTimeVal t1, GTimeVal t2)
{
    if (t1.tv_sec > t2.tv_sec) {
        return 1;
    }

    if (t1.tv_sec < t2.tv_sec) {
        return -1;
    }

    if(t1.tv_usec == t2.tv_usec) {
        return 0;
    }

    if (t1.tv_usec < t2.tv_usec) {
        return -1;
    }

    return 1;
}

const gchar *
timeval_to_date(GTimeVal time, gchar *out)
{
    GDate date;
    g_date_set_time_val(&date, &time);
    g_date_strftime(out, 11, "%Y/%m/%d", &date);
    return out;
}


const char *
timeval_to_time(GTimeVal time, char *out)
{
    GDateTime *datetime = g_date_time_new_from_timeval_local(&time);
    g_sprintf(out, "%s.%06d",
              g_date_time_format(datetime, "%H:%M:%S"),
              (guint) time.tv_usec
    );
    g_date_time_unref(datetime);
    return out;
}

const char *
timeval_to_duration(GTimeVal start, GTimeVal end, char *out)
{
    gint seconds;
    gchar duration[20];

    if (!out || !start.tv_sec || !end.tv_sec)
        return NULL;

    // Differnce in secons
    seconds = end.tv_sec - start.tv_sec;
    // Set Human readable format
    g_sprintf(duration, "%d:%02d", seconds / 60, seconds % 60);
    g_sprintf(out, "%7s", duration);
    return out;
}

const gchar *
timeval_to_delta(GTimeVal start, GTimeVal end, gchar *out)
{
    glong diff;
    gint nsec, nusec;
    gint sign;

    if (!out || !start.tv_sec || !end.tv_sec)
        return NULL;

    diff = end.tv_sec  * 1000000 + end.tv_usec;
    diff -= start.tv_sec * 1000000 + start.tv_usec;

    nsec = diff / 1000000;
    nusec = labs(diff - (nsec * 1000000));

    sign = (diff >= 0) ? '+' : '-';

    g_sprintf(out, "%c%d.%06d", sign, abs(nsec), nusec);
    return out;
}
