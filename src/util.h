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
 * @file util.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions for general use in the program
 *
 */
#ifndef __SNGREP_UTIL_H
#define __SNGREP_UTIL_H

// Capture headers has some fixes for pcap timevals in BSD systems
#include "capture.h"

// Max Memmory allocation
#define MALLOC_MAX_SIZE 102400

// Stringify numbers for concatenation
#define STRINGIFY_ARG(x)    #x
#define STRINGIFY(x)        STRINGIFY_ARG(x)

/**
 * @brief Wrapper for memory allocation
 */
void *
sng_malloc(size_t size);

/**
 * @brief Wrapper for memmory deallocation
 */
void
sng_free(void *ptr);

/*
 * @brief Generic implementation of basename
 */
char *
sng_basename(const char *name);

/**
 * @brief Compare two timeval structures
 *
 * @param t1 First timeval structure
 * @param t2 Second timval structure
 * @return 1 if t1 > t2, 0 if t1 <= t2
 */
int
timeval_is_older(struct timeval t1, struct timeval t2);

/**
 * @brief Convert timeval to yyyy/mm/dd format
 */
const char *
timeval_to_date(struct timeval time, char *out);

/**
 * @brief Convert timeval to HH:MM:SS.mmmmmm format
 */
const char *
timeval_to_time(struct timeval time, char *out);

/**
 * @brief Calculate the time difference between two timeval
 *
 * @return Human readable time difference in mm:ss format
 */
const char *
timeval_to_duration(struct timeval start, struct timeval end, char *out);

/**
 * @brief Convert timeval diference to +mm:ss.mmmmmm
 */
const char *
timeval_to_delta(struct timeval start, struct timeval end, char *out);

/**
 * @brief Return a given string without trailing spaces
 */
char *
strtrim(char *str);

/**
 * @brief Set up handler for SIGTERM, SIGINT and SIGQUIT
 */
void setup_sigterm_handler(void);

/**
 * @brief Check if SIGTERM, SIGINT or SIGQUIT were received
 *
 * @return true if any of the exit signals were received
 */
bool was_sigterm_received(void);

#endif /* __SNGREP_UTIL_H */
