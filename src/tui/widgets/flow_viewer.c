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
 * @file flow_viewer.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include "setting.h"
#include "glib-extra/glib.h"
#include "tui/widgets/flow_column.h"
#include "tui/widgets/flow_arrow.h"
#include "tui/widgets/flow_msg_arrow.h"
#include "tui/widgets/flow_rtp_arrow.h"
#include "tui/widgets/flow_viewer.h"

// Class Definition
G_DEFINE_TYPE(SngFlowViewer, sng_flow_viewer, SNG_TYPE_CONTAINER)

SngWidget *
sng_flow_viewer_new()
{
    return g_object_new(
        SNG_TYPE_FLOW_VIEWER,
        "hexpand", TRUE,
        "vexpand", TRUE,
        NULL
    );
}

/**
 * @brief Return selected flow arrow
 *
 * User can select an arrow to compare times or payload with another
 * arrow. Don't confuse this with the current arrow (where the cursor is)
 *
 * @param ui UI Structure pointer
 * @return user selected arrow
 */
static SngFlowArrow *
sng_flow_viewer_arrow_selected(SngFlowViewer *flow_viewer)
{
    // Get panel info
    g_return_val_if_fail(flow_viewer != NULL, NULL);

    if (flow_viewer->selected != NULL)
        return SNG_FLOW_ARROW(flow_viewer->selected);

    return NULL;
}

/**
 * @brief Sort arrows by timestamp
 *
 * This function acts as sorter for arrows vector. Each time a new arrow
 * is appended, it's sorted based on its timestamp.
 */
static gint
sng_flow_viewer_arrow_time_sorter(gpointer *a, gpointer *b)
{
    return sng_flow_arrow_get_time(SNG_FLOW_ARROW(*a))
           > sng_flow_arrow_get_time(SNG_FLOW_ARROW(*b));
}

/**
 * @brief Filter displayed arrows based on configuration
 */
static int
sng_flow_viewer_arrow_filter(void *item)
{
    SngFlowArrow *arrow = (SngFlowArrow *) item;

    // SIP arrows are never filtered
    if (SNG_IS_FLOW_MSG_ARROW(arrow) && setting_disabled(SETTING_TUI_CF_ONLYMEDIA)) {
        return 1;
    }

    // RTP arrows are only displayed when requested
    if (SNG_IS_FLOW_RTP_ARROW(arrow) && setting_enabled(SETTING_TUI_CF_MEDIA)) {
        return 1;
    }

    // Rest of the arrows are never displayed
    return 0;
}

/**
 * @brief Callback for searching arrows by item
 */
static gint
sng_flow_viewer_arrow_find_item_cb(gpointer arrow, gpointer item)
{
    if (SNG_IS_FLOW_MSG_ARROW(arrow)) {
        return sng_flow_msg_arrow_get_message(SNG_FLOW_MSG_ARROW(arrow)) == item ? 0 : 1;
    }

    if (SNG_IS_FLOW_RTP_ARROW(arrow)) {
        return sng_flow_rtp_arrow_get_stream(SNG_FLOW_RTP_ARROW(arrow)) == item ? 0 : 1;
    }

    return 1;
}

/**
 * @brief Return the arrow of a SIP msg or RTP stream
 *
 * This function will try to find an existing arrow with a
 * message or stream equals to the giving pointer.
 *
 * @param flow_viewer UI structure pointer
 * @param data Data to search in the arrow structure
 * @return a pointer to the found arrow or NULL
 */
static SngFlowArrow *
sng_flow_viewer_arrow_find(SngFlowViewer *flow_viewer, gconstpointer data)
{
    g_return_val_if_fail(flow_viewer != NULL, NULL);
    g_return_val_if_fail(data != NULL, NULL);

    GList *arrows = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows));
    GList *arrow = g_list_find_custom(arrows, data, (GCompareFunc) sng_flow_viewer_arrow_find_item_cb);

    if (arrow != NULL) {
        return SNG_FLOW_ARROW(arrow->data);
    }

    return NULL;
}

