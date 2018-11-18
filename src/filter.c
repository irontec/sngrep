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
 * @file filter.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in filter.h
 *
 */
#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "glib-utils.h"
#include "storage.h"
#include "curses/screens/ui_call_list.h"
#include "filter.h"

//! Storage of filter information
filter_t filters[FILTER_COUNT] = { { 0 } };

int
filter_set(int type, const char *expr)
{
    GRegex *regex = NULL;

    // If we have an expression, check if compiles before changing the filter
    if (expr) {
        // Check if we have a valid expression
        regex = g_regex_new(expr, G_REGEX_CASELESS, 0, NULL);
        if (regex == NULL) {
            return 1;
        }
    }

    // Remove previous value
    if (filters[type].expr) {
        g_free(filters[type].expr);
        g_regex_unref(filters[type].regex);
    }

    // Set new expresion values
    filters[type].expr = (expr) ? strdup(expr) : NULL;
    filters[type].regex = regex;

    return 0;
}

const char *
filter_get(int type)
{
    return filters[type].expr;
}

gboolean
filter_check_call(gconstpointer item, G_GNUC_UNUSED gconstpointer user_data)
{
    int i;
    char data[MAX_SIP_PAYLOAD];
    SipCall *call = (SipCall*) item;
    SipMsg *msg;

    // Dont filter calls without messages
    if (call_msg_count(call) == 0)
        return FALSE;

    // Filter for this call has already be processed
    if (call->filtered != -1)
        return (call->filtered == 0);

    // By default, call matches all filters
    call->filtered = 0;

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
            case FILTER_METHOD:
                call_get_attribute(call, SIP_ATTR_METHOD, data);
                break;
            case FILTER_PAYLOAD:
                break;
            case FILTER_CALL_LIST:
                // FIXME Maybe call should know hot to calculate this line
                call_list_line_text(ui_find_by_type(PANEL_CALL_LIST), call, data);
                break;
            default:
                // Unknown filter id
                return FALSE;
        }

        // For payload filtering, check all messages payload
        if (i == FILTER_PAYLOAD) {
            // Assume this call doesn't match the filter
            call->filtered = 1;
            // Create an iterator for the call messages
            for (guint i = 0; i < g_ptr_array_len(call->msgs); i++) {
                msg = g_ptr_array_index(call->msgs, i);
                // Copy message payload
                strcpy(data, msg_get_payload(msg));
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
    return (g_regex_match(filter.regex, data, 0, NULL)) ? 0 : 1;
}

static void
filter_mark_call_unfiltered(SipCall *call, G_GNUC_UNUSED gpointer user_data)
{
    call->filtered = -1;
}

void
filter_reset_calls()
{
    g_ptr_array_foreach(storage_calls(), (GFunc) filter_mark_call_unfiltered, NULL);
}
