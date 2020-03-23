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
 * @file call_flow_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions to handle Message arrows display window
 */

#include "config.h"
#include <glib.h>
#include "glib-extra/glib.h"
#include <storage/message.h>
#include <storage/stream.h>
#include "packet/packet_sip.h"
#include "capture/capture_pcap.h"
#include "tui/tui.h"
#include "tui/dialog.h"
#include "tui/windows/call_flow_win.h"
#include "tui/windows/call_raw_win.h"
#include "tui/windows/msg_diff_win.h"
#include "tui/windows/auth_validate_win.h"
#include "tui/windows/save_win.h"
#ifdef WITH_PULSE
#include "tui/windows/rtp_player_win.h"
#endif

/***
 *
 * Some basic ascii art of this panel.
 *
 * +--------------------------------------------------------+
 * |                     Title                              |
 * |   addr1  addr2  addr3  addr4 | Selected Raw Message    |
 * |   -----  -----  -----  ----- | preview                 |
 * | Tmst|      |      |      |   |                         |
 * | Tmst|----->|      |      |   |                         |
 * | Tmst|      |----->|      |   |                         |
 * | Tmst|      |<-----|      |   |                         |
 * | Tmst|      |      |----->|   |                         |
 * | Tmst|<-----|      |      |   |                         |
 * | Tmst|      |----->|      |   |                         |
 * | Tmst|      |<-----|      |   |                         |
 * | Tmst|      |------------>|   |                         |
 * | Tmst|      |<------------|   |                         |
 * |     |      |      |      |   |                         |
 * |     |      |      |      |   |                         |
 * |     |      |      |      |   |                         |
 * | Useful hotkeys                                         |
 * +--------------------------------------------------------+
 *
 */

G_DEFINE_TYPE(CallFlowWindow, call_flow_win, TUI_TYPE_WINDOW)

/**
 * @brief Return selected flow arrow
 *
 * User can select an arrow to compare times or payload with another
 * arrow. Don't confuse this with the current arrow (where the cursor is)
 *
 * @param ui UI Structure pointer
 * @return user selected arrow
 */
static CallFlowArrow *
call_flow_arrow_selected(Window *window)
{
    // Get panel info
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, NULL);

    // No selected call
    if (self->selected == -1)
        return NULL;

    return g_ptr_array_index(self->darrows, self->selected);

}

/**
 * @brief Return timestamp for given arrow
 *
 * This function is a wrapper to return arrow timestamp no matter what
 * type of arrow is passed. Arrows of different types store their times
 * in different locations.
 *
 * If pointer is invalid of arrow type doesn't match anything known, the
 * timestamp returned structure will be zero'd
 *
 * @param arrow Arrow structure pointer
 * @return timestamp for given arrow
 */
static guint64
call_flow_arrow_time(const CallFlowArrow *arrow)
{
    guint64 ts = 0;

    if (arrow) {
        if (arrow->type == CF_ARROW_SIP) {
            Message *msg = (Message *) arrow->item;
            ts = msg_get_time(msg);
        } else if (arrow->type == CF_ARROW_RTP) {
            Stream *stream = (Stream *) arrow->item;
            ts = stream_time(stream);
        }
    }

    return ts;
}

/**
 * @brief Sort arrows by timestamp
 *
 * This function acts as sorter for arrows vector. Each time a new arrow
 * is appended, it's sorted based on its timestamp.
 */
static gint
call_flow_arrow_time_sorter(const CallFlowArrow **a, const CallFlowArrow **b)
{
    return call_flow_arrow_time(*a) > call_flow_arrow_time(*b);
}

/**
 * @brief Filter displayed arrows based on configuration
 */
static int
call_flow_arrow_filter(void *item)
{
    CallFlowArrow *arrow = (CallFlowArrow *) item;

    // SIP arrows are never filtered
    if (arrow->type == CF_ARROW_SIP && setting_disabled(SETTING_CF_ONLYMEDIA)) {
        return 1;
    }

    // RTP arrows are only displayed when requested
    if (arrow->type == CF_ARROW_RTP && setting_enabled(SETTING_CF_MEDIA)) {
        return 1;
    }

    // Rest of the arrows are never displayed
    return 0;
}

/**
 * @brief Callback for searching arrows by item
 */
static gboolean
call_flow_arrow_find_item_cb(CallFlowArrow *arrow, gpointer item)
{
    return arrow->item == item;
}

/**
 * @brief Return the arrow of a SIP msg or RTP stream
 *
 * This function will try to find an existing arrow with a
 * message or stream equals to the giving pointer.
 *
 * @param window UI structure pointer
 * @param data Data to search in the arrow structure
 * @return a pointer to the found arrow or NULL
 */
static CallFlowArrow *
call_flow_arrow_find(Window *window, const void *data)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(data != NULL, NULL);

    guint index;
    if (g_ptr_array_find_with_equal_func(
        self->arrows, data,
        (GEqualFunc) call_flow_arrow_find_item_cb, &index)) {
        return g_ptr_array_index(self->arrows, index);
    }

    return NULL;
}

/**
 * @brief Create a new arrow of given type
 *
 * Allocate memory for a new arrow of the given type and associate the
 * item pointer. If the arrow already exists in the ui arrows vector
 * this function will return that arrow instead of creating a new one.
 *
 * This function WON'T add the arrow to any ui vector.
 *
 * @param window UI structure pointer
 * @param item Item pointer to associate to the arrow
 * @param type Type of arrow as defined in enum @CallFlowArrowType
 * @return an arrow pointer or NULL in case of error
 */
static CallFlowArrow *
call_flow_arrow_create(Window *window, void *item, CallFlowArrowType type)
{
    CallFlowArrow *arrow;

    if ((arrow = call_flow_arrow_find(window, item)) == NULL) {
        // Create a new arrow of the given type
        arrow = g_malloc0(sizeof(CallFlowArrow));
        arrow->type = type;
        arrow->item = item;
    }

    return arrow;
}

/**
 * @brief Get how many lines of screen an arrow will use
 *
 * Depending on the arrow type and panel display mode lines can
 * take more than two lines. This function will calculate how many
 * lines the arrow will use.
 *
 * @param window UI structure pointer
 * @param arrow Arrow structure to calculate height
 * @return height the arrow will have
 */
static int
call_flow_arrow_height(G_GNUC_UNUSED Window *window, const CallFlowArrow *arrow)
{
    if (arrow->type == CF_ARROW_SIP) {
        if (setting_enabled(SETTING_CF_ONLYMEDIA))
            return 0;
        if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_COMPRESSED)
            return 1;
        if (!msg_has_sdp(arrow->item))
            return 2;
        if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_OFF)
            return 2;
        if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_FIRST)
            return 2;
        if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_FULL)
            return msg_media_count(arrow->item) + 2;
    } else if (arrow->type == CF_ARROW_RTP || arrow->type == CF_ARROW_RTCP) {
        if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_COMPRESSED)
            return 1;
        if (setting_disabled(SETTING_CF_MEDIA))
            return 0;
        return 2;
    }

    return 0;
}

/**
 * @brief Return the SIP message associated with the arrow
 *
 * Return the SIP message. If the arrow is of type SIP msg, it will
 * return the message itself. If the arrow is of type RTP stream,
 * it will return the SIP msg that setups the stream.
 *
 * @param arrow Call Flow Arrow pointer
 * @return associated SIP message with the arrow
 */
static Message *
call_flow_arrow_message(const CallFlowArrow *arrow)
{
    g_return_val_if_fail(arrow != NULL, NULL);

    if (arrow->type == CF_ARROW_SIP) {
        return arrow->item;
    }

    if (arrow->type == CF_ARROW_RTP) {
        Stream *stream = arrow->item;
        return stream->msg;
    }

    return NULL;
}

static CallFlowArrow *
call_flow_arrow_find_prev_callid(Window *window, const CallFlowArrow *arrow)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(arrow->type == CF_ARROW_SIP, NULL);

    Message *msg = call_flow_arrow_message(arrow);

    // Given arrow index
    gint cur_idx = g_ptr_array_data_index(self->darrows, arrow);

    for (gint i = cur_idx - 1; i > 0; i--) {
        CallFlowArrow *prev = g_ptr_array_index(self->darrows, i);

        if (prev->type != CF_ARROW_SIP)
            continue;

        Message *prev_msg = call_flow_arrow_message(prev);
        g_return_val_if_fail(prev_msg != NULL, NULL);

        if (msg_get_call(msg) == msg_get_call(prev_msg)
            && msg_is_request(msg) == msg_is_request(prev_msg)
            && !msg_is_retransmission(prev_msg)) {

            return prev;
        }
    }

    return NULL;
}