static SngFlowArrow *
sng_flow_viewer_arrow_find_prev_callid(SngFlowViewer *flow_viewer, SngFlowArrow *arrow)
{
    g_return_val_if_fail(flow_viewer != NULL, NULL);
    g_return_val_if_fail(SNG_IS_FLOW_MSG_ARROW(arrow), NULL);

    Message *msg = sng_flow_msg_arrow_get_message(SNG_FLOW_MSG_ARROW(arrow));

    // Given arrow index
    GList *children = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows));
    g_return_val_if_fail(children != NULL, NULL);

    for (GList *l = children->prev; l != NULL; l = l->prev) {
        SngFlowArrow *prev = SNG_FLOW_ARROW(l->data);

        if (!SNG_IS_FLOW_MSG_ARROW(prev))
            continue;

        Message *prev_msg = sng_flow_msg_arrow_get_message(SNG_FLOW_MSG_ARROW(prev));
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
 * @param flow_viewer UI structure pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 * @param start Column index to start searching
 *
 * @return column structure pointer or NULL if not found
 */
static SngFlowColumn *
sng_flow_viewer_column_get_first(SngFlowViewer *flow_viewer, const Address addr)
{
    g_return_val_if_fail(flow_viewer != NULL, NULL);

    // Look for address or address:port ?
    gboolean match_port = address_get_port(addr) != 0;

    // Get alias value for given address
    const gchar *alias = setting_get_alias(address_get_ip(addr));

    GList *columns = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_columns));
    for (GList *l = columns; l != NULL; l = l->next) {
        SngFlowColumn *column = l->data;

        // In compressed mode, we search using alias instead of address
        if (setting_enabled(SETTING_TUI_CF_SPLITCALLID)) {
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
 * @param flow_viewer UI structure pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 * @param start Column index to start searching
 *
 * @return column structure pointer or NULL if not found
 */
static SngFlowColumn *
sng_flow_viewer_column_get_last(SngFlowViewer *flow_viewer, const Address addr)
{
    g_return_val_if_fail(flow_viewer != NULL, NULL);

    // Look for address or address:port ?
    gboolean match_port = address_get_port(addr) != 0;

    // Get alias value for given address
    const gchar *alias = setting_get_alias(address_get_ip(addr));

    GList *columns = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_columns));
    for (GList *l = g_list_last(columns); l != NULL; l = l->prev) {
        SngFlowColumn *column = l->data;

        // In compressed mode, we search using alias instead of address
        if (setting_enabled(SETTING_TUI_CF_SPLITCALLID)) {
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

void
sng_flow_viewer_set_group(SngFlowViewer *flow_viewer, CallGroup *group)
{
    g_return_if_fail(flow_viewer != NULL);

    flow_viewer->group = group;

    sng_container_remove_all(SNG_CONTAINER(flow_viewer->box_columns));
    sng_container_remove_all(SNG_CONTAINER(flow_viewer->box_arrows));
    flow_viewer->selected = NULL;
    flow_viewer->current = NULL;
}

CallGroup *
sng_flow_viewer_get_group(SngFlowViewer *flow_viewer)
{
    return flow_viewer->group;
}

SngFlowArrow *
sng_flow_viewer_get_current(SngFlowViewer *flow_viewer)
{
    return SNG_FLOW_ARROW(flow_viewer->current);
}

static void
sng_flow_viewer_arrow_set_columns(SngFlowViewer *flow_viewer, SngFlowArrow *arrow, SngFlowArrowDir dir)
{
    g_return_if_fail(flow_viewer != NULL);
    g_return_if_fail(arrow != NULL);
    g_return_if_fail(SNG_IS_FLOW_MSG_ARROW(arrow));

    // Get Window info
    g_return_if_fail(flow_viewer != NULL);

    // Get arrow information
    Message *msg = sng_flow_msg_arrow_get_message(SNG_FLOW_MSG_ARROW(arrow));

    if (dir == CF_ARROW_DIR_ANY) {
        // Try to reuse current call columns if found
        Call *call = msg_get_call(msg);
        g_return_if_fail(call != NULL);

        GPtrArray *msgs = call->msgs;
        for (guint i = 0; i < g_ptr_array_len(msgs); i++) {
            SngFlowArrow *msg_arrow = sng_flow_viewer_arrow_find(flow_viewer, g_ptr_array_index(msgs, i));

            SngFlowColumn *scolumn = sng_flow_arrow_get_src_column(msg_arrow);
            SngFlowColumn *dcolumn = sng_flow_arrow_get_dst_column(msg_arrow);

            if (msg_arrow == NULL)
                continue;

            if (!SNG_IS_FLOW_MSG_ARROW(msg_arrow))
                continue;

            if (sng_flow_arrow_get_src_column(msg_arrow) == NULL)
                continue;

            if (sng_flow_arrow_get_dst_column(msg_arrow) == NULL)
                continue;

            if (msg_arrow == arrow)
                break;

            if (addressport_equals(msg_src_address(msg), scolumn->addr)
                && addressport_equals(msg_dst_address(msg), dcolumn->addr)) {
                sng_flow_arrow_set_src_column(arrow, scolumn);
                sng_flow_arrow_set_dst_column(arrow, dcolumn);
                break;
            }

            if (addressport_equals(msg_src_address(msg), dcolumn->addr)
                && addressport_equals(msg_dst_address(msg), scolumn->addr)) {
                sng_flow_arrow_set_src_column(arrow, dcolumn);
                sng_flow_arrow_set_dst_column(arrow, scolumn);
                break;
            }
        }
    } else if (dir == CF_ARROW_DIR_RIGHT) {
        sng_flow_arrow_set_src_column(
            arrow,
            sng_flow_viewer_column_get_first(flow_viewer, msg_src_address(msg))
        );

        GList *columns = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_columns));
        GList *lscolumn = g_list_find(columns, sng_flow_arrow_get_src_column(arrow));
        for (GList *l = lscolumn; l != NULL; l = l->next) {
            SngFlowColumn *dcolumn = l->data;
            if (addressport_equals(msg_dst_address(msg), dcolumn->addr)) {
                sng_flow_arrow_set_dst_column(arrow, dcolumn);

                // Check if there is a source column with src address is nearer
                for (GList *m = l; m != NULL; m = m->prev) {
                    SngFlowColumn *scolumn = m->data;
                    if (addressport_equals(msg_src_address(msg), scolumn->addr)) {
                        sng_flow_arrow_set_src_column(arrow, scolumn);
                        break;
                    }
                }

                break;
            }
        }

        // If we need to create destination arrow, use nearest source column to the end
        if (sng_flow_arrow_get_dst_column(arrow) == NULL) {
            sng_flow_arrow_set_dst_column(
                arrow,
                sng_flow_viewer_column_get_last(flow_viewer, msg_src_address(msg))
            );
        }

    } else if (dir == CF_ARROW_DIR_LEFT) {
        sng_flow_arrow_set_src_column(
            arrow,
            sng_flow_viewer_column_get_last(flow_viewer, msg_src_address(msg))
        );

        GList *columns = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_columns));
        GList *lscolumn = g_list_find(columns, sng_flow_arrow_get_src_column(arrow));
        for (GList *l = lscolumn; l != NULL; l = l->prev) {
            SngFlowColumn *dcolumn = l->data;
            if (addressport_equals(msg_dst_address(msg), dcolumn->addr)) {
                sng_flow_arrow_set_dst_column(arrow, dcolumn);

                // Check if there is a destination column with dst address is nearer
                for (GList *m = l; m != NULL; m = m->next) {
                    SngFlowColumn *scolumn = m->data;
                    if (addressport_equals(msg_src_address(msg), scolumn->addr)) {
                        sng_flow_arrow_set_src_column(arrow, scolumn);
                        break;
                    }
                }

                break;
            }
        }

        // If we need to create destination arrow, use nearest source column to the end
        if (sng_flow_arrow_get_dst_column(arrow) == NULL) {
            sng_flow_arrow_set_dst_column(
                arrow,
                sng_flow_viewer_column_get_last(flow_viewer, msg_dst_address(msg))
            );
        }
    }

    // Create any non-existent columns
    if (sng_flow_arrow_get_src_column(arrow) == NULL) {
        SngWidget *column = sng_flow_column_new(msg_src_address(msg));
        sng_box_pack_start(SNG_BOX(flow_viewer->box_columns), column);
        sng_flow_arrow_set_src_column(arrow, SNG_FLOW_COLUMN(column));
    }

    if (sng_flow_arrow_get_dst_column(arrow) == NULL) {
        SngWidget *column = sng_flow_column_new(msg_dst_address(msg));
        sng_box_pack_start(SNG_BOX(flow_viewer->box_columns), column);
        sng_flow_arrow_set_dst_column(arrow, SNG_FLOW_COLUMN(column));
    }
}

