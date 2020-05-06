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
#include "tui/tui.h"
#include "tui/dialog.h"
#include "tui/windows/call_flow_win.h"
#include "tui/windows/call_raw_win.h"
#include "tui/windows/msg_diff_win.h"
#include "tui/windows/auth_validate_win.h"
#include "tui/windows/save_win.h"
#include "tui/widgets/flow_arrow.h"
#include "tui/widgets/flow_msg_arrow.h"
#include "tui/widgets/flow_rtp_arrow.h"
#include "tui/widgets/flow_viewer.h"
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

G_DEFINE_TYPE(CallFlowWindow, call_flow_win, SNG_TYPE_APP_WINDOW)

SngAppWindow *
call_flow_win_new()
{
    return g_object_new(
        WINDOW_TYPE_CALL_FLOW,
        "height", getmaxy(stdscr),
        "width", getmaxx(stdscr),
        NULL
    );
}

/**
 * @brief Draw the footer of the panel with keybindings info
 *
 * @param call_flow_window UI structure pointer
 */
static void
call_flow_win_draw_footer(CallFlowWindow *call_flow_window)
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

}

/**
 * @brief Draw panel preview of current arrow
 *
 * If user request to not draw preview panel, this function does nothing.
 *
 * @param window UI structure pointer
 */
static void
call_flow_win_draw_preview(CallFlowWindow *call_flow_window)
{
    SngFlowArrow *arrow = NULL;

    // Check if not displaying raw has been requested
    if (setting_disabled(SETTING_TUI_CF_FORCERAW))
        return;

    // Draw current arrow preview
    if ((arrow = sng_flow_viewer_get_current(SNG_FLOW_VIWER(call_flow_window->flow_viewer)))) {
    }
}

void
call_flow_win_set_group(CallFlowWindow *call_flow_win, CallGroup *group)
{
    sng_flow_viewer_set_group(
        SNG_FLOW_VIWER(call_flow_win->flow_viewer),
        group
    );
}

