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
 * @file call_flow_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions to handle Message arrows display window
 */

#include "config.h"
#include <glib.h>
#include "capture/packet/packet_sip.h"
#include "capture/capture_pcap.h"
#include "ncurses/manager.h"
#include "ncurses/dialog.h"
#include "ncurses/windows/call_flow_win.h"
#include "ncurses/windows/call_raw_win.h"
#include "ncurses/windows/msg_diff_win.h"
#include "ncurses/windows/auth_validate_win.h"
#include "ncurses/windows/save_win.h"
#include "glib-extra.h"

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

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param window UI structure pointer
 * @return a pointer to info structure of given panel
 */
static CallFlowWinInfo *
call_flow_win_info(Window *window)
{
    return (CallFlowWinInfo *) panel_userptr(window->panel);
}

/*
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
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, NULL);

    // No selected call
    if (info->selected == -1)
        return NULL;

    return g_ptr_array_index(info->darrows, info->selected);

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
static GTimeVal
call_flow_arrow_time(const CallFlowArrow *arrow)
{
    GTimeVal ts = { 0 };
    Message *msg;
    RtpStream *stream;

    if (!arrow)
        return ts;

    if (arrow->type == CF_ARROW_SIP) {
        msg = (Message *) arrow->item;
        ts = packet_time(msg->packet);
    } else if (arrow->type == CF_ARROW_RTP) {
        stream = (RtpStream *) arrow->item;
        ts = stream_time(stream);
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
    return timeval_is_older(call_flow_arrow_time(*a), call_flow_arrow_time(*b));
}

/**
 * @brief Filter displayed arrows based on configuration
 */
static int
call_flow_arrow_filter(void *item)
{
    CallFlowArrow *arrow = (CallFlowArrow *) item;

    // SIP arrows are never filtered
    if (arrow->type == CF_ARROW_SIP && setting_disabled(SETTING_CF_ONLYMEDIA))
        return 1;

    // RTP arrows are only displayed when requested
    if (arrow->type == CF_ARROW_RTP) {
        // Display all streams
        if (setting_enabled(SETTING_CF_MEDIA))
            return 1;
        // Otherwise only show active streams
        if (setting_has_value(SETTING_CF_MEDIA, "active"))
            return stream_is_active(arrow->item);
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
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, NULL);
    g_return_val_if_fail(data != NULL, NULL);

    guint index;
    if (g_ptr_array_find_with_equal_func(
        info->arrows, data,
        (GEqualFunc) call_flow_arrow_find_item_cb, &index)) {
        return g_ptr_array_index(info->arrows, index);
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
call_flow_arrow_create(Window *window, void *item, enum CallFlowArrowType type)
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
 * Depending on the arrow tipe and panel display mode lines can
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
        if (setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
            return 1;
        if (!msg_has_sdp(arrow->item))
            return 2;
        if (setting_has_value(SETTING_CF_SDP_INFO, "off"))
            return 2;
        if (setting_has_value(SETTING_CF_SDP_INFO, "first"))
            return 2;
        if (setting_has_value(SETTING_CF_SDP_INFO, "full"))
            return msg_media_count(arrow->item) + 2;
    } else if (arrow->type == CF_ARROW_RTP || arrow->type == CF_ARROW_RTCP) {
        if (setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
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
        RtpStream *stream = arrow->item;
        return stream->msg;
    }

    return NULL;
}

static CallFlowArrow *
call_flow_arrow_find_prev_callid(Window *window, const CallFlowArrow *arrow)
{
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, NULL);
    g_return_val_if_fail(arrow->type == CF_ARROW_SIP, NULL);

    Message *msg = call_flow_arrow_message(arrow);

    // Given arrow index
    gint cur_idx = g_ptr_array_data_index(info->darrows, arrow);

    for (gint i = cur_idx - 1; i > 0; i--) {
        CallFlowArrow *prev = g_ptr_array_index(info->darrows, i);

        if (prev->type != CF_ARROW_SIP)
            continue;

        Message *prev_msg = call_flow_arrow_message(prev);
        g_return_val_if_fail(prev_msg != NULL, NULL);

        if (msg_get_call(msg) == msg_get_call(prev_msg)
            && msg_is_request(msg) == msg_is_request(prev_msg)
            && !msg_is_retrans(prev_msg)) {

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
call_flow_column_get_starting(Window *window, G_GNUC_UNUSED const char *callid, Address addr, guint start)
{
    CallFlowColumn *candidate = NULL;

    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, NULL);

    // Look for address or address:port ?
    gboolean match_port = addr.port != 0;

    // Get alias value for given address
    const gchar *alias = setting_get_alias(addr.ip);

    for (guint i = start; i < g_ptr_array_len(info->columns); i++) {
        CallFlowColumn *column = g_ptr_array_index(info->columns, i);

        // In compressed mode, we search using alias instead of address
        if (setting_enabled(SETTING_CF_SPLITCALLID)) {
            if (g_strcmp0(column->alias, alias) == 0) {
                return column;
            }
        } else {
            // Check if this column matches requested address
            if (match_port) {
                if (addressport_equals(column->addr, addr)) {
                    if (callid && g_list_index(column->callids, (void *) callid) >= 0) {
                        return column;
                    } else {
                        candidate = column;
                    }
                }
            } else {
                // Dont check port
                if (address_equals(column->addr, addr)) {
                    return column;
                }
            }
        }
    }

    return candidate;
}


/**
 * @brief Get a flow column data for a given call-id
 *
 * @param window UI structure pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 *
 * @return column structure pointer or NULL if not found
 */
static CallFlowColumn *
call_flow_column_get_callid(Window *window, const char *callid, Address addr)
{
    return call_flow_column_get_starting(window, callid, addr, 0);
}

/**
 * @brief Get a flow column data
 *
 * @param window UI structure pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 *
 * @return column structure pointer or NULL if not found
 */
static CallFlowColumn *
call_flow_column_get(Window *window, Address addr)
{
    return call_flow_column_get_starting(window, NULL, addr, 0);
}

static void
call_flow_column_add_callid(CallFlowColumn *column, const gchar *callid)
{
    column->callids = g_list_append(column->callids, (gpointer) callid);
}

static gint
call_flow_column_sorter(CallFlowColumn **a, CallFlowColumn **b)
{
    return (*a)->pos - (*b)->pos;
}

static CallFlowColumn *
call_flow_column_create(Window *window, Address addr)
{
    // Get Window info
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, NULL);

    // Create a new column
    CallFlowColumn *column = g_malloc0(sizeof(CallFlowColumn));
    g_return_val_if_fail(column != NULL, NULL);

    column->addr = addr;
    column->alias = setting_get_alias(column->addr.ip);

    // Check if column has externip
    const gchar *twinip = setting_get_externip(column->addr.ip);
    if (twinip != NULL) {
        CallFlowColumn *twin = call_flow_column_get(window, address_from_str(twinip));
        if (twin != NULL) {
            // Set position based on twin column
            twin->twin = column;
            column->twin = twin;
            column->pos = twin->pos + 1;
        }
    }

    // Set position after last existing column
    if (column->twin == NULL) {
        if (g_ptr_array_len(info->columns)) {
            CallFlowColumn *last = g_ptr_array_last(info->columns);
            if (last != NULL) {
                column->pos = last->pos + CF_COLUMN_WIDTH;
            }
        }
    }

    // Add to columns list
    g_ptr_array_add(info->columns, column);
    g_ptr_array_sort(info->columns, (GCompareFunc) call_flow_column_sorter);

    return column;
}

static void
call_flow_arrow_set_columns(Window *window, CallFlowArrow *arrow)
{
    g_return_if_fail(window != NULL);
    g_return_if_fail(arrow != NULL);
    g_return_if_fail(arrow->type == CF_ARROW_SIP);

    // Get Window info
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    // Get arrow information
    Message *msg = arrow->item;
    Call *call = msg_get_call(msg);

    // Find source column for this arrow
    arrow->scolumn = call_flow_column_get_callid(window, call->callid, msg_src_address(msg));
    if (arrow->scolumn == NULL) {
        arrow->scolumn = call_flow_column_create(window, msg_src_address(msg));
    }
    call_flow_column_add_callid(arrow->scolumn, call->callid);

    // Initial requests always goes from left to right (-->)
    if (msg_is_request(msg) && msg_is_initial_transaction(msg) && setting_disabled(SETTING_CF_SPLITCALLID)) {
        guint scolumn_idx = (guint) g_ptr_array_data_index(info->columns, arrow->scolumn);
        arrow->dcolumn = call_flow_column_get_starting(window, call->callid, msg_dst_address(msg), scolumn_idx);
    } else {
        arrow->dcolumn = call_flow_column_get_callid(window, call->callid, msg_dst_address(msg));
    }

    if (arrow->dcolumn == NULL) {
        arrow->dcolumn = call_flow_column_create(window, msg_dst_address(msg));
    }
    call_flow_column_add_callid(arrow->dcolumn, call->callid);
}

/**
 * @brief Draw the footer of the panel with keybindings info
 *
 * @param window UI structure pointer
 */
static void
call_flow_draw_footer(Window *window)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Calls List",
        key_action_key_str(ACTION_CONFIRM), "Raw",
        key_action_key_str(ACTION_SELECT), "Compare",
        key_action_key_str(ACTION_SHOW_HELP), "Help",
        key_action_key_str(ACTION_SDP_INFO), "SDP",
        key_action_key_str(ACTION_TOGGLE_MEDIA), "RTP",
        key_action_key_str(ACTION_SHOW_FLOW_EX), "Extended",
        key_action_key_str(ACTION_COMPRESS), "Compressed",
        key_action_key_str(ACTION_SHOW_RAW), "Raw",
        key_action_key_str(ACTION_CYCLE_COLOR), "Colour by",
        key_action_key_str(ACTION_INCREASE_RAW), "Increase Raw"
    };

    window_draw_bindings(window, keybindings, 22);
}