gint
sng_flow_viewer_columns_width(SngFlowViewer *flow_viewer)
{
    g_return_val_if_fail(flow_viewer != NULL, 0);
    GList *columns = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_columns));
    return g_list_length(columns) * CF_COLUMN_WIDTH + 2;
}

static gint
sng_flow_viewer_arrows_height(SngFlowViewer *flow_viewer)
{
    g_return_val_if_fail(flow_viewer != NULL, 0);

    gint height = 0;
    GList *arrows = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows));
    for (GList *l = arrows; l != NULL; l = l->next) {
        height += sng_widget_get_preferred_height(SNG_WIDGET(l->data));
    }

    return height;
}

static void
sng_flow_viewer_create_arrows(SngFlowViewer *flow_viewer)
{
    // Get panel information
    g_return_if_fail(flow_viewer != NULL);

    // Create pending SIP arrows
    Message *msg = NULL;
    while ((msg = call_group_get_next_msg(flow_viewer->group, msg))) {
        if (sng_flow_viewer_arrow_find(flow_viewer, msg) == NULL) {
            sng_box_pack_start(
                SNG_BOX(flow_viewer->box_arrows),
                sng_flow_msg_arrow_new(msg)
            );
        }
    }

//    // Create pending RTP arrows
//    Stream *stream = NULL;
//    while ((stream = call_group_get_next_stream(flow_viewer->group, stream))) {
//        if (!sng_flow_viewer_arrow_find(flow_viewer, stream)) {
//            SngWidget *arrow = sng_flow_rtp_arrow_new(stream);
//            g_ptr_array_add(flow_viewer->arrows, arrow);
//            sng_box_pack_start(
//                SNG_BOX(flow_viewer->box_arrows),
//                arrow
//            );
//        }
//    }

    // Sort arrows by time
//    GList *arrows = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows));
//    arrows = g_list_sort(arrows, (GCompareFunc) sng_flow_viewer_arrow_time_sorter);
//    sng_container_set_children(SNG_CONTAINER(flow_viewer->box_arrows), arrows);
}