/**
 * @brief Get a flow column data starting at a given column index
 *
 * @param window UI structure pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 * @param start Column index to start searching
 *
 * @return column structure pointer or NULL if not found
 */
static CallFlowColumn *
call_flow_column_get_first(Window *window, const Address addr)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, NULL);

    // Look for address or address:port ?
    gboolean match_port = address_get_port(addr) != 0;

    // Get alias value for given address
    const gchar *alias = setting_get_alias(address_get_ip(addr));

    for (GList *l = self->columns; l != NULL; l = l->next) {
        CallFlowColumn *column = l->data;

        // In compressed mode, we search using alias instead of address
        if (setting_enabled(SETTING_CF_SPLITCALLID)) {
            if (g_strcmp0(column->alias, alias) == 0) {
                return column;
            }
        } else {
            // Check if this column matches requested address
            if (match_port) {
                if (addressport_equals(column->addr, addr)) {
                    return column;
                }
            } else {
                // Dont check port
                if (address_equals(column->addr, addr)) {
                    return column;
                }
            }
        }
    }

    return NULL;
}

/**
 * @brief Get a flow column data starting at a given column index
 *
 * @param window UI structure pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 * @param start Column index to start searching
 *
 * @return column structure pointer or NULL if not found
 */
static CallFlowColumn *
call_flow_column_get_last(Window *window, const Address addr)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, NULL);

    // Look for address or address:port ?
    gboolean match_port = address_get_port(addr) != 0;

    // Get alias value for given address
    const gchar *alias = setting_get_alias(address_get_ip(addr));

    for (GList *l = g_list_last(self->columns); l != NULL; l = l->prev) {
        CallFlowColumn *column = l->data;

        // In compressed mode, we search using alias instead of address
        if (setting_enabled(SETTING_CF_SPLITCALLID)) {
            if (g_strcmp0(column->alias, alias) == 0) {
                return column;
            }
        } else {
            // Check if this column matches requested address
            if (match_port) {
                if (addressport_equals(column->addr, addr)) {
                    return column;
                }
            } else {
                // Dont check port
                if (address_equals(column->addr, addr)) {
                    return column;
                }
            }
        }
    }

    return NULL;
}

static gint
call_flow_column_sorter(CallFlowColumn *a, CallFlowColumn *b)
{
    return a->pos - b->pos;
}

static CallFlowColumn *
call_flow_column_create(Window *window, const Address addr)
{
    // Get Window info
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, NULL);

    // Create a new column
    CallFlowColumn *column = g_malloc0(sizeof(CallFlowColumn));
    g_return_val_if_fail(column != NULL, NULL);

    column->addr = addr;
    column->alias = setting_get_alias(address_get_ip(column->addr));

    // Check if column has externip
    const gchar *twin_ip = setting_get_externip(address_get_ip(column->addr));
    if (twin_ip != NULL) {
        Address twin_address = address_from_str(twin_ip);
        CallFlowColumn *twin = call_flow_column_get_first(window, twin_address);
        if (twin != NULL) {
            // Set position based on twin column
            twin->twin = column;
            column->twin = twin;
            column->pos = twin->pos + 1;
        }
        address_free(twin_address);
    }

    // Set position after last existing column
    if (column->twin == NULL) {
        if (g_list_length(self->columns)) {
            CallFlowColumn *last = g_list_last_data(self->columns);
            if (last != NULL) {
                column->pos = last->pos + CF_COLUMN_WIDTH;
            }
        }
    }

    // Add to columns list
    self->columns = g_list_insert_sorted(self->columns, column, (GCompareFunc) call_flow_column_sorter);

    return column;
}

void
call_flow_column_free(CallFlowColumn *column)
{
    g_free(column);
}

static void
call_flow_arrow_set_columns(Window *window, CallFlowArrow *arrow, CallFlowArrowDir dir)
{
    g_return_if_fail(window != NULL);
    g_return_if_fail(arrow != NULL);
    g_return_if_fail(arrow->type == CF_ARROW_SIP);

    // Get Window info
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);

    // Get arrow information
    Message *msg = arrow->item;

    if (dir == CF_ARROW_DIR_ANY) {
        // Try to reuse current call columns if found
        Call *call = msg_get_call(msg);
        g_return_if_fail(call != NULL);

        GPtrArray *msgs = call->msgs;
        for (guint i = 0; i < g_ptr_array_len(msgs); i++) {
            CallFlowArrow *msg_arrow = call_flow_arrow_find(window, g_ptr_array_index(msgs, i));

            if (msg_arrow == NULL
                || msg_arrow->type != CF_ARROW_SIP
                || msg_arrow->scolumn == NULL
                || msg_arrow->dcolumn == NULL) {
                continue;
            }

            if (msg_arrow == arrow)
                break;

            if (addressport_equals(msg_src_address(msg), msg_arrow->scolumn->addr)
                && addressport_equals(msg_dst_address(msg), msg_arrow->dcolumn->addr)) {
                arrow->scolumn = msg_arrow->scolumn;
                arrow->dcolumn = msg_arrow->dcolumn;
                break;
            }

            if (addressport_equals(msg_src_address(msg), msg_arrow->dcolumn->addr)
                && addressport_equals(msg_dst_address(msg), msg_arrow->scolumn->addr)) {
                arrow->scolumn = msg_arrow->dcolumn;
                arrow->dcolumn = msg_arrow->scolumn;
                break;
            }
        }

        // Fallback use any available arrow
        if (arrow->scolumn == NULL)
            arrow->scolumn = call_flow_column_get_first(window, msg_src_address(msg));

        if (arrow->dcolumn == NULL)
            arrow->dcolumn = call_flow_column_get_first(window, msg_dst_address(msg));

    } else if (dir == CF_ARROW_DIR_RIGHT) {
        arrow->scolumn = call_flow_column_get_first(window, msg_src_address(msg));

        GList *lscolumn = g_list_find(self->columns, arrow->scolumn);
        for (GList *l = lscolumn; l != NULL; l = l->next) {
            CallFlowColumn *dcolumn = l->data;
            if (addressport_equals(msg_dst_address(msg), dcolumn->addr)) {
                arrow->dcolumn = dcolumn;

                // Check if there is a source column with src address is nearer
                for (GList *m = l; m != NULL; m = m->prev) {
                    CallFlowColumn *scolumn = m->data;
                    if (addressport_equals(msg_src_address(msg), scolumn->addr)) {
                        arrow->scolumn = scolumn;
                        break;
                    }
                }

                break;
            }
        }

        // If we need to create destination arrow, use nearest source column to the end
        if (arrow->dcolumn == NULL)
            arrow->scolumn = call_flow_column_get_last(window, msg_src_address(msg));

    } else if (dir == CF_ARROW_DIR_LEFT) {
        arrow->scolumn = call_flow_column_get_last(window, msg_src_address(msg));

        GList *lscolumn = g_list_find(self->columns, arrow->scolumn);
        for (GList *l = lscolumn; l != NULL; l = l->prev) {
            CallFlowColumn *dcolumn = l->data;
            if (addressport_equals(msg_dst_address(msg), dcolumn->addr)) {
                arrow->dcolumn = dcolumn;

                // Check if there is a destination column with dst address is nearer
                for (GList *m = l; m != NULL; m = m->next) {
                    CallFlowColumn *scolumn = m->data;
                    if (addressport_equals(msg_src_address(msg), scolumn->addr)) {
                        arrow->scolumn = scolumn;
                        break;
                    }
                }

                break;
            }
        }

        // If we need to create destination arrow, use nearest source column to the end
        if (arrow->scolumn == NULL)
            arrow->dcolumn = call_flow_column_get_last(window, msg_dst_address(msg));
    }

    // Create any non-existent columns
    if (arrow->scolumn == NULL)
        arrow->scolumn = call_flow_column_create(window, msg_src_address(msg));

    if (arrow->dcolumn == NULL)
        arrow->dcolumn = call_flow_column_create(window, msg_dst_address(msg));
}

static gint
call_flow_win_columns_width(Window *window)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, 0);

    return g_list_length(self->columns) * CF_COLUMN_WIDTH + 3;
}

static gint
call_flow_win_arrows_height(Window *window)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, 0);

    gint height = 0;
    for (guint i = 0; i < g_ptr_array_len(self->darrows); i++) {
        CallFlowArrow *arrow = g_ptr_array_index(self->darrows, i);
        height += call_flow_arrow_height(window, arrow);
    }

    return height;
}

