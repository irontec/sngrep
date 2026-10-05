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
 * @file filter.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in filter.h
 *
 */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "sip.h"
#include "curses/ui_call_list.h"
#include "filter.h"

//! Storage of filter information
filter_t filters[FILTER_COUNT] = { 0 };

//! Mask of call states displayed calls must be in (0 = disabled)
static unsigned int filter_callstates = 0;

int
filter_set(int type, const char *expr)
{
#ifdef WITH_PCRE
    pcre *regex = NULL;

    // If we have an expression, check if compiles before changing the filter
    if (expr) {
        const char *re_err = NULL;
        int32_t err_offset;
        int32_t pcre_options = PCRE_UNGREEDY | PCRE_CASELESS;

        // Check if we have a valid expression
        if (!(regex = pcre_compile(expr, pcre_options, &re_err, &err_offset, 0)))
            return 1;
    }

    // Remove previous value
    if (filters[type].expr) {
        free(filters[type].expr);
        pcre_free(filters[type].regex);
    }

    // Set new expresion values
    filters[type].expr = (expr) ? strdup(expr) : NULL;
    filters[type].regex = regex;
#elif defined(WITH_PCRE2)
    pcre2_code *regex = NULL;
    pcre2_match_data *match_data = NULL;

    // If we have an expression, check if compiles before changing the filter
    if (expr) {
        int re_err = 0;
        PCRE2_SIZE err_offset = 0;
        uint32_t pcre_options = PCRE2_UNGREEDY | PCRE2_CASELESS;

        // Check if we have a valid expression
        if (!(regex = pcre2_compile((PCRE2_SPTR) expr, PCRE2_ZERO_TERMINATED, pcre_options, &re_err, &err_offset, NULL)))
            return 1;

        // Match data only depends on the compiled pattern. Reuse it for all
        // calls instead of allocating and freeing it for every match.
        if (!(match_data = pcre2_match_data_create_from_pattern(regex, NULL))) {
            pcre2_code_free(regex);
            return 1;
        }
    }

    // Remove previous value
    if (filters[type].expr) {
        free(filters[type].expr);
        pcre2_code_free(filters[type].regex);
        pcre2_match_data_free(filters[type].match_data);
    }

    // Set new expresion values
    filters[type].expr = (expr) ? strdup(expr) : NULL;
    filters[type].regex = regex;
    filters[type].match_data = match_data;
#else
    regex_t regex;
    // If we have an expression, check if compiles before changing the filter
    if (expr) {
        // Check if we have a valid expression
        if (regcomp(&regex, expr, REG_EXTENDED | REG_ICASE) != 0)
            return 1;
    }

    // Remove previous value
    if (filters[type].expr) {
        free(filters[type].expr);
        regfree(&filters[type].regex);
    }

    // Set new expresion values
    filters[type].expr = (expr) ? strdup(expr) : NULL;
    memcpy(&filters[type].regex, &regex, sizeof(regex));
#endif

    return 0;
}

const char *
filter_get(int type)
{
    return filters[type].expr;
}