void
sng_flow_viewer_create_columns(SngFlowViewer *flow_viewer)
{
    // Set arrow columns after sorting arrows by time
    GList *arrows = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows));
    for (GList *l = arrows; l != NULL; l = l->next) {
        SngFlowArrow *arrow = l->data;

        if (!SNG_IS_FLOW_MSG_ARROW(arrow))
            continue;

        Message *msg = sng_flow_msg_arrow_get_message(SNG_FLOW_MSG_ARROW(arrow));

        if (setting_disabled(SETTING_TUI_CF_SPLITCALLID)
            && msg_is_initial_transaction(msg)) {
            // Force Initial Transaction Arrows direction
            sng_flow_viewer_arrow_set_columns(
                flow_viewer, arrow,
                msg_is_request(msg) ? CF_ARROW_DIR_RIGHT : CF_ARROW_DIR_LEFT
            );
        } else {
            // Get origin and destination column
            sng_flow_viewer_arrow_set_columns(
                flow_viewer, arrow,
                CF_ARROW_DIR_ANY
            );
        }
    }
}

/**
 * @brief Move selection cursor up N times
 *
 * @param flow_viewer UI structure pointer
 * @param times number of lines to move up
 */
static void
sng_flow_viewer_move_vertical(SngFlowViewer *flow_viewer, gint times)
{
    g_return_if_fail(flow_viewer != NULL);

    GList *arrows = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows));
    gint index = g_list_index(arrows, flow_viewer->current);
    g_return_if_fail(index != -1);

    // Set the new current selected index
    index = CLAMP(
        index + times,
        0,
        (gint) g_list_length(arrows) - 1
    );

    // Change focus from previous arrow to new one
    sng_widget_focus_lost(flow_viewer->current);
    flow_viewer->current = g_list_nth_data(arrows, index);
    sng_widget_focus_gain(flow_viewer->current);

    // Move the first index if required (moving up)
    flow_viewer->vscroll.pos = MIN(
        flow_viewer->vscroll.pos,
        sng_widget_get_ypos(flow_viewer->current) - sng_widget_get_height(flow_viewer->current) + 1
    );

    gint arrow_win_height = getmaxy(sng_widget_get_ncurses_window(flow_viewer->box_arrows));
    if (scrollbar_visible(flow_viewer->hscroll)) {
        arrow_win_height -= 1;
    }

    // Move the first index if required (moving down)
    flow_viewer->vscroll.pos = MAX(
        flow_viewer->vscroll.pos,
        sng_widget_get_ypos(flow_viewer->current) - arrow_win_height
    );
}


