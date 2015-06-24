/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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
#endif /* __SNGREP_UTIL_H */