static void
call_flow_win_size_request(SngWidget *widget)
{
    CallFlowWindow *call_flow_win = TUI_CALL_FLOW(widget);

    // Get min raw width
    gint width = sng_widget_get_width(SNG_WIDGET(call_flow_win));
    gint flow_viewer_width = sng_flow_viewer_columns_width(SNG_FLOW_VIWER(call_flow_win->flow_viewer));
    gint min_raw_width = setting_get_intvalue(SETTING_TUI_CF_RAWMINWIDTH);
    gint fixed_raw_width = setting_get_intvalue(SETTING_TUI_CF_RAWFIXEDWIDTH);

    // We can configure an exact detail size
    if (fixed_raw_width > 0) {
        sng_widget_set_width(call_flow_win->box_detail, fixed_raw_width);
    } else {
        sng_widget_set_width(
            call_flow_win->box_detail,
            MAX(width - flow_viewer_width, min_raw_width)
        );
    }

    // Chain-up parent size request function
    SNG_WIDGET_CLASS(call_flow_win_parent_class)->size_request(widget);
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
static void
call_flow_win_handle_key(SngWidget *widget, gint key)
{
    int raw_width;
    SngAppWindow *next_app_window;
    Call *call = NULL;
    SngFlowArrow *cur_arrow = NULL;

    // Sanity check, this should not happen
    SngAppWindow *window = SNG_APP_WINDOW(widget);
    CallFlowWindow *call_flow_window = TUI_CALL_FLOW(window);
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    CallGroup *group = sng_flow_viewer_get_group(SNG_FLOW_VIWER(call_flow_window->flow_viewer));

    // Check actions for this key
    SngAction action = ACTION_NONE;
    while ((action = key_find_action(key, action)) != ACTION_NONE) {
        // Check if we handle this action
        switch (action) {
            case ACTION_SHOW_FLOW_EX:
                werase(win);
                if (call_group_count(group) == 1) {
                    call = call_group_get_next(group, NULL);
                    call_group_add_calls(group, call->xcalls);
                    group->callid = call->callid;
                } else {
                    call = call_group_get_next(group, NULL);
                    call_group_remove_all(group);
                    call_group_add(group, call);
                    group->callid = 0;
                }
                sng_flow_viewer_set_group(SNG_FLOW_VIWER(call_flow_window->flow_viewer), group);
                break;
//            case ACTION_SHOW_RAW:
//                // KEY_R, display current call in raw mode
//                next_app_window = tui_create_app_window(SNG_WINDOW_TYPE_CALL_RAW);
//                call_raw_win_set_group(next_app_window, call_flow_window->group);
//                break;
            case ACTION_DECREASE_RAW:
                raw_width = sng_widget_get_width(call_flow_window->box_detail);
                if (raw_width - 2 > 1) {
                    setting_set_intvalue(SETTING_TUI_CF_RAWFIXEDWIDTH, raw_width - 2);
                }
                break;
            case ACTION_INCREASE_RAW:
                raw_width = MIN(sng_widget_get_width(call_flow_window->box_detail) + 2, sng_widget_get_width(SNG_WIDGET(window)) - 1);
                setting_set_intvalue(SETTING_TUI_CF_RAWFIXEDWIDTH, raw_width);
                break;
            case ACTION_RESET_RAW:
                setting_set_intvalue(SETTING_TUI_CF_RAWFIXEDWIDTH, -1);
                break;
            case ACTION_ONLY_SDP:
                // Toggle SDP mode
                group->sdp_only = !(group->sdp_only);
                // Disable sdp_only if there are not messages with sdp
                if (call_group_msg_count(group) == 0)
                    group->sdp_only = 0;
                // Reset screen
                sng_flow_viewer_set_group(SNG_FLOW_VIWER(call_flow_window->flow_viewer), group);
                break;
            case ACTION_SDP_INFO:
                setting_toggle(SETTING_TUI_CF_SDP_INFO);
                break;
            case ACTION_HIDE_DUPLICATE:
                setting_toggle(SETTING_TUI_CF_HIDEDUPLICATE);
                sng_flow_viewer_set_group(SNG_FLOW_VIWER(call_flow_window->flow_viewer), group);
                break;
            case ACTION_ONLY_MEDIA:
                setting_toggle(SETTING_TUI_CF_ONLYMEDIA);
                sng_flow_viewer_set_group(SNG_FLOW_VIWER(call_flow_window->flow_viewer), group);
                break;
            case ACTION_TOGGLE_MEDIA:
                setting_toggle(SETTING_TUI_CF_MEDIA);
                // Force reload arrows
                sng_flow_viewer_set_group(SNG_FLOW_VIWER(call_flow_window->flow_viewer), group);
                break;
            case ACTION_TOGGLE_RAW:
                setting_toggle(SETTING_TUI_CF_FORCERAW);
                break;
            case ACTION_COMPRESS:
                setting_toggle(SETTING_TUI_CF_SPLITCALLID);
                // Force columns reload
                sng_flow_viewer_set_group(SNG_FLOW_VIWER(call_flow_window->flow_viewer), group);
                break;
            case ACTION_SAVE:
                cur_arrow = sng_flow_viewer_get_current(SNG_FLOW_VIWER(call_flow_window->flow_viewer));
                if (SNG_IS_FLOW_MSG_ARROW(cur_arrow)) {
                    next_app_window = tui_create_app_window(SNG_WINDOW_TYPE_SAVE);
                    save_set_group(TUI_SAVE(next_app_window), group);
                    save_set_message(
                        TUI_SAVE(next_app_window),
                        sng_flow_msg_arrow_get_message(SNG_FLOW_MSG_ARROW(cur_arrow))
                    );
                }
#ifdef WITH_SND
                if (SNG_IS_FLOW_RTP_ARROW(cur_arrow)) {
                    StorageCaptureOpts storageCaptureOpts = storage_capture_options();
                    if (!storageCaptureOpts.rtp) {
                        dialog_run("RTP packets are not being stored, run with --rtp flag.");
                        break;
                    }
                    next_app_window = tui_create_app_window(SNG_WINDOW_TYPE_SAVE);
                    save_set_stream(
                        TUI_SAVE(next_app_window),
                        sng_flow_rtp_arrow_get_stream(SNG_FLOW_RTP_ARROW(cur_arrow))
                    );
                }
#endif
                break;
#ifdef WITH_PULSE
            case ACTION_SHOW_PLAYER:
                cur_arrow = sng_flow_viewer_get_current(SNG_FLOW_VIWER(call_flow_window->flow_viewer));
                if (SNG_IS_FLOW_RTP_ARROW(cur_arrow)) {
                    StorageCaptureOpts storageCaptureOpts = storage_capture_options();
                    if (!storageCaptureOpts.rtp) {
                        dialog_run("RTP packets are not being stored, run with --rtp flag.");
                        break;
                    }

                    next_app_window = tui_create_app_window(SNG_WINDOW_TYPE_RTP_PLAYER);
                    if (next_app_window != NULL) {
                        rtp_player_win_set_stream(
                            next_app_window,
                            sng_flow_rtp_arrow_get_stream(SNG_FLOW_RTP_ARROW(cur_arrow))
                        );
                    }
                }
                break;
#endif
            case ACTION_AUTH_VALIDATE:
                next_app_window = tui_create_app_window(SNG_WINDOW_TYPE_AUTH_VALIDATE);
                auth_validate_win_set_group(next_app_window, group);
                break;
//            case ACTION_SELECT:
//                if (call_flow_window->selected == -1) {
//                    call_flow_window->selected = call_flow_window->cur_idx;
//                } else {
//                    if (call_flow_window->selected == (gint) call_flow_window->cur_idx) {
//                        call_flow_window->selected = -1;
//                    } else {
//                        // Show diff panel
//                        next_app_window = tui_create_app_window(SNG_WINDOW_TYPE_MSG_DIFF);
//                        msg_diff_win_set_msgs(next_app_window,
//                                              call_flow_arrow_message(g_ptr_array_index(call_flow_window->darrows, call_flow_window->selected)),
//                                              call_flow_arrow_message(g_ptr_array_index(call_flow_window->darrows, call_flow_window->cur_idx)));
//                    }
//                }
//                break;
//            case ACTION_CONFIRM:
//                // KEY_ENTER, display current message in raw mode
//                next_app_window = tui_create_app_window(SNG_WINDOW_TYPE_CALL_RAW);
//                call_raw_win_set_group(next_app_window, call_flow_window->group);
//                call_raw_win_set_msg(next_app_window,
//                                     call_flow_arrow_message(g_ptr_array_index(call_flow_window->darrows, call_flow_window->cur_idx)));
//                break;
            case ACTION_CLEAR_CALLS:
            case ACTION_CLEAR_CALLS_SOFT:
                // Propagate the key to the previous panel
                return;
            case ACTION_CLOSE:
                sng_widget_destroy(widget);
                return;
            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }
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
call_flow_win_help(G_GNUC_UNUSED SngAppWindow *window)
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
call_flow_win_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(call_flow_win_parent_class)->constructed(object);

    // Get parent window information
    CallFlowWindow *call_flow_win = TUI_CALL_FLOW(object);

    // Create menu bar entries
    call_flow_win->menu_bar = sng_menu_bar_new();

    // Create Arrow Flow viewer widget
    SngWidget *box_content = sng_box_new(SNG_ORIENTATION_HORIZONTAL);
    call_flow_win->flow_viewer = sng_flow_viewer_new();
    sng_container_add(SNG_CONTAINER(box_content), call_flow_win->flow_viewer);
    sng_box_pack_start(SNG_BOX(box_content), sng_separator_new(SNG_ORIENTATION_VERTICAL));

    // Create detail Text area
    call_flow_win->box_detail = sng_widget_new();
    sng_box_pack_start(SNG_BOX(box_content), call_flow_win->box_detail);

    // Add content box to window
    sng_container_add(SNG_CONTAINER(call_flow_win), box_content);

    // Bottom button bar
    SngWidget *button_bar = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 3, 0);
    sng_widget_set_vexpand(button_bar, FALSE);
    sng_widget_set_height(button_bar, 1);
    sng_box_set_background(SNG_BOX(button_bar), COLOR_PAIR(CP_WHITE_ON_CYAN));

    // Button Quit
    g_autoptr(GString) bn_text = g_string_new(NULL);
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_CLOSE),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Quit");
    SngWidget *bn_quit = sng_button_new(bn_text->str);

    // Button Select
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SELECT),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Select");
    SngWidget *bn_select = sng_button_new(bn_text->str);

    // Button Help
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_HELP),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Help");
    SngWidget *bn_help = sng_button_new(bn_text->str);

    // Button Help
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_DISP_FILTER),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Search");
    SngWidget *bn_search = sng_button_new(bn_text->str);

    // Button Extended
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_FLOW_EX),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Extended");
    SngWidget *bn_extended = sng_button_new(bn_text->str);

    // Button Clear
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_CLEAR_CALLS),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Clear");
    SngWidget *bn_clear = sng_button_new(bn_text->str);

    // Button Filter
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_FILTERS),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Filter");
    SngWidget *bn_filter = sng_button_new(bn_text->str);

    // Button Settings
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_SETTINGS),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Settings");
    SngWidget *bn_settings = sng_button_new(bn_text->str);

    // Button Columns
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_COLUMNS),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Columns");
    SngWidget *bn_columns = sng_button_new(bn_text->str);

    sng_box_pack_start(SNG_BOX(button_bar), bn_quit);
    sng_box_pack_start(SNG_BOX(button_bar), bn_select);
    sng_box_pack_start(SNG_BOX(button_bar), bn_help);
    sng_box_pack_start(SNG_BOX(button_bar), bn_search);
    sng_box_pack_start(SNG_BOX(button_bar), bn_extended);
    sng_box_pack_start(SNG_BOX(button_bar), bn_clear);
    sng_box_pack_start(SNG_BOX(button_bar), bn_filter);
    sng_box_pack_start(SNG_BOX(button_bar), bn_settings);
    sng_box_pack_start(SNG_BOX(button_bar), bn_columns);
    sng_container_add(SNG_CONTAINER(call_flow_win), button_bar);


    // Set default widget
    sng_window_set_default_focus(SNG_WINDOW(call_flow_win), call_flow_win->flow_viewer);

    // Chain up parent constructor
    G_OBJECT_CLASS(call_flow_win_parent_class)->constructed(object);
}

static void
call_flow_win_class_init(CallFlowWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = call_flow_win_constructed;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->size_request = call_flow_win_size_request;
    widget_class->key_pressed = call_flow_win_handle_key;

    SngAppWindowClass *app_window_class = SNG_APP_WINDOW_CLASS(klass);
    app_window_class->help = call_flow_win_help;
}

static void
call_flow_win_init(G_GNUC_UNUSED CallFlowWindow *call_flow_window)
{
}