/**
 * @brief Move selection cursor up N times
 *
 * @param flow_viewer UI structure pointer
 * @param times number of lines to move up
 */
static void
sng_flow_viewer_move_horizontal(SngFlowViewer *flow_viewer, gint times)
{
    g_return_if_fail(flow_viewer != NULL);

    // Move the first index if required (moving left)
    flow_viewer->hscroll.pos = CLAMP(
        flow_viewer->hscroll.pos + times,
        0,
        sng_widget_get_width(flow_viewer->box_columns)
    );
}

static void
sng_flow_viewer_update(SngWidget *widget)
{
    SngFlowViewer *flow_viewer = SNG_FLOW_VIWER(widget);

    // Create arrows for SIP and RTP
    sng_flow_viewer_create_arrows(flow_viewer);

    // Create columns and assign arrows' columns
    sng_flow_viewer_create_columns(flow_viewer);

    // Set focus on first arrow
    if (flow_viewer->current == NULL) {
        GList *arrows = sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows));
        flow_viewer->current = g_list_first_data(arrows);
        sng_widget_focus_gain(flow_viewer->current);
    }
}

static void
sng_flow_viewer_size_request(SngWidget *widget)
{
    SngFlowViewer *flow_viewer = SNG_FLOW_VIWER(widget);

    // Change size and position of Columns Box
    sng_widget_set_size(
        flow_viewer->box_columns,
        MAX(
            sng_flow_viewer_columns_width(flow_viewer),
            sng_widget_get_width(widget)
        ),
        sng_widget_get_height(widget)
    );
    sng_widget_set_position(
        flow_viewer->box_columns,
        sng_widget_get_xpos(widget),
        sng_widget_get_ypos(widget)
    );
    sng_widget_size_request(flow_viewer->box_columns);

    // Change size and position of Arrows Box
    sng_widget_set_size(
        flow_viewer->box_arrows,
        MAX(
            sng_flow_viewer_columns_width(flow_viewer),
            sng_widget_get_width(widget)
        ),
        MAX(
            sng_flow_viewer_arrows_height(flow_viewer),
            sng_widget_get_height(widget)
        )
    );
    sng_widget_set_position(
        flow_viewer->box_arrows,
        sng_widget_get_xpos(widget),
        sng_widget_get_ypos(widget)
    );
    sng_widget_size_request(flow_viewer->box_arrows);
}

static void
sng_flow_viewer_realize(SngWidget *widget)
{
    SngFlowViewer *flow_viewer = SNG_FLOW_VIWER(widget);

    // Update Box internal Ncurses window
    sng_widget_realize(flow_viewer->box_columns);
    sng_widget_realize(flow_viewer->box_arrows);

    // Calculate available printable area for messages
    flow_viewer->vscroll = window_set_scrollbar(sng_widget_get_ncurses_window(flow_viewer->box_arrows), SB_VERTICAL,
                                                SB_LEFT);
    flow_viewer->hscroll = window_set_scrollbar(sng_widget_get_ncurses_window(flow_viewer->box_arrows), SB_HORIZONTAL,
                                                SB_BOTTOM);
    flow_viewer->vscroll.max = sng_flow_viewer_arrows_height(flow_viewer) - 1;
    flow_viewer->hscroll.max = sng_flow_viewer_columns_width(flow_viewer) - 1;
    flow_viewer->vscroll.postoffset = (scrollbar_visible(flow_viewer->hscroll) ? 1 : 0);
    flow_viewer->hscroll.preoffset = (scrollbar_visible(flow_viewer->vscroll) ? 1 : 0);

    // Create subwindows for all components
    SNG_WIDGET_CLASS(sng_flow_viewer_parent_class)->realize(widget);
}