static void
call_flow_create_arrows(Window *window)
{
    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    // Copy displayed arrows
    // vector_destroy(info->darrows);
    //info->darrows = vector_copy_if(info->arrows, call_flow_arrow_filter);
    info->darrows = info->arrows;

    // Create pending SIP arrows
    Message *msg = NULL;
    while ((msg = call_group_get_next_msg(info->group, msg))) {
        if (!call_flow_arrow_find(window, msg)) {
            CallFlowArrow *arrow = call_flow_arrow_create(window, msg, CF_ARROW_SIP);
            // Get origin and destination column
            call_flow_arrow_set_columns(window, arrow);
            g_ptr_array_add(info->arrows, arrow);
        }
    }

    // Create pending RTP arrows
    RtpStream *stream = NULL;
    while ((stream = call_group_get_next_stream(info->group, stream))) {
        if (!call_flow_arrow_find(window, stream)) {
            CallFlowArrow *arrow = call_flow_arrow_create(window, stream, CF_ARROW_RTP);
            g_ptr_array_add(info->arrows, arrow);
        }
    }

    // Sort arrows by time
    g_ptr_array_sort(info->arrows, (GCompareFunc) call_flow_arrow_time_sorter);
}

/**
 * @brief Draw the visible columns in panel window
 *
 * @param window UI structure pointer
 */
