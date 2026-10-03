/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
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
 * @file option.h
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

#include "config.h"
#ifdef WITH_PCRE
#include <pcre.h>
#elif defined(WITH_PCRE2)
#include <pcre2.h>
#else
#include <regex.h>
#endif
#include "sip.h"

//! Shorter declaration of sip_call_group structure
typedef struct filter filter_t;

/**
 * @brief Available filter types
 */
enum filter_type {
    //! SIP From header in packet payload
    FILTER_SIPFROM = 0,
    //! SIP To header in packet payload
    FILTER_SIPTO,
    //! Packet source address
    FILTER_SOURCE,
    //! Packet destination address
    FILTER_DESTINATION,
    //! SIP Call-ID header
    FILTER_CALLID,
    //! SIP X-Call-ID header in packet payload
    FILTER_XCALLID,
    //! SIP transport (UDP/TCP/TLS/WS/WSS)
    FILTER_TRANSPORT,
    //! SIP Method in packet payload
    FILTER_METHOD,
    //! SIP Payload in any call packet
    FILTER_PAYLOAD,
    //! Displayed line in call list
    FILTER_CALL_LIST,
    //! Number of available filter types
    FILTER_COUNT,
};

//! Bit of a call state in the call state filter mask @see call_state
#define FILTER_CALLSTATE_BIT(state) (1U << (state))

/**
 * @brief Filter information
 */
struct filter {
    //! The filter text
    char *expr;
#ifdef WITH_PCRE
    //! The filter compiled expression
    pcre *regex;
#elif defined(WITH_PCRE2)
    //! The filter compiled expression
    pcre2_code *regex;
    //! Reusable match data for the compiled pattern
    pcre2_match_data *match_data;
#else
    //! The filter compiled expression
    regex_t regex;
#endif
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
int
filter_set(int type, const char *expr);

/**
 * @brief Get filter text expression
 *
 * @param type filter type
 * @return filter text expressions
 */
const char *
filter_get(int type);

/**
 * @brief Check if a call if filtered
 *
 * @param call Call to be checked
 * @return 1 if call is filtered
 */
int
filter_check_call(void *item);

/**
 * @brief Check if data matches the filter regexp
 *
 * @return 0 if the given data matches the filter
 */
int
filter_check_expr(filter_t filter, const char *data);

/**
 * @brief Reset filtered flag in all calls
 *
 * This function can be used to force reevaluation
 * of filters in all calls.
 */
void
filter_reset_calls();

/**
 * @brief Set the call states filter
 *
 * Calls match this filter when their current state is any of
 * the given states. Dialogs without call state (not started with
 * an INVITE) never match an enabled call state filter.
 *
 * @param states Mask of FILTER_CALLSTATE_BIT values, 0 to disable the filter
 */
void
filter_set_callstates(unsigned int states);

/**
 * @brief Get the call states filter
 *
 * @return Mask of FILTER_CALLSTATE_BIT values, 0 if filter is disabled
 */
unsigned int
filter_get_callstates();

/**
 * @brief Parse a comma separated list of call state names
 *
 * Names are compared case insensitively with the output of
 * call_state_to_str. Unknown names are ignored.
 *
 * @param value Comma separated list of call states (i.e. "IN CALL,COMPLETED")
 * @return Mask of FILTER_CALLSTATE_BIT values
 */
unsigned int
filter_callstates_from_str(const char *value);

/**
 * @brief Notify a call state has changed
 *
 * Discard the cached filter result of the given call if it depends
 * on the call state, so it is evaluated again on next check.
 *
 * @param call Call whose state has changed
 */
void
filter_call_state_changed(sip_call_t *call);

#endif /* __SNGREP_FILTER_H_ */