/**
 * @brief Draw the footer of the panel with keybindings info
 *
 * @param window UI structure pointer
 */
static void
call_flow_win_draw_footer(Window *window)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_CONFIRM), "Raw",
        key_action_key_str(ACTION_SELECT), "Compare",
        key_action_key_str(ACTION_SHOW_PLAYER), "RTP Player",
        key_action_key_str(ACTION_SHOW_HELP), "Help",
        key_action_key_str(ACTION_SDP_INFO), "SDP",
        key_action_key_str(ACTION_TOGGLE_MEDIA), "RTP",
        key_action_key_str(ACTION_COMPRESS), "Compressed",
        key_action_key_str(ACTION_CYCLE_COLOR), "Colour by",
        key_action_key_str(ACTION_TOGGLE_RAW), "Toggle Raw",
        key_action_key_str(ACTION_AUTH_VALIDATE), "Auth Validate"
    };

    window_draw_bindings(window, keybindings, 20);
}

static void
call_flow_win_create_arrows(Window *window)
{
    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);

    // Copy displayed arrows
    self->darrows = self->arrows;

    // Create pending SIP arrows
    Message *msg = NULL;
    while ((msg = call_group_get_next_msg(self->group, msg))) {
        if (call_flow_arrow_find(window, msg) == NULL) {
            g_ptr_array_add(
                self->arrows,
                call_flow_arrow_create(window, msg, CF_ARROW_SIP)
            );
        }
    }

    // Create pending RTP arrows
    Stream *stream = NULL;
    while ((stream = call_group_get_next_stream(self->group, stream))) {
        if (!call_flow_arrow_find(window, stream)) {
            CallFlowArrow *arrow = call_flow_arrow_create(window, stream, CF_ARROW_RTP);
            g_ptr_array_add(self->arrows, arrow);
        }
    }

    // Sort arrows by time
    g_ptr_array_sort(self->arrows, (GCompareFunc) call_flow_arrow_time_sorter);

    // Set arrow columns after sorting arrows by time
    for (guint i = 0; i < g_ptr_array_len(self->arrows); i++) {
        CallFlowArrow *arrow = g_ptr_array_index(self->arrows, i);

        if (arrow->type != CF_ARROW_SIP)
            continue;

        Message *msg = call_flow_arrow_message(arrow);

        if (setting_disabled(SETTING_CF_SPLITCALLID)
            && msg_is_initial_transaction(msg)) {
            // Force Initial Transaction Arrows direction
            call_flow_arrow_set_columns(
                window, arrow,
                msg_is_request(msg) ? CF_ARROW_DIR_RIGHT : CF_ARROW_DIR_LEFT);
        } else {
            // Get origin and destination column
            call_flow_arrow_set_columns(
                window, arrow,
                CF_ARROW_DIR_ANY);
        }
    }
}

/**
 * @brief Draw the visible columns in panel window
 *
 * @param window UI structure pointer
 */
static void
call_flow_win_draw_columns(Window *window)
{
    Call *call = NULL;
    Stream *stream;
    char coltext[SETTING_MAX_LEN];

    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);

    WINDOW *win = window_get_ncurses_window(window);

    // Add RTP columns FIXME Really
    if (!setting_disabled(SETTING_CF_MEDIA)) {
        while ((call = call_group_get_next(self->group, call))) {
            for (guint i = 0; i < g_ptr_array_len(call->streams); i++) {
                stream = g_ptr_array_index(call->streams, i);
                if (stream->type == STREAM_RTP && stream_get_count(stream)) {
                    if (call_flow_column_get_first(window, address_strip_port(stream->src)) == NULL) {
                        call_flow_column_create(window, address_strip_port(stream->src));
                    }
                    if (call_flow_column_get_first(window, address_strip_port(stream->dst)) == NULL) {
                        call_flow_column_create(window, address_strip_port(stream->dst));
                    }
                }
            }
        }
    }

    // Draw columns
    for (GList *l = self->columns; l != NULL; l = l->next) {
        CallFlowColumn *column = l->data;

        // Draw horizontal line below column address
        mvwhline(self->columns_pad, 2, 10 + column->pos, ACS_HLINE, 21);
        mvwaddch(self->columns_pad, 2, 20 + column->pos, ACS_TTEE);

        if (column->twin && column->twin->pos < column->pos) {
            mvwaddch(self->columns_pad, 2, 20 + column->twin->pos, ACS_TTEE);
        }

        // Draw vertical line below column (in arrows pad)
        mvwvline(self->arrows_pad, 0, 20 + column->pos, ACS_VLINE, getmaxy(self->arrows_pad));

        // Set bold to this address if it's local
        if (setting_enabled(SETTING_CF_LOCALHIGHLIGHT)) {
            if (address_is_local(column->addr))
                wattron(self->columns_pad, A_BOLD);
        }

        if (setting_enabled(SETTING_CF_SPLITCALLID) || !address_get_port(column->addr)) {
            snprintf(coltext, SETTING_MAX_LEN, "%s", address_get_ip(column->addr));
        } else if (setting_enabled(SETTING_DISPLAY_ALIAS)) {
            if (strlen(address_get_ip(column->addr)) > 15) {
                snprintf(coltext, SETTING_MAX_LEN, "..%.*s:%hu",
                         SETTING_MAX_LEN - 7,
                         column->alias + strlen(column->alias) - 13,
                         address_get_port(column->addr)
                );
            } else {
                snprintf(coltext, SETTING_MAX_LEN, "%.*s:%hu",
                         SETTING_MAX_LEN - 7,
                         column->alias,
                         address_get_port(column->addr)
                );
            }
        } else {
            if (strlen(address_get_ip(column->addr)) > 15) {
                snprintf(coltext, SETTING_MAX_LEN, "..%.*s:%hu",
                         SETTING_MAX_LEN - 7,
                         address_get_ip(column->addr) + strlen(address_get_ip(column->addr)) - 13,
                         address_get_port(column->addr)
                );
            } else {
                snprintf(coltext, SETTING_MAX_LEN, "%.*s:%hu",
                         SETTING_MAX_LEN - 7,
                         address_get_ip(column->addr),
                         address_get_port(column->addr)
                );
            }
        }

        if (column->twin == NULL) {
            mvwprintw(self->columns_pad, 1, 10 + column->pos + (22 - strlen(coltext)) / 2, "%s", coltext);
        } else if (column->pos < column->twin->pos) {
            mvwprintw(self->columns_pad, 0, 5 + column->pos + (22 - strlen(coltext)) / 2, "%s", coltext);
        } else {
            mvwprintw(self->columns_pad, 1, 15 + column->pos + (22 - strlen(coltext)) / 2, "%s", coltext);
        }
        wattroff(self->columns_pad, A_BOLD);
    }

    // Copy columns to column subwindow
    copywin(
        self->columns_pad,
        self->colunms_win,
        0, self->hscroll.pos, 0, 0,
        getmaxy(self->colunms_win) - 1,
        getmaxx(self->colunms_win) - 1,
        FALSE
    );
    touchwin(win);
}

/**
 * @brief Draw the message arrow in the given line
 *
 * Draw the given message arrow in the given line.
 * This function will calculate origin and destination coordinates
 * base on message information. Each message use multiple lines
 * depending on the display mode of call flow
 *
 * @param window UI structure pointer
 * @param arrow Call flow arrow with message to be drawn
 * @param cline Window line to draw the message
 * @return the number of screen lines this arrow uses on screen
 */
