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
 * @file filter.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in filter.h
 *
 */
#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include "glib/glib-extra.h"
#include "storage.h"
#include "ncurses/windows/call_list_win.h"
#include "filter.h"

//! Storage of filter information
Filter filters[FILTER_COUNT] = {{ 0 }};

gboolean
filter_set(enum FilterType type, const gchar *expr)
{
    GRegex *regex = NULL;

    // If we have an expression, check if compiles before changing the filter
    if (expr != NULL) {
        // Check if we have a valid expression
        regex = g_regex_new(expr, G_REGEX_CASELESS, 0, NULL);
        if (regex == NULL) {
            return FALSE;
        }
    }

    // Remove previous value
    if (filters[type].expr) {
        g_free(filters[type].expr);
        g_regex_unref(filters[type].regex);
    }

    // Set new expresion values
    filters[type].expr = (expr) ? g_strdup(expr) : NULL;
    filters[type].regex = regex;

    return FALSE;
}

const gchar *
filter_get(enum FilterType type)
{
    return filters[type].expr;
}

gboolean
filter_check_call(Call *call, G_GNUC_UNUSED gconstpointer user_data)
{
    // Dont filter calls without messages
    if (call_msg_count(call) == 0)
        return FALSE;

    // Filter for this call has already be processed
    if (call->filtered != -1)
        return (call->filtered == 0);

    // By default, call matches all filters
    call->filtered = 0;

    // Filter attributes based on first call message
    Message *msg = g_ptr_array_first(call->msgs);
    g_return_val_if_fail(msg != NULL, FALSE);

    // Check all filter types
    for (guint filter_type = 0; filter_type < FILTER_COUNT; filter_type++) {

        // If filter is not enabled, go to the next
        if (!filters[filter_type].expr)
            continue;

        // Initialize
        const gchar *data = NULL;

        // Get filtered field
        switch (filter_type) {
            case FILTER_SIPFROM:
                data = msg_get_attribute(msg, ATTR_SIPFROM);
                break;
            case FILTER_SIPTO:
                data = msg_get_attribute(msg, ATTR_SIPTO);
                break;
            case FILTER_SOURCE:
                data = msg_get_attribute(msg, ATTR_SRC);
                break;
            case FILTER_DESTINATION:
                data = msg_get_attribute(msg, ATTR_DST);
                break;
            case FILTER_METHOD:
                data = msg_get_attribute(msg, ATTR_METHOD);
                break;
            case FILTER_PAYLOAD:
                break;
            case FILTER_CALL_LIST:
                // FIXME Maybe call should know hot to calculate this line
                data = call_list_win_line_text(ncurses_find_by_type(WINDOW_CALL_LIST), call);
                break;
            default:
                // Unknown filter id
                return FALSE;
        }

        // For payload filtering, check all messages payload
        if (filter_type == FILTER_PAYLOAD) {
            // Assume this call doesn't match the filter
            call->filtered = 1;
            // Create an iterator for the call messages
            for (guint j = 0; j < g_ptr_array_len(call->msgs); j++) {
                msg = g_ptr_array_index(call->msgs, j);
                // Check if this payload matches the filter
                if (filter_check_expr(filters[j], msg_get_payload(msg)) == 0) {
                    call->filtered = 0;
                    break;
                }
            }
            if (call->filtered == 1)
                break;
        } else {
            // Check the filter against given data
            if (filter_check_expr(filters[filter_type], data) != 0) {
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
filter_check_expr(Filter filter, const gchar *data)
{
    return (g_regex_match(filter.regex, data, 0, NULL)) ? 0 : 1;
}

static void
filter_mark_call_unfiltered(Call *call, G_GNUC_UNUSED gpointer user_data)
{
    call->filtered = -1;
}

void
filter_reset_calls()
{
    g_ptr_array_foreach(storage_calls(), (GFunc) filter_mark_call_unfiltered, NULL);
}

void
filter_method_from_setting(const gchar *value)
{
    // If there's a method filter
    if (value != NULL && strlen(value) > 0) {
        gchar *methods = g_strdup_printf("(%s)", value);

        // Replace all commas with pipes
        gchar *comma = NULL;
        while ((comma = strchr(methods, ','))) {
            *comma = '|';
        }

        // Create a regular expression
        filter_set(FILTER_METHOD, methods);

        // Free me
        g_free(methods);
    } else {
        filter_set(FILTER_METHOD, " ");
    }
}

void
filter_payload_from_setting(const gchar *value)
{
    if (value) {
        filter_set(FILTER_PAYLOAD, value);
    }
}