static void
sng_flow_viewer_draw(SngWidget *widget)
{
    SngFlowViewer *flow_viewer = SNG_FLOW_VIWER(widget);

    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wbkgdset(win, COLOR_PAIR(CP_WHITE_ON_CYAN));
    werase(win);

    // Draw columns and arrows
    sng_widget_draw(flow_viewer->box_columns);
    sng_widget_draw(flow_viewer->box_arrows);

    // Draw scrollbars
    scrollbar_draw(flow_viewer->vscroll);
    scrollbar_draw(flow_viewer->hscroll);

    // Chain-up parent draw function
    SNG_WIDGET_CLASS(sng_flow_viewer_parent_class)->draw(widget);
}

static void
sng_flow_viewer_map(SngWidget *widget)
{
    SngFlowViewer *flow_viewer = SNG_FLOW_VIWER(widget);

    WINDOW *win = sng_widget_get_ncurses_window(widget);

    // Map Column Box in flow viewer
    sng_widget_map(flow_viewer->box_columns);
    copywin(
        sng_widget_get_ncurses_window(flow_viewer->box_columns),
        win,
        0, 0, 0, 0,
        sng_widget_get_height(widget) - 1,
        sng_widget_get_width(widget) - 1,
        FALSE
    );

    // Map Arrow Box in flow viewer
    sng_widget_map(flow_viewer->box_arrows);
    copywin(
        sng_widget_get_ncurses_window(flow_viewer->box_arrows),
        win,
        0, 0, 0, 0,
        sng_widget_get_height(widget) - 1,
        sng_widget_get_width(widget) - 1,
        TRUE
    );

    // Chain-up parent map function
    SNG_WIDGET_CLASS(sng_flow_viewer_parent_class)->map(widget);
}