static gint
call_flow_win_draw_message(Window *window, CallFlowArrow *arrow, gint cline)
{
    char msg_method[ATTR_MAXLEN];
    char msg_time[80];
    char method[ATTR_MAXLEN + 7];
    char delta[15] = { 0 };
    char mediastr[40];
    guint64 color = 0;
    int msglen;

    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, 0);

    // Store arrow start line
    arrow->line = cline + 1;

    // Calculate how many lines this message requires
    arrow->height = call_flow_arrow_height(window, arrow);

    Message *msg = arrow->item;
    g_return_val_if_fail(msg != NULL, 0);

    Call *call = msg_get_call(msg);
    g_return_val_if_fail(call != NULL, 0);

    // Packet SDP data
    PacketSdpData *sdp_data = packet_sdp_data(msg->packet);
    PacketSdpMedia *media = NULL;
    if (sdp_data != NULL) {
        media = g_list_nth_data(sdp_data->medias, 0);
    }
    // For extended, use xcallid instead
    date_time_time_to_str(msg_get_time(msg), msg_time);

    // Get Message method (include extra info)
    sprintf(msg_method, "%s", msg_get_method_str(msg));
    strcpy(method, msg_method);

    // If message has sdp information
    if (msg_has_sdp(msg) && setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_OFF) {
        // Show sdp tag in title
        sprintf(method, "%s (SDP)", msg_method);
    }

    // If message has sdp information
    if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_COMPRESSED) {
        // Show sdp tag in title
        if (msg_has_sdp(msg)) {
            sprintf(method, "%.*s (SDP)", 12, msg_method);
        } else {
            sprintf(method, "%.*s", 17, msg_method);
        }
    }

    if (media && setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_FIRST) {
        sprintf(method, "%.3s (%s:%u)",
                msg_method,
                (media->sconn != NULL) ? media->sconn->address : sdp_data->sconn->address,
                media->rtpport);
    }

    if (media && setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_FULL) {
        sprintf(method, "%.3s (%s)",
                msg_method,
                (media->sconn != NULL) ? media->sconn->address : sdp_data->sconn->address
        );
    }

    // Draw message type or status and line
    msglen = (strlen(method) > 24) ? 24 : strlen(method);

    // Determine start and end position of the arrow line
    int startpos, endpos;
    if (arrow->scolumn == arrow->dcolumn) {
        // Try to follow previous arrow direction
        CallFlowArrow *prev_arrow = call_flow_arrow_find_prev_callid(window, arrow);
        if (prev_arrow != NULL && prev_arrow->dir == CF_ARROW_DIR_LEFT) {
            arrow->dir = CF_ARROW_DIR_SPIRAL_LEFT;
            startpos = 21 + arrow->dcolumn->pos;
            endpos = 17 + arrow->scolumn->pos;
        } else {
            arrow->dir = CF_ARROW_DIR_SPIRAL_RIGHT;
            startpos = 19 + arrow->dcolumn->pos;
            endpos = 20 + arrow->scolumn->pos;
        }
    } else if (arrow->scolumn->pos < arrow->dcolumn->pos) {
        arrow->dir = CF_ARROW_DIR_RIGHT;
        startpos = 20 + arrow->scolumn->pos;
        endpos = 20 + arrow->dcolumn->pos;
    } else {
        arrow->dir = CF_ARROW_DIR_LEFT;
        startpos = 20 + arrow->dcolumn->pos;
        endpos = 20 + arrow->scolumn->pos;
    }
    int distance = abs(endpos - startpos) - 3;

    // Highlight current message
    if (arrow == g_ptr_array_index(self->darrows, self->cur_idx)) {
        switch (setting_get_enum(SETTING_CF_HIGHTLIGHT)) {
            case SETTING_ARROW_HIGHLIGH_BOLD:
                wattron(self->arrows_pad, A_BOLD);
                break;
            case SETTING_ARROW_HIGHLIGH_REVERSE:
                wattron(self->arrows_pad, A_REVERSE);
                break;
            case SETTING_ARROW_HIGHLIGH_REVERSEBOLD:
                wattron(self->arrows_pad, A_REVERSE);
                wattron(self->arrows_pad, A_BOLD);
                break;
        }
    }

    // Color the message {
    switch (setting_get_enum(SETTING_COLORMODE)) {
        case SETTING_COLORMODE_REQUEST:
            // Color by request / response
            color = (msg_is_request(msg)) ? CP_RED_ON_DEF : CP_GREEN_ON_DEF;
            break;
        case SETTING_COLORMODE_CALLID:
            // Color by call-id
            color = call_group_color(self->group, msg->call);
            break;
        case SETTING_COLORMODE_CSEQ:
            // Color by CSeq within the same call
            color = msg_get_cseq(msg) % 7 + 1;
            break;
    }

    // Print arrow in the same line than message
    if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_COMPRESSED) {
        arrow->line = cline;
    }

    // Turn on the message color
    wattron(self->arrows_pad, COLOR_PAIR(color));

    // Clear the line
    mvwprintw(self->arrows_pad, cline, startpos + 2, "%*s", distance, "");

    // Draw method
    if (arrow->dir == CF_ARROW_DIR_SPIRAL_RIGHT) {
        mvwprintw(self->arrows_pad, cline, startpos + 5, "%.26s", method);
    } else if (arrow->dir == CF_ARROW_DIR_SPIRAL_LEFT) {
        mvwprintw(self->arrows_pad, cline, startpos - msglen - 4, "%.26s", method);
    } else {
        mvwprintw(self->arrows_pad, cline, startpos + distance / 2 - msglen / 2 + 2, "%.26s", method);
    }

    // Draw media information
    if (msg_has_sdp(msg) && setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_FULL) {
        arrow->line += g_list_length(sdp_data->medias);
        for (GList *l = sdp_data->medias; l != NULL; l = l->next) {
            cline++;
            PacketSdpMedia *media = l->data;
            sprintf(mediastr, "%s %d (%s)",
                    packet_sdp_media_type_str(media->type),
                    media->rtpport,
                    msg_get_preferred_codec_alias(msg));
            mvwprintw(self->arrows_pad, cline, startpos + distance / 2 - strlen(mediastr) / 2 + 2, mediastr);
        }
    }

    if (arrow->dir != CF_ARROW_DIR_SPIRAL_RIGHT && arrow->dir != CF_ARROW_DIR_SPIRAL_LEFT) {
        if (arrow == call_flow_arrow_selected(window)) {
            mvwhline(self->arrows_pad, arrow->line, startpos + 2, '=', distance);
        } else {
            mvwhline(self->arrows_pad, arrow->line, startpos + 2, ACS_HLINE, distance);
        }
    }

    // Write the arrow at the end of the message (two arrows if this is a retrans)
    if (arrow->dir == CF_ARROW_DIR_SPIRAL_RIGHT) {
        mvwaddwstr(self->arrows_pad, arrow->line, startpos + 2, tui_acs_utf8('<'));
        if (msg_is_retransmission(msg)) {
            mvwaddwstr(self->arrows_pad, arrow->line, startpos + 3, tui_acs_utf8('<'));
            mvwaddwstr(self->arrows_pad, arrow->line, startpos + 4, tui_acs_utf8('<'));
        }
        // If multiple lines are available, print a spiral icon
        if (arrow->line != cline) {
            mvwaddch(self->arrows_pad, arrow->line, startpos + 3, ACS_LRCORNER);
            mvwaddch(self->arrows_pad, arrow->line - 1, startpos + 3, ACS_URCORNER);
            mvwaddch(self->arrows_pad, arrow->line - 1, startpos + 2, ACS_HLINE);
        }
    } else if (arrow->dir == CF_ARROW_DIR_SPIRAL_LEFT) {
        mvwaddwstr(self->arrows_pad, arrow->line, startpos - 2, tui_acs_utf8('>'));
        if (msg_is_retransmission(msg)) {
            mvwaddwstr(self->arrows_pad, arrow->line, startpos - 3, tui_acs_utf8('>'));
            mvwaddwstr(self->arrows_pad, arrow->line, startpos - 4, tui_acs_utf8('>'));
        }
        // If multiple lines are available, print a spiral icon
        if (arrow->line != cline) {
            mvwaddch(self->arrows_pad, arrow->line, startpos - 3, ACS_LLCORNER);
            mvwaddch(self->arrows_pad, arrow->line - 1, startpos - 3, ACS_ULCORNER);
            mvwaddch(self->arrows_pad, arrow->line - 1, startpos - 2, ACS_HLINE);
        }
    } else if (arrow->dir == CF_ARROW_DIR_RIGHT) {
        mvwaddwstr(self->arrows_pad, arrow->line, endpos - 2, tui_acs_utf8('>'));
        if (msg_is_retransmission(msg)) {
            mvwaddwstr(self->arrows_pad, arrow->line, endpos - 3, tui_acs_utf8('>'));
            mvwaddwstr(self->arrows_pad, arrow->line, endpos - 4, tui_acs_utf8('>'));
        }
    } else {
        mvwaddwstr(self->arrows_pad, arrow->line, startpos + 2, tui_acs_utf8('<'));
        if (msg_is_retransmission(msg)) {
            mvwaddwstr(self->arrows_pad, arrow->line, startpos + 3, tui_acs_utf8('<'));
            mvwaddwstr(self->arrows_pad, arrow->line, startpos + 4, tui_acs_utf8('<'));
        }
    }

    if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_COMPRESSED)
        mvwprintw(self->arrows_pad, cline, startpos + distance / 2 - msglen / 2 + 2, " %.26s ", method);

    // Turn off colors
    wattroff(self->arrows_pad, COLOR_PAIR(CP_RED_ON_DEF));
    wattroff(self->arrows_pad, COLOR_PAIR(CP_GREEN_ON_DEF));
    wattroff(self->arrows_pad, COLOR_PAIR(CP_CYAN_ON_DEF));
    wattroff(self->arrows_pad, COLOR_PAIR(CP_YELLOW_ON_DEF));
    wattroff(self->arrows_pad, A_BOLD | A_REVERSE);

    // Print timestamp
    if (self->arrowtime) {
        if (arrow == call_flow_arrow_selected(window))
            wattron(self->arrows_pad, COLOR_PAIR(CP_CYAN_ON_DEF));

        if (arrow == g_ptr_array_index(self->darrows, self->cur_idx)) {
            wattron(self->arrows_pad, A_BOLD);
            mvwprintw(self->arrows_pad, cline, 2, "%s", msg_time);
            wattroff(self->arrows_pad, A_BOLD);
        } else {
            mvwprintw(self->arrows_pad, cline, 2, "%s", msg_time);
        }

        // Print delta from selected message
        if (setting_get_enum(SETTING_CF_SDP_INFO) != SETTING_SDP_COMPRESSED) {
            if (self->selected == -1) {
                if (setting_enabled(SETTING_CF_DELTA)) {
                    date_time_to_delta(
                        msg_get_time(msg),
                        msg_get_time(call_group_get_next_msg(self->group, msg)),
                        delta
                    );
                }
            } else if (arrow == g_ptr_array_index(self->darrows, self->cur_idx)) {
                date_time_to_delta(
                    msg_get_time(call_flow_arrow_message(call_flow_arrow_selected(window))),
                    msg_get_time(msg),
                    delta
                );
            }

            if (strlen(delta)) {
                wattron(self->arrows_pad, COLOR_PAIR(CP_CYAN_ON_DEF));
                mvwprintw(self->arrows_pad, cline + 1, 2, "%15s", delta);
            }
            wattroff(self->arrows_pad, COLOR_PAIR(CP_CYAN_ON_DEF));
        }
    }
    wattroff(self->arrows_pad, COLOR_PAIR(CP_CYAN_ON_DEF));

    return arrow->height;
}