int
filter_check_call(void *item)
{
    int i;
    char data[MAX_SIP_PAYLOAD];
    sip_call_t *call = (sip_call_t*) item;
    sip_msg_t *msg;
    vector_iter_t it;

    // Dont filter calls without messages
    if (call_msg_count(call) == 0)
        return 0;

    // Filter for this call has already be processed
    if (call->filtered != -1)
        return (call->filtered == 0);

    // By default, call matches all filters
    call->filtered = 0;

    // Check call state filter
    if (filter_callstates && !(filter_callstates & FILTER_CALLSTATE_BIT(call->state))) {
        call->filtered = 1;
        return 0;
    }

    // Check all filter types
    for (i=0; i < FILTER_COUNT; i++) {
        // If filter is not enabled, go to the next
        if (!filters[i].expr)
            continue;

        // Initialize
        memset(data, 0, sizeof(data));

        // Get filtered field
        switch(i) {
            case FILTER_SIPFROM:
                call_get_attribute(call, SIP_ATTR_SIPFROM, data);
                break;
            case FILTER_SIPTO:
                call_get_attribute(call, SIP_ATTR_SIPTO, data);
                break;
            case FILTER_SOURCE:
                call_get_attribute(call, SIP_ATTR_SRC, data);
                break;
            case FILTER_DESTINATION:
                call_get_attribute(call, SIP_ATTR_DST, data);
                break;
            case FILTER_CALLID:
                call_get_attribute(call, SIP_ATTR_CALLID, data);
                break;
            case FILTER_XCALLID:
                call_get_attribute(call, SIP_ATTR_XCALLID, data);
                break;
            case FILTER_TRANSPORT:
                call_get_attribute(call, SIP_ATTR_TRANSPORT, data);
                break;
            case FILTER_METHOD:
                call_get_attribute(call, SIP_ATTR_METHOD, data);
                break;
            case FILTER_PAYLOAD:
                break;
            case FILTER_CALL_LIST:
                // FIXME Maybe call should know how to calculate this line
                call_list_search_text(ui_find_by_type(PANEL_CALL_LIST), call, data, sizeof(data));
                break;
            default:
                // Unknown filter id
                return 0;
        }

        // For payload filtering, check all messages payload
        if (i == FILTER_PAYLOAD) {
            // Assume this call doesn't match the filter
            call->filtered = 1;
            // Create an iterator for the call messages
            it = vector_iterator(call->msgs);
            while ((msg = vector_iterator_next(&it))) {
                // Copy message payload
                sng_strlcpy(data, msg_get_payload(msg), sizeof(data));
                // Check if this payload matches the filter
                if (filter_check_expr(filters[i], data) == 0) {
                    call->filtered = 0;
                    break;
                }
            }
            if (call->filtered == 1)
                break;
        } else {
            // Check the filter against given data
            if (filter_check_expr(filters[i], data) != 0) {
                // The data didn't matched the filter
                call->filtered = 1;
                break;
            }
        }
    }

    // Return the final filter status
    return (call->filtered == 0);
}

int
filter_check_expr(filter_t filter, const char *data)
{
#ifdef WITH_PCRE
        return pcre_exec(filter.regex, 0, data, strlen(data), 0, 0, 0, 0);
#elif defined(WITH_PCRE2)
    int ret = pcre2_match(filter.regex, (PCRE2_SPTR) data, (PCRE2_SIZE) strlen(data), 0, 0, filter.match_data, NULL);
    return (ret == PCRE2_ERROR_NOMATCH) ? 1 : 0;
#else
        // Call doesn't match this filter
        return regexec(&filter.regex, data, 0, NULL, 0);
#endif
}

void
filter_reset_calls()
{
    sip_call_t *call;
    vector_iter_t calls = sip_calls_iterator();

    // Force filter evaluation
    while ((call = vector_iterator_next(&calls)))
        call->filtered = -1;
}

void
filter_set_callstates(unsigned int states)
{
    filter_callstates = states;
}

unsigned int
filter_get_callstates()
{
    return filter_callstates;
}

unsigned int
filter_callstates_from_str(const char *value)
{
    char states[256];
    char *name, *saveptr = NULL;
    unsigned int mask = 0;
    int state;

    if (!value)
        return 0;

    sng_strlcpy(states, value, sizeof(states));

    for (name = strtok_r(states, ",", &saveptr); name; name = strtok_r(NULL, ",", &saveptr)) {
        // Ignore spaces around state names
        while (*name == ' ')
            name++;
        strtrim(name);

        // Check against all known call states
        for (state = SIP_CALLSTATE_CALLSETUP; *call_state_to_str(state); state++) {
            if (!strcasecmp(name, call_state_to_str(state))) {
                mask |= FILTER_CALLSTATE_BIT(state);
                break;
            }
        }
    }

    return mask;
}

void
filter_call_state_changed(sip_call_t *call)
{
    // Cached result only depends on call state when the filter is enabled
    if (filter_callstates)
        call->filtered = -1;
}