/**
 * @brief Handle Call flow extended key strokes
 *
 * This function will manage the custom keybindings of the panel.
 * This function return one of the values defined in @key_handler_ret
 *
 * @param flow_viewer UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
static void
sng_flow_viewer_handle_key(SngWidget *widget, gint key)
{
    guint rnpag_steps = (guint) setting_get_intvalue(SETTING_TUI_CF_SCROLLSTEP);

    // Sanity check, this should not happen
    SngFlowViewer *flow_viewer = SNG_FLOW_VIWER(widget);

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                sng_flow_viewer_move_vertical(flow_viewer, 1);
                break;
            case ACTION_UP:
                sng_flow_viewer_move_vertical(flow_viewer, -1);
                break;
            case ACTION_RIGHT:
                sng_flow_viewer_move_horizontal(flow_viewer, 15);
                break;
            case ACTION_LEFT:
                sng_flow_viewer_move_horizontal(flow_viewer, -15);
                break;
            case ACTION_HNPAGE:
                sng_flow_viewer_move_vertical(flow_viewer, rnpag_steps / 2);;
                break;
            case ACTION_NPAGE:
                sng_flow_viewer_move_vertical(flow_viewer, rnpag_steps);
                break;
            case ACTION_HPPAGE:
                sng_flow_viewer_move_vertical(flow_viewer, -1 * rnpag_steps / 2);;
                break;
            case ACTION_PPAGE:
                sng_flow_viewer_move_vertical(flow_viewer, -1 * rnpag_steps);
                break;
            case ACTION_BEGIN:
                sng_flow_viewer_move_vertical(
                    flow_viewer,
                    -1 * g_list_length(sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows)))
                );
                break;
            case ACTION_END:
                sng_flow_viewer_move_vertical(
                    flow_viewer,
                    g_list_length(sng_container_get_children(SNG_CONTAINER(flow_viewer->box_arrows)))
                );
                break;
            case ACTION_RESET_RAW:
                setting_set_intvalue(SETTING_TUI_CF_RAWFIXEDWIDTH, -1);
                break;
            case ACTION_ONLY_SDP:
                // Toggle SDP mode
                flow_viewer->group->sdp_only = !(flow_viewer->group->sdp_only);
                // Disable sdp_only if there are not messages with sdp
                if (call_group_msg_count(flow_viewer->group) == 0)
                    flow_viewer->group->sdp_only = 0;
                // Reset screen
                sng_flow_viewer_set_group(flow_viewer, flow_viewer->group);
                break;
            case ACTION_SDP_INFO:
                setting_toggle(SETTING_TUI_CF_SDP_INFO);
                break;
            case ACTION_HIDE_DUPLICATE:
                setting_toggle(SETTING_TUI_CF_HIDEDUPLICATE);
                sng_flow_viewer_set_group(flow_viewer, flow_viewer->group);
                break;
            case ACTION_ONLY_MEDIA:
                setting_toggle(SETTING_TUI_CF_ONLYMEDIA);
                sng_flow_viewer_set_group(flow_viewer, flow_viewer->group);
                break;
            case ACTION_TOGGLE_MEDIA:
                setting_toggle(SETTING_TUI_CF_MEDIA);
                // Force reload arrows
                sng_flow_viewer_set_group(flow_viewer, flow_viewer->group);
                break;
            case ACTION_TOGGLE_RAW:
                setting_toggle(SETTING_TUI_CF_FORCERAW);
                break;
            case ACTION_COMPRESS:
                setting_toggle(SETTING_TUI_CF_SPLITCALLID);
                // Force columns reload
                sng_flow_viewer_set_group(flow_viewer, flow_viewer->group);
                break;
                break;
            case ACTION_TOGGLE_TIME:
                flow_viewer->arrowtime = (flow_viewer->arrowtime) ? FALSE : TRUE;
                break;
            case ACTION_CLEAR:
                flow_viewer->selected = NULL;
                break;
            case ACTION_CLEAR_CALLS:
            case ACTION_CLEAR_CALLS_SOFT:
                // Propagate the key to the previous panel
                return;
            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Key not handled, check parent
    SNG_WIDGET_CLASS(sng_flow_viewer_parent_class)->key_pressed(widget, key);
}

static void
sng_flow_viewer_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(sng_flow_viewer_parent_class)->constructed(object);

    // Get parent window information
    SngFlowViewer *flow_viewer = SNG_FLOW_VIWER(object);

    // Create a container for columns
    flow_viewer->box_columns = sng_box_new(SNG_ORIENTATION_HORIZONTAL);
    sng_box_set_padding_full(SNG_BOX(flow_viewer->box_columns), 1, 0, 0, 0);

    flow_viewer->box_arrows = sng_box_new(SNG_ORIENTATION_VERTICAL);
    sng_box_set_padding_full(SNG_BOX(flow_viewer->box_arrows), 3, 0, 0, 0);
}

static void
sng_flow_viewer_finalized(GObject *object)
{
    SngFlowViewer *flow_viewer = SNG_FLOW_VIWER(object);
    g_return_if_fail(flow_viewer != NULL);

    // Delete displayed call group
    call_group_free(flow_viewer->group);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(sng_flow_viewer_parent_class)->finalize(object);
}

static void
sng_flow_viewer_class_init(SngFlowViewerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = sng_flow_viewer_constructed;
    object_class->finalize = sng_flow_viewer_finalized;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->update = sng_flow_viewer_update;
    widget_class->size_request = sng_flow_viewer_size_request;
    widget_class->realize = sng_flow_viewer_realize;
    widget_class->draw = sng_flow_viewer_draw;
    widget_class->map = sng_flow_viewer_map;
    widget_class->key_pressed = sng_flow_viewer_handle_key;
}

static void
sng_flow_viewer_init(SngFlowViewer *flow_viewer)
{
    // Display timestamp next to each arrow
    flow_viewer->arrowtime = TRUE;
}