/**
 * @brief Draw the stream data in the given line
 *
 * Draw the given arrow of type stream in the given line.
 *
 * @param window UI structure pointer
 * @param arrow Call flow arrow of stream to be drawn
 * @param cline Window line to draw the message
 * @return the number of screen lines this arrow uses on screen
 */
static int
call_flow_win_draw_rtp_stream(Window *window, CallFlowArrow *arrow, int cline)
{
    char text[50], time[20];
    int height;
    Stream *stream = arrow->item;
    Message *msg;
    CallFlowArrow *msgarrow;

    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, 0);

    // Get the messages window
    height = getmaxy(self->arrows_win);

    // Store arrow start line
    arrow->line = cline + 1;

    // Calculate how many lines this message requires
    arrow->height = call_flow_arrow_height(window, arrow);

    // Get arrow text
    g_autofree const gchar *stream_format = stream_get_format(stream);
    guint stream_count = stream_get_count(stream);
    sprintf(text, "RTP (%s) %d", stream_format, stream_count);

    // Message with Stream destination configured in SDP content
    msg = stream->msg;

    // Reuse the msg arrow columns as destination column
    if ((msgarrow = call_flow_arrow_find(window, msg))) {
        if (address_equals(msgarrow->scolumn->addr, stream->src))
            arrow->scolumn = msgarrow->scolumn;
        if (address_equals(msgarrow->scolumn->addr, stream->dst))
            arrow->dcolumn = msgarrow->scolumn;
        if (address_equals(msgarrow->dcolumn->addr, stream->src))
            arrow->scolumn = msgarrow->dcolumn;
        if (address_equals(msgarrow->dcolumn->addr, stream->dst))
            arrow->dcolumn = msgarrow->dcolumn;
    }

    // fallback: Just use any column that have the destination IP printed
    if (arrow->dcolumn == NULL) {
        arrow->dcolumn =
            call_flow_column_get_first(window, address_strip_port(stream->dst));
    }

    if (arrow->scolumn == NULL) {
        arrow->scolumn =
            call_flow_column_get_first(window, address_strip_port(stream->src));
    }

    // Determine start and end position of the arrow line
    int startpos, endpos;
    if (arrow->scolumn->pos < arrow->dcolumn->pos) {
        arrow->dir = CF_ARROW_DIR_RIGHT;
        startpos = 20 + arrow->scolumn->pos;
        endpos = 20 + arrow->dcolumn->pos;
    } else {
        arrow->dir = CF_ARROW_DIR_LEFT;
        startpos = 20 + arrow->dcolumn->pos;
        endpos = 20 + arrow->scolumn->pos;
    }
    int distance = 0;

    if (startpos != endpos) {
        // In compressed mode, we display the src and dst port inside the arrow
        // so fixup the stard and end position
        if (setting_get_enum(SETTING_CF_SDP_INFO) != SETTING_SDP_COMPRESSED) {
            startpos += 5;
            endpos -= 5;
        }
        distance = abs(endpos - startpos) - 4 + 1;
    } else {
        // Fix port positions
        startpos -= 2;
        endpos += 2;
        distance = 1;

        // Fix arrow direction based on ports
        if (address_get_port(stream->src) < address_get_port(stream->dst)) {
            arrow->dir = CF_ARROW_DIR_RIGHT;
        } else {
            arrow->dir = CF_ARROW_DIR_LEFT;
        }
    }

    // Highlight current message
    if (arrow == g_ptr_array_index(self->darrows, self->cur_idx)) {
        switch (setting_get_enum(SETTING_CF_HIGHTLIGHT)) {
            case SETTING_ARROW_HIGHLIGH_BOLD:
                wattron(self->arrows_pad, A_BOLD);
                break;
            case SETTING_ARROW_HIGHLIGH_REVERSE:
                wattron(self->arrows_pad, A_REVERSE);
                break;
            case SETTING_ARROW_HIGHLIGH_REVERSEBOLD:
                wattron(self->arrows_pad, A_REVERSE);
                wattron(self->arrows_pad, A_BOLD);
                break;
        }
    }

    // Check if displayed stream is active
    gboolean active = stream_is_active(stream);

    // Clear the line
    mvwprintw(self->arrows_pad, cline, startpos + 2, "%*s", distance, "");
    // Draw RTP arrow text
    mvwprintw(self->arrows_pad, cline, startpos + (distance) / 2 - strlen(text) / 2 + 2, "%s", text);

    if (setting_get_enum(SETTING_CF_SDP_INFO) != SETTING_SDP_COMPRESSED)
        cline++;

    // Draw line between columns
    if (active)
        mvwhline(self->arrows_pad, cline, startpos + 2, '-', distance);
    else
        mvwhline(self->arrows_pad, cline, startpos + 2, ACS_HLINE, distance);

    // Write the arrow at the end of the message (two arrows if this is a retrans)
    if (arrow->dir == CF_ARROW_DIR_RIGHT) {
        if (setting_get_enum(SETTING_CF_SDP_INFO) != SETTING_SDP_COMPRESSED) {
            mvwprintw(self->arrows_pad, cline, startpos - 4, "%d", address_get_port(stream->src));
            mvwprintw(self->arrows_pad, cline, endpos, "%d", address_get_port(stream->dst));
        }
        mvwaddwstr(self->arrows_pad, cline, endpos - 2, tui_acs_utf8('>'));
        if (active) {
            arrow->rtp_count = stream_get_count(stream);
            arrow->rtp_ind_pos = (arrow->rtp_ind_pos + 1) % distance;
            mvwaddwstr(self->arrows_pad, cline, startpos + arrow->rtp_ind_pos + 2, tui_acs_utf8('>'));
        }
    } else {
        if (setting_get_enum(SETTING_CF_SDP_INFO) != SETTING_SDP_COMPRESSED) {
            mvwprintw(self->arrows_pad, cline, endpos, "%d", address_get_port(stream->src));
            mvwprintw(self->arrows_pad, cline, startpos - 4, "%d", address_get_port(stream->dst));
        }
        mvwaddwstr(self->arrows_pad, cline, startpos + 2, tui_acs_utf8('<'));
        if (active) {
            arrow->rtp_count = stream_get_count(stream);
            arrow->rtp_ind_pos = (arrow->rtp_ind_pos + 1) % distance;
            mvwaddwstr(self->arrows_pad, cline, endpos - arrow->rtp_ind_pos - 2, tui_acs_utf8('<'));
        }
    }

    if (setting_get_enum(SETTING_CF_SDP_INFO) == SETTING_SDP_COMPRESSED)
        mvwprintw(self->arrows_pad, cline, startpos + (distance) / 2 - strlen(text) / 2 + 2, " %s ", text);

    wattroff(self->arrows_pad, A_BOLD | A_REVERSE);

    // Print timestamp
    if (self->arrowtime) {
        date_time_time_to_str(stream_time(stream), time);
        if (arrow == g_ptr_array_index(self->darrows, self->cur_idx)) {
            wattron(self->arrows_pad, A_BOLD);
            mvwprintw(self->arrows_pad, cline - 1, 2, "%s", time);
            wattroff(self->arrows_pad, A_BOLD);
        } else {
            mvwprintw(self->arrows_pad, cline - 1, 2, "%s", time);
        }

    }

    return arrow->height;
}

