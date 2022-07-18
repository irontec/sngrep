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
 * @file util.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in util.h
 *
 */
#include "config.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include "util.h"

#if __STDC_VERSION__ >= 201112L && __STDC_NO_ATOMICS__ != 1
// modern C with atomics
#include <stdatomic.h>
typedef atomic_int signal_flag_type;
#else
// no atomics available
typedef volatile sig_atomic_t signal_flag_type;
#endif

static signal_flag_type sigterm_received = 0;

static void sigterm_handler(int signum)
{
    sigterm_received = 1;
}

void setup_sigterm_handler(void)
{
    // set up SIGTERM handler (also used for SIGINT and SIGQUIT)
    // the handler will be served by any of the running threads
    // so we just set a flag and check it in one of the two available
    // main loops of the program (main for --no-interface and
    // ui_wait_for_input() for curses)

    if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
        exit(EXIT_FAILURE);
    if (signal(SIGINT, sigterm_handler) == SIG_ERR)
        exit(EXIT_FAILURE);
    if (signal(SIGQUIT, sigterm_handler) == SIG_ERR)
        exit(EXIT_FAILURE);
}

bool was_sigterm_received(void)
{
    return (sigterm_received == 1);
}

void *
sng_malloc(size_t size)
{
    void *data;

    // Check memory allocation size
    if (size <= 0 || size > MALLOC_MAX_SIZE)
        return NULL;

    // Allocate memory
    if (!(data = malloc(size)))
        return NULL;

    // Initialize allocated memory
    memset(data, 0, size);
    return data;
}

void
sng_free(void *ptr)
{
    if (ptr)
        free(ptr);
}

char *
sng_basename(const char *name)
{
    const char *base = name;

    while (*name)
    {
        if (*name++ == '/') {
            base = name;
        }
    }
    return (char *) base;
}

int
timeval_is_older(struct timeval t1, struct timeval t2)
{
    long long int t1sec, t2sec;
    t1sec = t1.tv_sec;
    t1sec = t1sec * 1000000;
    t2sec = t2.tv_sec;
    t2sec = t2sec * 1000000;
    return ((t2sec + t2.tv_usec) - (t1sec + t1.tv_usec) <= 0);
}

const char *
timeval_to_date(struct timeval time, char *out)
{
    time_t t = (time_t) time.tv_sec;
    struct tm *timestamp = localtime(&t);
    strftime(out, 11, "%Y/%m/%d", timestamp);
    return out;
}


const char *
timeval_to_time(struct timeval time, char *out)
{
    time_t t = (time_t) time.tv_sec;
    struct tm *timestamp = localtime(&t);
    strftime(out, 19, "%H:%M:%S", timestamp);
    sprintf(out + 8, ".%06d", (int) time.tv_usec);
    return out;
}

const char *
timeval_to_duration(struct timeval start, struct timeval end, char *out)
{
    int seconds;
    char duration[20];

    if (!out || !start.tv_sec || !end.tv_sec)
        return NULL;

    // Differnce in secons
    seconds = end.tv_sec - start.tv_sec;
    // Set Human readable format
    sprintf(duration, "%d:%02d", seconds / 60, seconds % 60);
    sprintf(out, "%7s", duration);
    return out;
}

const char *
timeval_to_delta(struct timeval start, struct timeval end, char *out)
{
    long diff;
    int nsec, nusec;
    int sign;

    if (!out || !start.tv_sec || !end.tv_sec)
        return NULL;

    diff = end.tv_sec  * 1000000 + end.tv_usec;
    diff -= start.tv_sec * 1000000 + start.tv_usec;

    nsec = diff / 1000000;
    nusec = labs(diff - (nsec * 1000000));

    sign = (diff >= 0) ? '+' : '-';

    sprintf(out, "%c%d.%06d", sign, abs(nsec), nusec);
    return out;
}
char *
strtrim(char *str)
{
    int i;

    if (!str || !strlen(str))
        return str;

    for (i = strlen(str) - 1; i >= 0 && isspace(str[i]); i--)
        str[i] = 0;

    return str;
}