static void
call_flow_draw_columns(Window *window)
{
    Call *call = NULL;
    RtpStream *stream;
    char coltext[SETTING_MAX_LEN];
    Address addr;

    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    // In extended call flow, columns can have multiple call-ids
    if (info->group->callid) {
        info->maxcallids = call_group_count(info->group);
    } else {
        info->maxcallids = 2;
    }

    // Add RTP columns FIXME Really
    if (!setting_disabled(SETTING_CF_MEDIA)) {
        while ((call = call_group_get_next(info->group, call))) {
            for (guint i = 0; i < g_ptr_array_len(call->streams); i++) {
                stream = g_ptr_array_index(call->streams, i);
                if (stream->type == STREAM_RTP && stream_get_count(stream)) {
                    addr = stream->src;
                    addr.port = 0;
                    if (call_flow_column_get(window, addr) == NULL) {
                        call_flow_column_create(window, addr);
                    }
                    addr = stream->dst;
                    addr.port = 0;
                    if (call_flow_column_get(window, addr) == NULL) {
                        call_flow_column_create(window, addr);
                    }
                }
            }
        }
    }

    // Draw columns
    for (guint i = 0; i < g_ptr_array_len(info->columns); i++) {
        CallFlowColumn *column = g_ptr_array_index(info->columns, i);

        mvwvline(info->flow_win, 0, 20 + column->pos, ACS_VLINE, window->height - 6);
        mvwhline(window->win, 3, 10 + column->pos, ACS_HLINE, 20);
        mvwaddch(window->win, 3, 20 + column->pos, ACS_TTEE);

        if (column->twin && column->twin->pos < column->pos) {
            mvwaddch(window->win, 3, 20 + column->twin->pos, ACS_TTEE);
        }

        // Set bold to this address if it's local
        if (setting_enabled(SETTING_CF_LOCALHIGHLIGHT)) {
            if (address_is_local(column->addr))
                wattron(window->win, A_BOLD);
        }

        if (setting_enabled(SETTING_CF_SPLITCALLID) || !column->addr.port) {
            snprintf(coltext, SETTING_MAX_LEN, "%s", column->addr.ip);
        } else if (setting_enabled(SETTING_DISPLAY_ALIAS)) {
            if (strlen(column->addr.ip) > 15) {
                snprintf(coltext, SETTING_MAX_LEN, "..%.*s:%hu",
                         SETTING_MAX_LEN - 7, column->alias + strlen(column->alias) - 13, column->addr.port);
            } else {
                snprintf(coltext, SETTING_MAX_LEN, "%.*s:%hu",
                         SETTING_MAX_LEN - 7, column->alias, column->addr.port);
            }
        } else {
            if (strlen(column->addr.ip) > 15) {
                snprintf(coltext, SETTING_MAX_LEN, "..%.*s:%hu",
                         SETTING_MAX_LEN - 7, column->addr.ip + strlen(column->addr.ip) - 13, column->addr.port);
            } else {
                snprintf(coltext, SETTING_MAX_LEN, "%.*s:%hu",
                         SETTING_MAX_LEN - 7, column->addr.ip, column->addr.port);
            }
        }

        if (column->twin == NULL) {
            mvwprintw(window->win, 2, 10 + column->pos + (22 - strlen(coltext)) / 2, "%s", coltext);
        } else if (column->pos < column->twin->pos) {
            mvwprintw(window->win, 1, 5 + column->pos + (22 - strlen(coltext)) / 2, "%s", coltext);
        } else {
            mvwprintw(window->win, 2, 15 + column->pos + (22 - strlen(coltext)) / 2, "%s", coltext);
        }
        wattroff(window->win, A_BOLD);
    }
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
static int
call_flow_draw_message(Window *window, CallFlowArrow *arrow, guint cline)
{
    char msg_method[ATTR_MAXLEN];
    char msg_time[80];
    char method[ATTR_MAXLEN + 7];
    char delta[15] = { 0 };
    char mediastr[40];
    guint64 color = 0;
    int msglen;

    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, 0);

    // Get the messages window
    WINDOW *flow_win = info->flow_win;

    // Store arrow start line
    arrow->line = cline;
    // Actual arrow position (dashes with < or >)
    guint aline = cline + 1;

    // Calculate how many lines this message requires
    arrow->height = call_flow_arrow_height(window, arrow);

    // Check this message fits on the panel
    if (cline > (guint) (getmaxy(flow_win) + arrow->height))
        return 0;

    Message *msg = arrow->item;
    g_return_val_if_fail(msg != NULL, 0);

    Call *call = msg_get_call(msg);
    g_return_val_if_fail(call != NULL, 0);

    // For extended, use xcallid nstead
    PacketSdpMedia *media = g_list_nth_data(msg->medias, 0);
    timeval_to_time(msg_get_time(msg), msg_time);

    // Get Message method (include extra info)
    sprintf(msg_method, "%s", packet_sip_method_str(msg->packet));
    strcpy(method, msg_method);

    // If message has sdp information
    if (msg_has_sdp(msg) && setting_has_value(SETTING_CF_SDP_INFO, "off")) {
        // Show sdp tag in title
        sprintf(method, "%s (SDP)", msg_method);
    }

    // If message has sdp information
    if (setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
        // Show sdp tag in title
        if (msg_has_sdp(msg)) {
            sprintf(method, "%.*s (SDP)", 12, msg_method);
        } else {
            sprintf(method, "%.*s", 17, msg_method);
        }
    }

    if (msg_has_sdp(msg) && setting_has_value(SETTING_CF_SDP_INFO, "first")) {
        sprintf(method, "%.3s (%s:%u)",
                msg_method,
                media->sconn->address,
                media->rtpport);
    }

    if (msg_has_sdp(msg) && setting_has_value(SETTING_CF_SDP_INFO, "full")) {
        sprintf(method, "%.3s (%s)", msg_method, media->sconn->address);
    }

    // Draw message type or status and line
    msglen = (strlen(method) > 24) ? 24 : strlen(method);

    // Determine start and end position of the arrow line
    int startpos, endpos;
    if (arrow->scolumn == arrow->dcolumn) {
        // Try to follow previous arrow direction
        CallFlowArrow *prev_arrow = call_flow_arrow_find_prev_callid(window, arrow);
        if (prev_arrow != NULL && prev_arrow->dir == CF_ARROW_LEFT) {
            arrow->dir = CF_ARROW_SPIRAL_LEFT;
            startpos = 21 + arrow->dcolumn->pos;
            endpos = 17 + arrow->scolumn->pos;
        } else {
            arrow->dir = CF_ARROW_SPIRAL_RIGHT;
            startpos = 19 + arrow->dcolumn->pos;
            endpos = 20 + arrow->scolumn->pos;
        }
    } else if (arrow->scolumn->pos < arrow->dcolumn->pos) {
        arrow->dir = CF_ARROW_RIGHT;
        startpos = 20 + arrow->scolumn->pos;
        endpos = 20 + arrow->dcolumn->pos;
    } else {
        arrow->dir = CF_ARROW_LEFT;
        startpos = 20 + arrow->dcolumn->pos;
        endpos = 20 + arrow->scolumn->pos;
    }
    int distance = abs(endpos - startpos) - 3;

    // Highlight current message
    if (arrow == g_ptr_array_index(info->darrows, info->cur_idx)) {
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "reverse")) {
            wattron(flow_win, A_REVERSE);
        }
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "bold")) {
            wattron(flow_win, A_BOLD);
        }
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "reversebold")) {
            wattron(flow_win, A_REVERSE);
            wattron(flow_win, A_BOLD);
        }
    }

    // Color the message {
    if (setting_has_value(SETTING_COLORMODE, "request")) {
        // Color by request / response
        color = (msg_is_request(msg)) ? CP_RED_ON_DEF : CP_GREEN_ON_DEF;
    } else if (setting_has_value(SETTING_COLORMODE, "callid")) {
        // Color by call-id
        color = call_group_color(info->group, msg->call);
    } else if (setting_has_value(SETTING_COLORMODE, "cseq")) {
        // Color by CSeq within the same call
        color = packet_sip_cseq(msg->packet) % 7 + 1;
    }

    // Print arrow in the same line than message
    if (setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
        aline = cline;
    }

    // Turn on the message color
    wattron(flow_win, COLOR_PAIR(color));

    // Clear the line
    mvwprintw(flow_win, cline, startpos + 2, "%*s", distance, "");

    // Draw method
    if (arrow->dir == CF_ARROW_SPIRAL_RIGHT) {
        mvwprintw(flow_win, cline, startpos + 5, "%.26s", method);
    } else if (arrow->dir == CF_ARROW_SPIRAL_LEFT) {
        mvwprintw(flow_win, cline, startpos - msglen - 4, "%.26s", method);
    } else {
        mvwprintw(flow_win, cline, startpos + distance / 2 - msglen / 2 + 2, "%.26s", method);
    }

    // Draw media information
    if (msg_has_sdp(msg) && setting_has_value(SETTING_CF_SDP_INFO, "full")) {
        for (GList *l = msg->medias; l != NULL; l = l->next) {
            aline++;
            cline++;
            PacketSdpMedia *media = l->data;
            sprintf(mediastr, "%s %d (%s)",
                    packet_sdp_media_type_str(media->type),
                    media->rtpport,
                    msg_get_preferred_codec_alias(msg));
            mvwprintw(flow_win, cline, startpos + distance / 2 - strlen(mediastr) / 2 + 2, mediastr);
        }
    }

    if (arrow->dir != CF_ARROW_SPIRAL_RIGHT && arrow->dir != CF_ARROW_SPIRAL_LEFT) {
        if (arrow == call_flow_arrow_selected(window)) {
            mvwhline(flow_win, aline, startpos + 2, '=', distance);
        } else {
            mvwhline(flow_win, aline, startpos + 2, ACS_HLINE, distance);
        }
    }

    // Write the arrow at the end of the message (two arrows if this is a retrans)
    if (arrow->dir == CF_ARROW_SPIRAL_RIGHT) {
        mvwaddch(flow_win, aline, startpos + 2, '<');
        if (msg->retrans) {
            mvwaddch(flow_win, aline, startpos + 3, '<');
            mvwaddch(flow_win, aline, startpos + 4, '<');
        }
        // If multiple lines are available, print a spiral icon
        if (aline != cline) {
            mvwaddch(flow_win, aline, startpos + 3, ACS_LRCORNER);
            mvwaddch(flow_win, aline - 1, startpos + 3, ACS_URCORNER);
            mvwaddch(flow_win, aline - 1, startpos + 2, ACS_HLINE);
        }
    } else if (arrow->dir == CF_ARROW_SPIRAL_LEFT) {
        mvwaddch(flow_win, aline, startpos - 2, '>');
        if (msg->retrans) {
            mvwaddch(flow_win, aline, startpos - 3, '>');
            mvwaddch(flow_win, aline, startpos - 4, '>');
        }
        // If multiple lines are available, print a spiral icon
        if (aline != cline) {
            mvwaddch(flow_win, aline, startpos - 3, ACS_LLCORNER);
            mvwaddch(flow_win, aline - 1, startpos - 3, ACS_ULCORNER);
            mvwaddch(flow_win, aline - 1, startpos - 2, ACS_HLINE);
        }
    } else if (arrow->dir == CF_ARROW_RIGHT) {
        mvwaddch(flow_win, aline, endpos - 2, '>');
        if (msg->retrans) {
            mvwaddch(flow_win, aline, endpos - 3, '>');
            mvwaddch(flow_win, aline, endpos - 4, '>');
        }
    } else {
        mvwaddch(flow_win, aline, startpos + 2, '<');
        if (msg->retrans) {
            mvwaddch(flow_win, aline, startpos + 3, '<');
            mvwaddch(flow_win, aline, startpos + 4, '<');
        }
    }

    if (setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
        mvwprintw(flow_win, cline, startpos + distance / 2 - msglen / 2 + 2, " %.26s ", method);

    // Turn off colors
    wattroff(flow_win, COLOR_PAIR(CP_RED_ON_DEF));
    wattroff(flow_win, COLOR_PAIR(CP_GREEN_ON_DEF));
    wattroff(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));
    wattroff(flow_win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    wattroff(flow_win, A_BOLD | A_REVERSE);

    // Print timestamp
    if (info->arrowtime) {
        if (arrow == call_flow_arrow_selected(window))
            wattron(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));

        if (arrow == g_ptr_array_index(info->darrows, info->cur_idx)) {
            wattron(flow_win, A_BOLD);
            mvwprintw(flow_win, cline, 2, "%s", msg_time);
            wattroff(flow_win, A_BOLD);
        } else {
            mvwprintw(flow_win, cline, 2, "%s", msg_time);
        }

        // Print delta from selected message
        if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
            if (info->selected == -1) {
                if (setting_enabled(SETTING_CF_DELTA)) {
                    GTimeVal nextts = msg_get_time(call_group_get_next_msg(info->group, msg));
                    GTimeVal curts = msg_get_time(msg);
                    timeval_to_delta(curts, nextts, delta);
                }
            } else if (arrow == g_ptr_array_index(info->darrows, info->cur_idx)) {
                GTimeVal selts = msg_get_time(call_flow_arrow_message(call_flow_arrow_selected(window)));
                GTimeVal curts = msg_get_time(msg);
                timeval_to_delta(selts, curts, delta);
            }

            if (strlen(delta)) {
                wattron(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));
                mvwprintw(flow_win, cline + 1, 2, "%15s", delta);
            }
            wattroff(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));
        }
    }
    wattroff(flow_win, COLOR_PAIR(CP_CYAN_ON_DEF));

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
call_flow_draw_rtp_stream(Window *window, CallFlowArrow *arrow, int cline)
{
    WINDOW *win;
    char text[50], time[20];
    int height;
    RtpStream *stream = arrow->item;
    Message *msg;
    CallFlowArrow *msgarrow;

    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, 0);

    // Get the messages window
    win = info->flow_win;
    height = getmaxy(win);

    // Store arrow start line
    arrow->line = cline;

    // Calculate how many lines this message requires
    arrow->height = call_flow_arrow_height(window, arrow);

    // Check this media fits on the panel
    if (cline > height + arrow->height)
        return 0;

    // Get arrow text
    sprintf(text, "RTP (%s) %d", stream_get_format(stream), stream_get_count(stream));

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
            call_flow_column_get_callid(window, 0, address_from_str(stream->dst.ip));
    }

    if (arrow->scolumn == NULL) {
        arrow->scolumn =
            call_flow_column_get_callid(window, 0, address_from_str(stream->src.ip));
    }

    // Determine start and end position of the arrow line
    int startpos, endpos;
    if (arrow->scolumn->pos < arrow->dcolumn->pos) {
        arrow->dir = CF_ARROW_RIGHT;
        startpos = 20 + arrow->scolumn->pos;
        endpos = 20 + arrow->dcolumn->pos;
    } else {
        arrow->dir = CF_ARROW_LEFT;
        startpos = 20 + arrow->dcolumn->pos;
        endpos = 20 + arrow->scolumn->pos;
    }
    int distance = 0;

    if (startpos != endpos) {
        // In compressed mode, we display the src and dst port inside the arrow
        // so fixup the stard and end position
        if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
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
        if (stream->src.port < stream->dst.port) {
            arrow->dir = CF_ARROW_RIGHT;
        } else {
            arrow->dir = CF_ARROW_LEFT;
        }
    }

    // Highlight current message
    if (arrow == g_ptr_array_index(info->darrows, info->cur_idx)) {
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "reverse")) {
            wattron(win, A_REVERSE);
        }
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "bold")) {
            wattron(win, A_BOLD);
        }
        if (setting_has_value(SETTING_CF_HIGHTLIGHT, "reversebold")) {
            wattron(win, A_REVERSE);
            wattron(win, A_BOLD);
        }
    }

    // Check if displayed stream is active
    int active = stream_is_active(stream);

    // Clear the line
    mvwprintw(win, cline, startpos + 2, "%*s", distance, "");
    // Draw RTP arrow text
    mvwprintw(win, cline, startpos + (distance) / 2 - strlen(text) / 2 + 2, "%s", text);

    if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
        cline++;

    // Draw line between columns
    if (active)
        mvwhline(win, cline, startpos + 2, '-', distance);
    else
        mvwhline(win, cline, startpos + 2, ACS_HLINE, distance);

    // Write the arrow at the end of the message (two arrows if this is a retrans)
    if (arrow->dir == CF_ARROW_RIGHT) {
        if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
            mvwprintw(win, cline, startpos - 4, "%d", stream->src.port);
            mvwprintw(win, cline, endpos, "%d", stream->dst.port);
        }
        mvwaddch(win, cline, endpos - 2, '>');
        if (active) {
            arrow->rtp_count = stream_get_count(stream);
            arrow->rtp_ind_pos = (arrow->rtp_ind_pos + 1) % distance;
            mvwaddch(win, cline, startpos + arrow->rtp_ind_pos + 2, '>');
        }
    } else {
        if (!setting_has_value(SETTING_CF_SDP_INFO, "compressed")) {
            mvwprintw(win, cline, endpos, "%d", stream->src.port);
            mvwprintw(win, cline, startpos - 4, "%d", stream->dst.port);
        }
        mvwaddch(win, cline, startpos + 2, '<');
        if (active) {
            arrow->rtp_count = stream_get_count(stream);
            arrow->rtp_ind_pos = (arrow->rtp_ind_pos + 1) % distance;
            mvwaddch(win, cline, endpos - arrow->rtp_ind_pos - 2, '<');
        }
    }

    if (setting_has_value(SETTING_CF_SDP_INFO, "compressed"))
        mvwprintw(win, cline, startpos + (distance) / 2 - strlen(text) / 2 + 2, " %s ", text);

    wattroff(win, A_BOLD | A_REVERSE);

    // Print timestamp
    if (info->arrowtime) {
        timeval_to_time(stream_time(stream), time);
        if (arrow == g_ptr_array_index(info->darrows, info->cur_idx)) {
            wattron(win, A_BOLD);
            mvwprintw(win, cline, 2, "%s", time);
            wattroff(win, A_BOLD);
        } else {
            mvwprintw(win, cline, 2, "%s", time);
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
static int
call_flow_draw_arrow(Window *window, CallFlowArrow *arrow, int line)
{
    g_return_val_if_fail(arrow != NULL, 0);

    if (arrow->type == CF_ARROW_SIP) {
        return call_flow_draw_message(window, arrow, line);
    } else {
        return call_flow_draw_rtp_stream(window, arrow, line);
    }
}

/**
 * @brief Draw arrows in the visible part of the panel
 *
 * @param window UI structure pointer
 */
static void
call_flow_draw_arrows(Window *window)
{
    int cline = 0;

    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    // Draw arrows
    for (guint i = info->first_idx; i < g_ptr_array_len(info->darrows); i++) {
        CallFlowArrow *arrow = g_ptr_array_index(info->darrows, i);

        if (!call_flow_arrow_filter(arrow))
            continue;

        // Stop if we have reached the bottom of the screen
        if (cline >= getmaxy(info->flow_win))
            break;

        // Draw arrow
        cline += call_flow_draw_arrow(window, arrow, cline);
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
call_flow_draw_raw(Window *window, Message *msg)
{
    WINDOW *raw_win;
    int raw_width, raw_height;
    int min_raw_width, fixed_raw_width;

    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    // Get min raw width
    min_raw_width = setting_get_intvalue(SETTING_CF_RAWMINWIDTH);
    fixed_raw_width = setting_get_intvalue(SETTING_CF_RAWFIXEDWIDTH);

    // Calculate the raw data width (width - used columns for flow - vertical lines)
    CallFlowColumn *last = g_ptr_array_last(info->columns);
    raw_width = window->width - last->pos - CF_COLUMN_WIDTH - 2;

    // We can define a mininum size for rawminwidth
    if (raw_width < min_raw_width) {
        raw_width = min_raw_width;
    }
    // We can configure an exact raw size
    if (fixed_raw_width > 0) {
        raw_width = fixed_raw_width;
    }

    // Height of raw window is always available size minus 6 lines for header/footer
    raw_height = window->height - 3;

    // If we already have a raw window
    raw_win = info->raw_win;
    if (raw_win) {
        // Check it has the correct size
        if (getmaxx(raw_win) != raw_width) {
            // We need a new raw window
            delwin(raw_win);
            info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
        } else {
            // We have a valid raw win, clear its content
            werase(raw_win);
        }
    } else {
        // Create the raw window of required size
        info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
    }

    // Draw raw box li
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwvline(window->win, 1, window->width - raw_width - 2, ACS_VLINE, window->height - 2);
    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Print msg payload
    draw_message(info->raw_win, msg);

    // Copy the raw_win contents into the panel
    copywin(raw_win, window->win, 0, 0, 1, window->width - raw_width - 1, raw_height, window->width - 2, 0);
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
call_flow_draw_raw_rtcp(Window *window, G_GNUC_UNUSED RtpStream *stream)
{
    /**
     * TODO This is too experimental to even display it
     */
    return 0;

    CallFlowWinInfo *info;
    WINDOW *raw_win;
    int raw_width, raw_height;
    int min_raw_width, fixed_raw_width;

    // Get panel information
    if (!(info = call_flow_win_info(window)))
        return 1;

    // Get min raw width
    min_raw_width = setting_get_intvalue(SETTING_CF_RAWMINWIDTH);
    fixed_raw_width = setting_get_intvalue(SETTING_CF_RAWFIXEDWIDTH);

    // Calculate the raw data width (width - used columns for flow - vertical lines)
    raw_width = window->width - (CF_COLUMN_WIDTH * g_ptr_array_len(info->columns)) - 2;

    // We can define a mininum size for rawminwidth
    if (raw_width < min_raw_width) {
        raw_width = min_raw_width;
    }
    // We can configure an exact raw size
    if (fixed_raw_width > 0) {
        raw_width = fixed_raw_width;
    }

    // Height of raw window is always available size minus 6 lines for header/footer
    raw_height = window->height - 3;

    // If we already have a raw window
    raw_win = info->raw_win;
    if (raw_win) {
        // Check it has the correct size
        if (getmaxx(raw_win) != raw_width) {
            // We need a new raw window
            delwin(raw_win);
            info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
        } else {
            // We have a valid raw win, clear its content
            werase(raw_win);
        }
    } else {
        // Create the raw window of required size
        info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
    }

    // Draw raw box lines
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    mvwvline(window->win, 1, window->width - raw_width - 2, ACS_VLINE, window->height - 2);
    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Copy the raw_win contents into the panel
    copywin(raw_win, window->win, 0, 0, 1, window->width - raw_width - 1, raw_height, window->width - 2, 0);

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
call_flow_draw_preview(Window *window)
{
    CallFlowArrow *arrow = NULL;

    // Check if not displaying raw has been requested
    if (setting_disabled(SETTING_CF_FORCERAW))
        return;

    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    // Draw current arrow preview
    if ((arrow = g_ptr_array_index(info->darrows, info->cur_idx))) {
        if (arrow->type == CF_ARROW_SIP) {
            call_flow_draw_raw(window, arrow->item);
        } else {
            call_flow_draw_raw_rtcp(window, arrow->item);
        }
    }
}

/**
 * @brief Move selected cursor to given arrow
 *
 * This function will move the cursor to given arrow, taking into account
 * selected line and scrolling position.
 *
 * @param window UI structure pointer
 * @param idx Position to move the cursor
 */
static void
call_flow_move(Window *window, guint idx)
{
    int curh = 0;

    // Get panel info
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    // Already in this position?
    if (info->cur_idx == idx)
        return;

    // Get flow subwindow height (for scrolling)
    gint flowh = getmaxy(info->flow_win);

    // Moving down or up?
    gboolean move_down = (info->cur_idx < idx);

    if (move_down) {
        for (guint i = info->cur_idx + 1; i < g_ptr_array_len(info->darrows); i++) {
            CallFlowArrow *arrow = g_ptr_array_index(info->darrows, i);

            if (!call_flow_arrow_filter(arrow))
                continue;

            // Get next selected arrow
            info->cur_idx = i;

            // We have reached our destination
            if (info->cur_idx >= idx) {
                break;
            }
        }
    } else {
        for (gint i = info->cur_idx - 1; i >= 0; i--) {
            CallFlowArrow *arrow = g_ptr_array_index(info->darrows, i);

            if (!call_flow_arrow_filter(arrow))
                continue;

            // Get previous selected arrow
            info->cur_idx = (guint) i;

            // We have reached our destination
            if (info->cur_idx <= idx) {
                break;
            }
        }
    }

    // Update the first displayed arrow
    if (info->cur_idx <= info->first_idx) {
        info->first_idx = info->cur_idx;
    } else {
        // Draw the scrollbar
        for (guint i = info->first_idx; i < g_ptr_array_len(info->darrows); i++) {
            CallFlowArrow *arrow = g_ptr_array_index(info->darrows, i);

            // Increase current arrow height position
            curh += call_flow_arrow_height(window, arrow);
            // If we have reached current arrow
            if (i == info->cur_idx) {
                if (curh > flowh) {
                    // Go to the next first arrow and check if current arrow
                    // is still out of bottom bounds
                    i = info->first_idx;
                    info->first_idx++;
                    curh = 0;
                } else {
                    break;
                }
            }
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
call_flow_move_up(Window *window, guint times)
{
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    gint newpos = info->cur_idx - times;
    if (newpos < 0) newpos = 0;

    call_flow_move(window, (guint) newpos);
}

/**
 * @brief Move selection cursor down N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move down
 */
static void
call_flow_move_down(Window *window, guint times)
{
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    guint newpos = info->cur_idx + times;
    if (newpos >= g_ptr_array_len(info->darrows))
        newpos = g_ptr_array_len(info->darrows) - 1;

    call_flow_move(window, newpos);
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
static int
call_flow_handle_key(Window *window, int key)
{
    int raw_width;
    Window *next_window;
    Call *call = NULL;
    guint rnpag_steps = (guint) setting_get_intvalue(SETTING_CF_SCROLLSTEP);

    // Sanity check, this should not happen
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, KEY_NOT_HANDLED);

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                call_flow_move_down(window, 1);
                break;
            case ACTION_UP:
                call_flow_move_up(window, 1);
                break;
            case ACTION_HNPAGE:
                call_flow_move_down(window, rnpag_steps / 2);;
                break;
            case ACTION_NPAGE:
                call_flow_move_down(window, rnpag_steps);
                break;
            case ACTION_HPPAGE:
                call_flow_move_up(window, rnpag_steps / 2);;
                break;
            case ACTION_PPAGE:
                call_flow_move_up(window, rnpag_steps);
                break;
            case ACTION_BEGIN:
                call_flow_move(window, 0);
                break;
            case ACTION_END:
                call_flow_move(window, g_ptr_array_len(info->darrows));
                break;
            case ACTION_SHOW_FLOW_EX:
                werase(window->win);
                if (call_group_count(info->group) == 1) {
                    call = call_group_get_next(info->group, NULL);
                    call_group_add_calls(info->group, call->xcalls);
                    info->group->callid = call->callid;
                } else {
                    call = call_group_get_next(info->group, NULL);
                    call_group_remove_all(info->group);
                    call_group_add(info->group, call);
                    info->group->callid = 0;
                }
                call_flow_win_set_group(window, info->group);
                break;
            case ACTION_SHOW_RAW:
                // KEY_R, display current call in raw mode
                next_window = ncurses_create_window(WINDOW_CALL_RAW);
                call_raw_win_set_group(next_window, info->group);
                break;
            case ACTION_DECREASE_RAW:
                raw_width = getmaxx(info->raw_win);
                if (raw_width - 2 > 1) {
                    setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, raw_width - 2);
                }
                break;
            case ACTION_INCREASE_RAW:
                raw_width = getmaxx(info->raw_win);
                if (raw_width + 2 < COLS - 1) {
                    setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, raw_width + 2);
                }
                break;
            case ACTION_RESET_RAW:
                setting_set_intvalue(SETTING_CF_RAWFIXEDWIDTH, -1);
                break;
            case ACTION_ONLY_SDP:
                // Toggle SDP mode
                info->group->sdp_only = !(info->group->sdp_only);
                // Disable sdp_only if there are not messages with sdp
                if (call_group_msg_count(info->group) == 0)
                    info->group->sdp_only = 0;
                // Reset screen
                call_flow_win_set_group(window, info->group);
                break;
            case ACTION_SDP_INFO:
                setting_toggle(SETTING_CF_SDP_INFO);
                break;
            case ACTION_ONLY_MEDIA:
                setting_toggle(SETTING_CF_ONLYMEDIA);
                call_flow_win_set_group(window, info->group);
                break;
            case ACTION_TOGGLE_MEDIA:
                setting_toggle(SETTING_CF_MEDIA);
                // Force reload arrows
                call_flow_win_set_group(window, info->group);
                break;
            case ACTION_TOGGLE_RAW:
                setting_toggle(SETTING_CF_FORCERAW);
                break;
            case ACTION_COMPRESS:
                setting_toggle(SETTING_CF_SPLITCALLID);
                // Force columns reload
                call_flow_win_set_group(window, info->group);
                break;
            case ACTION_SAVE:
                if (capture_sources_count(capture_manager()) > 1) {
                    dialog_run("Saving is not possible when multiple input sources are specified.");
                    break;
                }
                next_window = ncurses_create_window(WINDOW_SAVE);
                save_set_group(next_window, info->group);
                save_set_msg(next_window, call_flow_arrow_message(g_ptr_array_index(info->darrows, info->cur_idx)));
                break;
            case ACTION_AUTH_VALIDATE:
                next_window = ncurses_create_window(WINDOW_AUTH_VALIDATE);
                auth_validate_set_group(next_window, info->group);
                break;
            case ACTION_TOGGLE_TIME:
                info->arrowtime = (info->arrowtime) ? FALSE : TRUE;
                break;
            case ACTION_SELECT:
                if (info->selected == -1) {
                    info->selected = info->cur_idx;
                } else {
                    if (info->selected == (gint) info->cur_idx) {
                        info->selected = -1;
                    } else {
                        // Show diff panel
                        next_window = ncurses_create_window(WINDOW_MSG_DIFF);
                        msg_diff_win_set_msgs(next_window,
                                              call_flow_arrow_message(g_ptr_array_index(info->darrows, info->selected)),
                                              call_flow_arrow_message(g_ptr_array_index(info->darrows, info->cur_idx)));
                    }
                }
                break;
            case ACTION_CLEAR:
                info->selected = -1;
                break;
            case ACTION_CONFIRM:
                // KEY_ENTER, display current message in raw mode
                next_window = ncurses_create_window(WINDOW_CALL_RAW);
                call_raw_win_set_group(next_window, info->group);
                call_raw_win_set_msg(next_window,
                                     call_flow_arrow_message(g_ptr_array_index(info->darrows, info->cur_idx)));
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
static int
call_flow_help(G_GNUC_UNUSED Window *window)
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

void
call_flow_win_set_group(Window *window, CallGroup *group)
{
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    g_ptr_array_remove_all(info->columns);
    g_ptr_array_remove_all(info->arrows);

    info->group = group;
    info->cur_idx = info->first_idx = 0;
    info->selected = -1;
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
static int
call_flow_draw(Window *window)
{
    char title[256];

    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_val_if_fail(info != NULL, -1);

    // Get window of main panel
    werase(window->win);

    // Set title
    if (info->group->callid) {
        sprintf(title, "Extended Call flow for %s", info->group->callid);
    } else if (call_group_count(info->group) == 1) {
        Call *call = call_group_get_next(info->group, NULL);
        sprintf(title, "Call flow for %s", call->callid);
    } else {
        sprintf(title, "Call flow for %d dialogs", call_group_count(info->group));
    }

    // Print color mode in title
    if (setting_has_value(SETTING_COLORMODE, "request"))
        strcat(title, " (Color by Request/Response)");
    if (setting_has_value(SETTING_COLORMODE, "callid"))
        strcat(title, " (Color by Call-Id)");
    if (setting_has_value(SETTING_COLORMODE, "cseq"))
        strcat(title, " (Color by CSeq)");

    // Draw panel title
    window_set_title(window, title);

    // Show some keybinding
    call_flow_draw_footer(window);

    // Create pending arrows for SIP and RTP
    call_flow_create_arrows(window);

    // Redraw columns
    call_flow_draw_columns(window);

    // Redraw arrows
    call_flow_draw_arrows(window);

    // Redraw preview
    call_flow_draw_preview(window);

    // Draw the scrollbar
    info->scroll.max = info->scroll.pos = 0;
    for (guint i = 0; i < g_ptr_array_len(info->darrows); i++) {
        CallFlowArrow *arrow = g_ptr_array_index(info->darrows, i);

        // Store current position arrow
        if (i == info->first_idx) {
            info->scroll.pos = info->scroll.max;
        }
        info->scroll.max += call_flow_arrow_height(window, arrow);
    }
    scrollbar_draw(info->scroll);

    // Redraw flow win
    wnoutrefresh(info->flow_win);
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
call_flow_redraw(Window *window)
{
    int maxx, maxy;

    // Get panel information
    CallFlowWinInfo *info = call_flow_win_info(window);
    // Get current screen dimensions
    getmaxyx(stdscr, maxy, maxx);

    // Change the main window size
    wresize(window->win, maxy, maxx);

    // Store new size
    window->width = maxx;
    window->height = maxy;

    // Calculate available printable area
    wresize(info->flow_win, maxy - 6, maxx);

    // Check if any of the group has changed
    return call_group_changed(info->group);
}

void
call_flow_win_free(Window *window)
{
    CallFlowWinInfo *info = call_flow_win_info(window);
    g_return_if_fail(info != NULL);

    // Free the panel information
    g_ptr_array_free(info->columns, TRUE);
    g_ptr_array_free(info->arrows, TRUE);

    // Delete panel windows
    delwin(info->flow_win);
    delwin(info->raw_win);

    // Delete displayed call group
    call_group_free(info->group);
    // Free panel info
    g_free(info);

    window_deinit(window);
}

Window *
call_flow_win_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_CALL_FLOW;
    window->destroy = call_flow_win_free;
    window->redraw = call_flow_redraw;
    window->draw = call_flow_draw;
    window->handle_key = call_flow_handle_key;
    window->help = call_flow_help;

    // Create a new panel to fill all the screen
    window_init(window, getmaxy(stdscr), getmaxx(stdscr));

    // Initialize Call List specific data
    CallFlowWinInfo *info = g_malloc0(sizeof(CallFlowWinInfo));

    // Display timestamp next to each arrow
    info->arrowtime = TRUE;

    // Calculate available printable area for messages
    info->flow_win = subwin(window->win, window->height - 6, window->width - 2, 4, 0);
    info->scroll = window_set_scrollbar(info->flow_win, SB_VERTICAL, SB_LEFT);

    // Create vectors for columns and flow arrows
    info->columns = g_ptr_array_new_with_free_func(g_free);
    info->arrows = g_ptr_array_new_with_free_func(g_free);

    // Store it into panel userptr
    set_panel_userptr(window->panel, (void *) info);

    return window;
}