/**
 * @brief Draw a single arrow in arrow flow
 *
 * This function draws an arrow of any type in the given line of the flow.
 *
 * @param window UI Structure pointer
 * @param arrow Arrow structure pointer of any type
 * @param line Line of the flow window to draw this arrow
 * @return the number of screen lines this arrow uses on screen
 */
static gint
call_flow_win_draw_arrow(Window *window, CallFlowArrow *arrow, gint line)
{
    g_return_val_if_fail(arrow != NULL, 0);

    if (arrow->type == CF_ARROW_SIP) {
        return call_flow_win_draw_message(window, arrow, line);
    } else {
        return call_flow_win_draw_rtp_stream(window, arrow, line);
    }
}

/**
 * @brief Draw arrows in the visible part of the panel
 *
 * @param window UI structure pointer
 */
static void
call_flow_win_draw_arrows(Window *window)
{
    int cline = 0;

    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);

    // Draw arrows
    for (guint i = 0; i < g_ptr_array_len(self->darrows); i++) {
        CallFlowArrow *arrow = g_ptr_array_index(self->darrows, i);

        if (!call_flow_arrow_filter(arrow))
            continue;

        // Draw arrow
        cline += call_flow_win_draw_arrow(window, arrow, cline);
    }

    // Copy Arrows to arrow subwindow
    copywin(
        self->arrows_pad,
        self->arrows_win,
        self->vscroll.pos, self->hscroll.pos,
        0, 0,
        getmaxy(self->arrows_win) - 1,
        getmaxx(self->arrows_win) - 1,
        FALSE
    );

    // Copy Arrow Times to arrow subwindow
    if (self->arrowtime) {
        copywin(
            self->arrows_pad,
            self->arrows_win,
            self->vscroll.pos, 0,
            0, 0,
            getmaxy(self->arrows_win) - 1,
            19,
            FALSE
        );
    }
}

/**
 * @brief Draw raw panel with message payload
 *
 * Draw the given message payload into the raw window.
 *
 * @param window UI structure pointer
 * @param msg Message data to draw
 */
static void
call_flow_win_draw_raw(Window *window, Message *msg)
{
    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);
    WINDOW *win = window_get_ncurses_window(window);

    // Print msg payload
    draw_message(self->raw_win, msg);
    touchwin(win);
}

/**
 * @brief Draw raw panel with RTCP data
 *
 * Draw the given stream data into the raw window.
 *
 * @param window UI structure pointer
 * @param rtcp stream containing the RTCP conection data
 * @return 0 in all cases
 */
static int
call_flow_win_draw_raw_rtcp(Window *window, G_GNUC_UNUSED Stream *stream)
{
    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);

    guint row = 1;
    mvwprintw(self->raw_win, row++, 1, "RTP Stream Analysis");
    mvwhline(self->raw_win, row++, 1, ACS_HLINE, getmaxx(self->raw_win) - 1);
    mvwprintw(self->raw_win, row++, 1, "Source: %s:%hu",
              address_get_ip(stream->src), address_get_port(stream->src));
    mvwprintw(self->raw_win, row++, 1, "Destination: %s:%hu",
              address_get_ip(stream->dst), address_get_port(stream->dst));
    mvwprintw(self->raw_win, row++, 1, "SSRC: 0x%X", stream->ssrc);
    mvwprintw(self->raw_win, row++, 1, "Packets: %d / %d", stream->packet_count, stream->stats.expected);
    mvwprintw(self->raw_win, row++, 1, "Lost: %d (%.1f%%)", stream->stats.lost,
              (gdouble) stream->stats.lost / stream->stats.expected * 100);
    mvwprintw(self->raw_win, row++, 1, "Out of sequence: %d (%.1f%%)", stream->stats.oos,
              (gdouble) stream->stats.oos / stream->packet_count * 100);
    mvwprintw(self->raw_win, row++, 1, "Max Delta: %.2f ms", stream->stats.max_delta);
    mvwprintw(self->raw_win, row++, 1, "Max Jitter: %.2f ms", stream->stats.max_jitter);
    mvwprintw(self->raw_win, row++, 1, "Mean Jitter: %.2f ms", stream->stats.mean_jitter);
    mvwprintw(self->raw_win, row++, 1, "Problems: %s", (stream->stats.lost) ? "Yes" : "No");
    row++;

    mvwprintw(self->raw_win, row++, 1, "RTCP VoIP Metrics Report");
    mvwhline(self->raw_win, row++, 1, ACS_HLINE, getmaxx(self->raw_win));

    return 0;
}

/**
 * @brief Draw panel preview of current arrow
 *
 * If user request to not draw preview panel, this function does nothing.
 *
 * @param window UI structure pointer
 */
static void
call_flow_win_draw_preview(Window *window)
{
    CallFlowArrow *arrow = NULL;

    // Check if not displaying raw has been requested
    if (setting_disabled(SETTING_CF_FORCERAW))
        return;

    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);

    // Draw current arrow preview
    if ((arrow = g_ptr_array_index(self->darrows, self->cur_idx))) {
        if (arrow->type == CF_ARROW_SIP) {
            call_flow_win_draw_raw(window, arrow->item);
        } else {
            call_flow_win_draw_raw_rtcp(window, arrow->item);
        }
    }
}

/**
 * @brief Move selection cursor up N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move up
 */
static void
call_flow_win_move_vertical(Window *window, gint times)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);

    // Set the new current selected index
    self->cur_idx = CLAMP(
        (gint) self->cur_idx + times,
        0,
        (gint) g_ptr_array_len(self->darrows) - 1
    );

    CallFlowArrow *cur_arrow = g_ptr_array_index(self->darrows, self->cur_idx);
    g_return_if_fail(cur_arrow != NULL);

    // Move the first index if required (moving up)
    self->vscroll.pos = MIN(self->vscroll.pos, cur_arrow->line - cur_arrow->height + 1);

    gint arrow_win_height = getmaxy(self->arrows_win);
    if (scrollbar_visible(self->hscroll)) {
        arrow_win_height -= 1;
    }

    // Move the first index if required (moving down)
    self->vscroll.pos = MAX(self->vscroll.pos, cur_arrow->line - arrow_win_height);
}


/**
 * @brief Move selection cursor up N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move up
 */
static void
call_flow_win_move_horizontal(Window *window, gint times)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);

    // Move the first index if required (moving left)
    self->hscroll.pos = CLAMP(self->hscroll.pos + times,
                              0,
                              getmaxx(self->columns_pad) - getmaxx(self->colunms_win)
    );
}

void
call_flow_win_set_group(Window *window, CallGroup *group)
{
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_if_fail(self != NULL);

    g_list_free_full(self->columns, (GDestroyNotify) call_flow_column_free);
    self->columns = NULL;
    g_ptr_array_remove_all(self->arrows);

    self->group = group;
    self->cur_idx = 0;
    self->selected = -1;
}

