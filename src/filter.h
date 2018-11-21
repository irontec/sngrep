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
 * @file filter.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage filtering options
 *
 * There are two types of filters: capture and display.
 *
 * Capture filters are handled in capture functions and they limit the
 * number of calls are created in sip storage.
 *
 * Display filters are handled in this file and they limit the number of
 * calls that are displayed to the user. Multiple display filters can be
 * set at the same time. In order to be valid, a call MUST match all the
 * enabled filters to be shown.
 *
 */

#ifndef __SNGREP_FILTER_H_
#define __SNGREP_FILTER_H_

#include <glib.h>
#include "storage.h"

//! Shorter declaration of sip_call_group structure
typedef struct _Filter Filter;

/**
 * @brief Available filter types
 */
enum FilterType {
    //! SIP From header in packet payload
    FILTER_SIPFROM = 0,
    //! SIP To header in packet payload
    FILTER_SIPTO,
    //! Packet source address
    FILTER_SOURCE,
    //! Packet destination address
    FILTER_DESTINATION,
    //! SIP Method in packet payload
    FILTER_METHOD,
    //! SIP Payload in any call packet
    FILTER_PAYLOAD,
    //! Displayed line in call list
    FILTER_CALL_LIST,
    //! Number of available filter types
    FILTER_COUNT,
};

/**
 * @brief Filter information
 */
struct _Filter {
    //! The filter text
    gchar *expr;
    //! The filter compiled expression
    GRegex *regex;
};

/**
 * @brief Set a given filter expression
 *
 * This function is used to set the filter expression
 * on a given filter. If given expression is NULL
 * the filter will be removed.
 *
 * @param type Type of the filter
 * @param expr Regexpression to match
 * @return 0 if the filter is valid, 1 otherwise
 */
gboolean
filter_set(enum FilterType type, const gchar *expr);

/**
 * @brief Get filter text expression
 *
 * @param type filter type
 * @return filter text expressions
 */
const gchar *
filter_get(enum FilterType type);

/**
 * @brief Check if a call if filtered
 *
 * @param call Call to be checked
 * @return TRUE if call is filtered
 */
gboolean
filter_check_call(Call *call, gconstpointer user_data);

/**
 * @brief Check if data matches the filter regexp
 *
 * @return 0 if the given data matches the filter
 */
gint
filter_check_expr(Filter filter, const gchar *data);

/**
 * @brief Reset filtered flag in all calls
 *
 * This function can be used to force reevaluation
 * of filters in all calls.
 */
void
filter_reset_calls();

/**
 * @brief Set Method filtering from filter.methods setting format
 */
void
filter_method_from_setting(const gchar *value);

/**
 * @brief Set Payload filter from filter.payload setting
 */
void
filter_payload_from_setting(const gchar *value);

#endif /* __SNGREP_FILTER_H_ */