/**
 * @brief Handle Call flow extended key strokes
 *
 * This function will manage the custom keybindings of the panel.
 * This function return one of the values defined in @key_handler_ret
 *
 * @param window UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
static gint
call_flow_win_handle_key(Window *window, gint key)
{
    int raw_width;
    Window *next_window;
    Call *call = NULL;
    CallFlowArrow *cur_arrow = NULL;
    guint rnpag_steps = (guint) setting_get_intvalue(SETTING_CF_SCROLLSTEP);

    // Sanity check, this should not happen
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, KEY_NOT_HANDLED);
    WINDOW *win = window_get_ncurses_window(window);

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                call_flow_win_move_vertical(window, 1);
                break;
            case ACTION_UP:
                call_flow_win_move_vertical(window, -1);
                break;
            case ACTION_RIGHT:
                call_flow_win_move_horizontal(window, 15);
                break;
            case ACTION_LEFT:
                call_flow_win_move_horizontal(window, -15);
                break;
            case ACTION_HNPAGE:
                call_flow_win_move_vertical(window, rnpag_steps / 2);;
                break;
            case ACTION_NPAGE:
                call_flow_win_move_vertical(window, rnpag_steps);
                break;
            case ACTION_HPPAGE:
                call_flow_win_move_vertical(window, -1 * rnpag_steps / 2);;
                break;
            case ACTION_PPAGE:
                call_flow_win_move_vertical(window, -1 * rnpag_steps);
                break;
            case ACTION_BEGIN:
                call_flow_win_move_vertical(window, -1 * g_ptr_array_len(self->darrows));
                break;
            case ACTION_END:
                call_flow_win_move_vertical(window, g_ptr_array_len(self->darrows));
                break;
            case ACTION_SHOW_FLOW_EX:
                werase(win);
                if (call_group_count(self->group) == 1) {
                    call = call_group_get_next(self->group, NULL);
                    call_group_add_calls(self->group, call->xcalls);
                    self->group->callid = call->callid;
                } else {
                    call = call_group_get_next(self->group, NULL);
                    call_group_remove_all(self->group);
                    call_group_add(self->group, call);
                    self->group->callid = 0;
                }
                call_flow_win_set_group(window, self->group);
                break;
            case ACTION_SHOW_RAW:
                // KEY_R, display current call in raw mode
                next_window = tui_create_window(WINDOW_CALL_RAW);
                call_raw_win_set_group(next_window, self->group);
                break;
            case ACTION_DECREASE_RAW:
                raw_width = getmaxx(self->raw_win);
                if (raw_width - 2 > 1) {
                    setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, raw_width - 2);
                }
                break;
            case ACTION_INCREASE_RAW:
                raw_width = MIN(getmaxx(self->raw_win) + 2, window_get_width(window) - 1);
                setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, raw_width);
                break;
            case ACTION_RESET_RAW:
                setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, -1);
                break;
            case ACTION_ONLY_SDP:
                // Toggle SDP mode
                self->group->sdp_only = !(self->group->sdp_only);
                // Disable sdp_only if there are not messages with sdp
                if (call_group_msg_count(self->group) == 0)
                    self->group->sdp_only = 0;
                // Reset screen
                call_flow_win_set_group(window, self->group);
                break;
            case ACTION_SDP_INFO:
                setting_toggle(SETTING_CF_SDP_INFO);
                break;
            case ACTION_HIDE_DUPLICATE:
                setting_toggle(SETTING_CF_HIDEDUPLICATE);
                call_flow_win_set_group(window, self->group);
                break;
            case ACTION_ONLY_MEDIA:
                setting_toggle(SETTING_CF_ONLYMEDIA);
                call_flow_win_set_group(window, self->group);
                break;
            case ACTION_TOGGLE_MEDIA:
                setting_toggle(SETTING_CF_MEDIA);
                // Force reload arrows
                call_flow_win_set_group(window, self->group);
                break;
            case ACTION_TOGGLE_RAW:
                setting_toggle(SETTING_CF_FORCERAW);
                break;
            case ACTION_COMPRESS:
                setting_toggle(SETTING_CF_SPLITCALLID);
                // Force columns reload
                call_flow_win_set_group(window, self->group);
                break;
            case ACTION_SAVE:
                cur_arrow = g_ptr_array_index(self->darrows, self->cur_idx);
                if (cur_arrow->type == CF_ARROW_SIP) {
                    next_window = tui_create_window(WINDOW_SAVE);
                    save_set_group(next_window, self->group);
                    save_set_msg(next_window, call_flow_arrow_message(cur_arrow));
                }
#ifdef WITH_SND
                if (cur_arrow->type == CF_ARROW_RTP) {
                    StorageCaptureOpts storageCaptureOpts = storage_capture_options();
                    if (!storageCaptureOpts.rtp) {
                        dialog_run("RTP packets are not being stored, run with --rtp flag.");
                        break;
                    }
                    next_window = tui_create_window(WINDOW_SAVE);
                    save_set_stream(next_window, cur_arrow->item);
                }
#endif
                break;
#ifdef WITH_PULSE
            case ACTION_SHOW_PLAYER:
                cur_arrow = g_ptr_array_index(self->darrows, self->cur_idx);
                if (cur_arrow->type == CF_ARROW_RTP) {
                    StorageCaptureOpts storageCaptureOpts = storage_capture_options();
                    if (!storageCaptureOpts.rtp) {
                        dialog_run("RTP packets are not being stored, run with --rtp flag.");
                        break;
                    }

                    next_window = tui_create_window(WINDOW_RTP_PLAYER);
                    if (next_window != NULL) {
                        rtp_player_win_set_stream(next_window, cur_arrow->item);
                    }
                }
                break;
#endif
            case ACTION_AUTH_VALIDATE:
                next_window = tui_create_window(WINDOW_AUTH_VALIDATE);
                auth_validate_win_set_group(next_window, self->group);
                break;
            case ACTION_TOGGLE_TIME:
                self->arrowtime = (self->arrowtime) ? FALSE : TRUE;
                break;
            case ACTION_SELECT:
                if (self->selected == -1) {
                    self->selected = self->cur_idx;
                } else {
                    if (self->selected == (gint) self->cur_idx) {
                        self->selected = -1;
                    } else {
                        // Show diff panel
                        next_window = tui_create_window(WINDOW_MSG_DIFF);
                        msg_diff_win_set_msgs(next_window,
                                              call_flow_arrow_message(g_ptr_array_index(self->darrows, self->selected)),
                                              call_flow_arrow_message(g_ptr_array_index(self->darrows, self->cur_idx)));
                    }
                }
                break;
            case ACTION_CLEAR:
                self->selected = -1;
                break;
            case ACTION_CONFIRM:
                // KEY_ENTER, display current message in raw mode
                next_window = tui_create_window(WINDOW_CALL_RAW);
                call_raw_win_set_group(next_window, self->group);
                call_raw_win_set_msg(next_window,
                                     call_flow_arrow_message(g_ptr_array_index(self->darrows, self->cur_idx)));
                break;
            case ACTION_CLEAR_CALLS:
            case ACTION_CLEAR_CALLS_SOFT:
                // Propagate the key to the previous panel
                return KEY_PROPAGATED;

            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

/**
 * @brief Request the panel to show its help
 *
 * This function will request to panel to show its help (if any) by
 * invoking its help function.
 *
 * @param window UI structure pointer
 * @return 0 if the screen has help, -1 otherwise
 */
static gint
call_flow_win_help(G_GNUC_UNUSED Window *window)
{
    WINDOW *help_win;
    int height, width;

    // Create a new panel and show centered
    height = 28;
    width = 65;
    help_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Set the window title
    mvwprintw(help_win, 1, 18, "Call Flow Help");

    // Write border and boxes around the window
    wattron(help_win, COLOR_PAIR(CP_BLUE_ON_DEF));
    box(help_win, 0, 0);
    mvwhline(help_win, 2, 1, ACS_HLINE, 63);
    mvwhline(help_win, 7, 1, ACS_HLINE, 63);
    mvwhline(help_win, height - 3, 1, ACS_HLINE, 63);
    mvwaddch(help_win, 2, 0, ACS_LTEE);
    mvwaddch(help_win, 7, 0, ACS_LTEE);
    mvwaddch(help_win, height - 3, 0, ACS_LTEE);
    mvwaddch(help_win, 2, 64, ACS_RTEE);
    mvwaddch(help_win, 7, 64, ACS_RTEE);
    mvwaddch(help_win, height - 3, 64, ACS_RTEE);

    // Set the window footer (nice blue?)
    mvwprintw(help_win, height - 2, 20, "Press any key to continue");

    // Some brief explanation abotu what window shows
    wattron(help_win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(help_win, 3, 2, "This window shows the messages from a call and its relative");
    mvwprintw(help_win, 4, 2, "ordered by sent or received time.");
    mvwprintw(help_win, 5, 2, "This panel is mosly used when capturing at proxy systems that");
    mvwprintw(help_win, 6, 2, "manages incoming and outgoing request between calls.");
    wattroff(help_win, COLOR_PAIR(CP_CYAN_ON_DEF));

    // A list of available keys in this window
    mvwprintw(help_win, 8, 2, "Available keys:");
    mvwprintw(help_win, 9, 2, "Esc/Q       Go back to Call list window");
    mvwprintw(help_win, 10, 2, "F5/Ctrl-L   Leave screen and clear call list");
    mvwprintw(help_win, 11, 2, "Enter       Show current message Raw");
    mvwprintw(help_win, 12, 2, "F1/h        Show this screen");
    mvwprintw(help_win, 13, 2, "F2/d        Toggle SDP Address:Port info");
    mvwprintw(help_win, 14, 2, "F3/m        Toggle RTP arrows display");
    mvwprintw(help_win, 15, 2, "F4/X        Show call-flow with X-CID/X-Call-ID dialog");
    mvwprintw(help_win, 16, 2, "F5/s        Toggle compressed view (One address <=> one column");
    mvwprintw(help_win, 17, 2, "F6/R        Show original call messages in raw mode");
    mvwprintw(help_win, 18, 2, "F7/c        Cycle between available color modes");
    mvwprintw(help_win, 19, 2, "F8/C        Turn on/off message syntax highlighting");
    mvwprintw(help_win, 20, 2, "F9/l        Turn on/off resolved addresses");
    mvwprintw(help_win, 21, 2, "9/0         Increase/Decrease raw preview size");
    mvwprintw(help_win, 22, 2, "t           Toggle raw preview display");
    mvwprintw(help_win, 23, 2, "T           Restore raw preview size");
    mvwprintw(help_win, 24, 2, "D           Only show SDP messages");

    // Press any key to close
    wgetch(help_win);

    return 0;
}

static void
call_flow_win_create_subwindows(Window *window)
{
    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    WINDOW *win = window_get_ncurses_window(window);
    gint win_height = window_get_height(window);
    gint win_width = window_get_width(window);

    // Create pad to store all arrows
    if (self->arrows_pad) delwin(self->arrows_pad);
    gint pad_height = MAX(call_flow_win_arrows_height(window) + 1, win_height - 4);
    gint pad_width = MAX(call_flow_win_columns_width(window), win_width);
    self->arrows_pad = newpad(pad_height, pad_width);

    // Create pad to store all columns
    if (self->columns_pad) delwin(self->columns_pad);
    self->columns_pad = newpad(3 /* column height */, pad_width);

    gint raw_width = 0;
    // Check if not displaying raw has been requested
    if (setting_enabled(SETTING_CF_FORCERAW)) {
        // Get min raw width
        gint min_raw_width = setting_get_intvalue(SETTING_CF_RAWMINWIDTH);
        gint fixed_raw_width = setting_get_intvalue(SETTING_CF_RAWFIXEDWIDTH);

        // We can configure an exact raw size
        if (fixed_raw_width > 0) {
            raw_width = fixed_raw_width;
        } else {
            raw_width = MAX(win_width - call_flow_win_columns_width(window), min_raw_width);
        }

        // Height of raw window is always available size minus 6 lines for header/footer
        gint raw_height = win_height - 2;

        // If we already have a raw window
        if (self->raw_win) delwin(self->raw_win);
        self->raw_win = subwin(win, raw_height, raw_width, 1, win_width - raw_width - 1);

        // Draw raw Separator line
        wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
        mvwvline(win, 1, win_width - raw_width - 2, ACS_VLINE, win_height - 2);
        wattroff(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    }

    // Call Flow arrows subwindow
    if (self->arrows_win) delwin(self->arrows_win);
    gint flow_height = win_height - 5;
    gint flow_width = (raw_width) ? win_width - raw_width - 2 : win_width;
    self->arrows_win = subwin(win, flow_height, flow_width, 4, 0);
    self->colunms_win = subwin(win, 3, flow_width, 1, 0);

    // Configure vertical scrollbar
    self->vscroll.max = getmaxy(self->arrows_pad) - 1;
    self->vscroll.win = self->arrows_win;

    // Configure horizontal scrollbar
    self->hscroll.max = getmaxx(self->arrows_pad) - 1;
    self->hscroll.win = self->arrows_win;
}

/**
 * @brief Draw the Call flow extended panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param window UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
static gint
call_flow_win_draw(Window *window)
{
    char title[256];

    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    g_return_val_if_fail(self != NULL, -1);

    // Get window of main panel
    WINDOW *win = window_get_ncurses_window(window);
    werase(win);

    // Set title
    if (self->group->callid) {
        sprintf(title, "Extended Call flow for %s", self->group->callid);
    } else if (call_group_count(self->group) == 1) {
        Call *call = call_group_get_next(self->group, NULL);
        sprintf(title, "Call flow for %s", call->callid);
    } else {
        sprintf(title, "Call flow for %d dialogs", call_group_count(self->group));
    }

    // Print color mode in title
    switch (setting_get_enum(SETTING_COLORMODE)) {
        case SETTING_COLORMODE_REQUEST:
            strcat(title, " (Color by Request/Response)");
            break;
        case SETTING_COLORMODE_CALLID:
            strcat(title, " (Color by Call-Id)");
            break;
        case SETTING_COLORMODE_CSEQ:
            strcat(title, " (Color by CSeq)");
            break;
    }

    // Draw panel title
    window_set_title(window, title);

    // Show some keybinding
    call_flow_win_draw_footer(window);

    // Create pending arrows for SIP and RTP
    call_flow_win_create_arrows(window);

    // Create subwindows for all components
    call_flow_win_create_subwindows(window);

    // Redraw columns
    call_flow_win_draw_columns(window);

    // Redraw arrows
    call_flow_win_draw_arrows(window);

    // Redraw preview
    call_flow_win_draw_preview(window);

    // Draw scrollbars
    scrollbar_draw(self->vscroll);
    scrollbar_draw(self->hscroll);

    return 0;
}

/**
 * @brief Determine if the screen requires redrawn
 *
 * This will query the interface if it requires to be redraw again.
 *
 * @param window UI structure pointer
 * @return true if the panel requires redraw, false otherwise
 */
static gboolean
call_flow_win_redraw(Window *window)
{
    // Get panel information
    CallFlowWindow *self = TUI_CALL_FLOW(window);
    WINDOW *win = window_get_ncurses_window(window);

    // Get current screen dimensions
    gint maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);

    // Change the main window size
    wresize(win, maxy, maxx);

    // Store new size
    window_set_width(window, maxx);
    window_set_height(window, maxy);

    // Calculate available printable area
    wresize(self->arrows_win, maxy - 6, maxx);

    // Check if any of the group has changed
    return call_group_changed(self->group);
}

void
call_flow_win_free(Window *window)
{
    g_object_unref(window);
}

Window *
call_flow_win_new()
{
    return g_object_new(
        WINDOW_TYPE_CALL_FLOW,
        NULL
    );
}

static void
call_flow_win_finalized(GObject *object)
{
    CallFlowWindow *self = TUI_CALL_FLOW(object);
    g_return_if_fail(self != NULL);

    // Free the panel information
    g_list_free(self->columns);
    g_ptr_array_free(self->arrows, TRUE);

    // Delete panel windows
    delwin(self->arrows_pad);
    delwin(self->colunms_win);
    delwin(self->arrows_win);
    delwin(self->raw_win);

    // Delete displayed call group
    call_group_free(self->group);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(call_flow_win_parent_class)->finalize(object);
}

static void
call_flow_win_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(call_flow_win_parent_class)->constructed(object);

    // Get parent window information
    CallFlowWindow *self = TUI_CALL_FLOW(object);

    // Calculate available printable area for messages
    self->vscroll = window_set_scrollbar(self->arrows_win, SB_VERTICAL, SB_LEFT);
    self->hscroll = window_set_scrollbar(self->arrows_win, SB_HORIZONTAL, SB_BOTTOM);
}

static void
call_flow_win_class_init(CallFlowWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = call_flow_win_constructed;
    object_class->finalize = call_flow_win_finalized;

    WindowClass *window_class = TUI_WINDOW_CLASS(klass);
    window_class->redraw = call_flow_win_redraw;
    window_class->draw = call_flow_win_draw;
    window_class->handle_key = call_flow_win_handle_key;
    window_class->help = call_flow_win_help;

}

static void
call_flow_win_init(CallFlowWindow *self)
{
    // Initialize attributes
    window_set_window_type(TUI_WINDOW(self), WINDOW_CALL_FLOW);

    // Display timestamp next to each arrow
    self->arrowtime = TRUE;

    // Create vectors for columns and flow arrows
    self->columns = NULL;
    self->arrows = g_ptr_array_new_with_free_func(g_free);
}
