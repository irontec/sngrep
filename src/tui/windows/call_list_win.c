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
 * @file call_list_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to handle Call List window
 *
 */
#include "config.h"
#include <glib.h>
#include <stdio.h>
#include "glib-extra/glib.h"
#include "setting.h"
#include "storage/filter.h"
#ifdef USE_HEP
#include "capture/capture_hep.h"
#endif
#include "tui/tui.h"
#include "tui/dialog.h"
#include "tui/windows/call_list_win.h"
#include "tui/windows/call_flow_win.h"
#include "tui/windows/call_raw_win.h"
#include "tui/windows/save_win.h"

G_DEFINE_TYPE(CallListWindow, call_list_win, SNG_TYPE_APP_WINDOW)

/**
 * @brief Determine if the screen requires redrawn
 *
 * This will query the interface if it requires to be redraw again.
 *
 * @param window UI structure pointer
 * @return true if the panel requires redraw, false otherwise
 */
static gboolean
call_list_win_redraw(G_GNUC_UNUSED SngAppWindow *window)
{
    return storage_calls_changed();
}

static void
call_list_win_show_save_win(CallListWindow *call_list_win)
{
    // Create a new Save window
    SngWindow *save_win = save_win_new();

    // Set selected calls
    CallGroup *group = sng_table_get_call_group(SNG_TABLE(call_list_win->tb_calls));
    if (group != NULL) {
        save_set_group(TUI_SAVE(save_win), group);
    }

    // Make window visible
    sng_widget_show(SNG_WIDGET(save_win));
}

static void
call_list_win_handle_action(SngWidget *sender, KeybindingAction action)
{
    CallListWindow *call_list_win = SNG_CALL_LIST_WIN(sng_widget_get_toplevel(sender));
    CallGroup *group = NULL;
    Call *call = NULL;
    SngWidget *exit_dialog = NULL;

    // Check if we handle this action
    switch (action) {
        case ACTION_DISP_FILTER:
            sng_widget_grab_focus(call_list_win->en_dfilter);
            break;
        case ACTION_SHOW_FLOW:
        case ACTION_SHOW_FLOW_EX:
        case ACTION_SHOW_RAW:
            // Create a new group of calls
            group = call_group_clone(sng_table_get_call_group(SNG_TABLE(call_list_win->tb_calls)));

            // If no call is selected, use current call
            if (call_group_count(group) == 0) {
                call = sng_table_get_current(SNG_TABLE(call_list_win->tb_calls));
                if (call != NULL) {
                    call_group_add(group, call);
                }
            }

            // No calls to display
            if (call_group_count(group) == 0) {
                break;
            }

            // Add xcall to the group
            if (action == ACTION_SHOW_FLOW_EX) {
                call = sng_table_get_current(SNG_TABLE(call_list_win->tb_calls));
                call_group_add_calls(group, call->xcalls);
                group->callid = call->callid;
            }

            if (action == ACTION_SHOW_RAW) {
                // Create a Call raw panel
                call_raw_win_set_group(tui_create_app_window(SNG_WINDOW_TYPE_CALL_RAW), group);
            } else {
                // Display current call flow (normal or extended)
                call_flow_win_set_group(tui_create_app_window(SNG_WINDOW_TYPE_CALL_FLOW), group);
            }
            break;
        case ACTION_SHOW_PROTOCOLS:
            tui_create_app_window(SNG_WINDOW_TYPE_PROTOCOL_SELECT);
            break;
        case ACTION_SHOW_FILTERS:
            tui_create_app_window(SNG_WINDOW_TYPE_FILTER);
            break;
        case ACTION_SHOW_COLUMNS:
            tui_create_app_window(SNG_WINDOW_TYPE_COLUMN_SELECT);
            sng_table_columns_update(SNG_TABLE(call_list_win->tb_calls));
            break;
        case ACTION_SHOW_STATS:
            tui_create_app_window(SNG_WINDOW_TYPE_STATS);
            break;
        case ACTION_SAVE:
            call_list_win_show_save_win(call_list_win);
            break;
        case ACTION_SHOW_SETTINGS:
            tui_create_app_window(SNG_WINDOW_TYPE_SETTINGS);
            break;
        case ACTION_TOGGLE_PAUSE:
            // Pause/Resume capture
            capture_manager_get_instance()->paused = !capture_manager_get_instance()->paused;
            break;
        case ACTION_SHOW_HELP:
            sng_app_window_help(SNG_APP_WINDOW(call_list_win));
            break;
        case ACTION_PREV_SCREEN:
            // Handle quit from this screen unless requested
            if (setting_enabled(SETTING_TUI_EXITPROMPT)) {
                exit_dialog = sng_dialog_new(
                    SNG_DIALOG_QUESTION,
                    SNG_BUTTONS_YES_NO,
                    "Confirm exit",
                    "<cyan> Are you sure you want to quit?"
                );
                if (sng_dialog_run(SNG_DIALOG(exit_dialog)) == SNG_RESPONSE_YES) {
                    sng_widget_destroy(SNG_WIDGET(call_list_win));
                }
                sng_widget_destroy(exit_dialog);
            } else {
                sng_widget_destroy(SNG_WIDGET(call_list_win));
            }
        default:
            break;
    }
}

/**
 * @brief Handle Call list key strokes
 *
 * This function will manage the custom keybindings of the panel.
 * This function return one of the values defined in @key_handler_ret
 *
 * @param window UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
static void
call_list_win_handle_key(SngWidget *widget, int key)
{
    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DISP_FILTER:
            case ACTION_SHOW_FLOW:
            case ACTION_SHOW_FLOW_EX:
            case ACTION_SHOW_RAW:
            case ACTION_SHOW_PROTOCOLS:
            case ACTION_SHOW_FILTERS:
            case ACTION_SHOW_COLUMNS:
            case ACTION_SHOW_STATS:
            case ACTION_SAVE:
            case ACTION_SHOW_SETTINGS:
            case ACTION_TOGGLE_PAUSE:
            case ACTION_SHOW_HELP:
            case ACTION_PREV_SCREEN:
                // This panel has handled the key successfully
                call_list_win_handle_action(widget, action);
                return;
            default:
                continue;
        }
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
static int
call_list_win_help(G_GNUC_UNUSED SngAppWindow *window)
{
    WINDOW *help_win;
    int height, width;

    // Create a new panel and show centered
    height = 28;
    width = 65;
    help_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);

    // Set the window title
    mvwprintw(help_win, 1, 25, "Call List Help");

    // Write border and boxes around the window
    wattron(help_win, COLOR_PAIR(CP_BLUE_ON_DEF));
    box(help_win, 0, 0);
    mvwhline(help_win, 2, 1, ACS_HLINE, width - 2);
    mvwhline(help_win, 7, 1, ACS_HLINE, width - 2);
    mvwhline(help_win, height - 3, 1, ACS_HLINE, width - 2);
    mvwaddch(help_win, 2, 0, ACS_LTEE);
    mvwaddch(help_win, 7, 0, ACS_LTEE);
    mvwaddch(help_win, height - 3, 0, ACS_LTEE);
    mvwaddch(help_win, 2, 64, ACS_RTEE);
    mvwaddch(help_win, 7, 64, ACS_RTEE);
    mvwaddch(help_win, height - 3, 64, ACS_RTEE);

    // Set the window footer (nice blue?)
    mvwprintw(help_win, height - 2, 20, "Press any key to continue");

    // Some brief explanation about what window shows
    wattron(help_win, COLOR_PAIR(CP_CYAN_ON_DEF));
    mvwprintw(help_win, 3, 2, "This windows show the list of parsed calls from a pcap file ");
    mvwprintw(help_win, 4, 2, "(Offline) or a live capture with libpcap functions (Online).");
    mvwprintw(help_win, 5, 2, "You can configure the columns shown in this screen and some");
    mvwprintw(help_win, 6, 2, "static filters using sngreprc resource file.");
    wattroff(help_win, COLOR_PAIR(CP_CYAN_ON_DEF));

    // A list of available keys in this window
    mvwprintw(help_win, 8, 2, "Available keys:");
    mvwprintw(help_win, 10, 2, "Esc/Q       Exit sngrep.");
    mvwprintw(help_win, 11, 2, "Enter       Show selected calls message flow");
    mvwprintw(help_win, 12, 2, "Space       Select call");
    mvwprintw(help_win, 13, 2, "F1/h        Show this screen");
    mvwprintw(help_win, 14, 2, "F2/S        Save captured packages to a file");
    mvwprintw(help_win, 15, 2, "F3//        Display filtering (match string case insensitive)");
    mvwprintw(help_win, 16, 2, "F4/X        Show selected call-flow (Extended) if available");
    mvwprintw(help_win, 17, 2, "F5/Ctrl-L   Clear call list (can not be undone!)");
    mvwprintw(help_win, 18, 2, "F6/R        Show selected call messages in raw mode");
    mvwprintw(help_win, 19, 2, "F7/F        Show filter options");
    mvwprintw(help_win, 20, 2, "F8/o        Show Settings");
    mvwprintw(help_win, 21, 2, "F10/t       Select displayed columns");
    mvwprintw(help_win, 22, 2, "i/I         Set display filter to invite");
    mvwprintw(help_win, 23, 2, "p           Stop/Resume packet capture");

    // Press any key to close
    wgetch(help_win);
    delwin(help_win);

    return 0;
}


static void
call_list_win_mode_label(SngWidget *widget)
{
    CaptureManager *capture = capture_manager_get_instance();
    gboolean online = capture_is_online(capture);
    const gchar *device = capture_input_pcap_device(capture);

    g_autoptr(GString) mode = g_string_new("Mode: ");
    g_string_append(mode, online ? "<green>" : "<red>");
    g_string_append(mode, capture_status_desc(capture));

    if (!online) {
        guint progress = capture_manager_load_progress(capture);
        if (progress > 0 && progress < 100) {
            g_string_append_printf(mode, "[%d%%]", progress);
        }
    }

    // Get online mode capture device
    if (device != NULL)
        g_string_append_printf(mode, "[%s]", device);

#ifdef USE_HEP
    const char *eep_port;
    if ((eep_port = capture_output_hep_port(capture))) {
        g_string_append_printf(mode, "[H:%s]", eep_port);
    }
    if ((eep_port = capture_input_hep_port(capture))) {
        g_string_append_printf(mode, "[L:%s]", eep_port);
    }
#endif

    // Set Mode label text
    sng_label_set_text(SNG_LABEL(widget), mode->str);
}

static void
call_list_win_dialog_label(SngWidget *widget)
{
    g_autoptr(GString) count = g_string_new(NULL);
    // Print Dialogs or Calls in label depending on calls filter
    StorageMatchOpts storageMatchOpts = storage_match_options();
    g_string_append(count, storageMatchOpts.invite ? "Calls: " : "Dialogs: ");
    // Print calls count (also filtered)
    StorageStats stats = storage_calls_stats();
    if (stats.total != stats.displayed) {
        g_string_append_printf(count, "%d / %d", stats.displayed, stats.total);
    } else {
        g_string_append_printf(count, "%d", stats.total);
    }
    // Set Count label text
    sng_label_set_text(SNG_LABEL(widget), count->str);
}

static void
call_list_win_memory_label(SngWidget *widget)
{
    g_autoptr(GString) memory = g_string_new(NULL);
    if (storage_memory_limit() > 0) {
        g_autofree const gchar *usage = g_format_size_full(
            storage_memory_usage(),
            G_FORMAT_SIZE_IEC_UNITS
        );
        g_autofree const gchar *limit = g_format_size_full(
            storage_memory_limit(),
            G_FORMAT_SIZE_IEC_UNITS
        );
        g_string_append_printf(memory, "Mem: %s / %s", usage, limit);
        sng_label_set_text(SNG_LABEL(widget), memory->str);
    }
}

static void
call_list_win_display_filter(SngWidget *widget)
{
    const gchar *text = sng_entry_get_text(SNG_ENTRY(widget));
    // Reset filters on each key stroke
    filter_reset_calls();
    filter_set(FILTER_CALL_LIST, strlen(text) ? text : NULL);
}

SngAppWindow *
call_list_win_new()
{
    SngAppWindow *window = g_object_new(
        SNG_TYPE_CALL_LIST_WIN,
        "window-type", SNG_WINDOW_TYPE_CALL_LIST,
        "height", getmaxy(stdscr),
        "width", getmaxx(stdscr),
        NULL
    );
    return window;
}

SngTable *
call_list_win_get_table(SngAppWindow *window)
{
    CallListWindow *call_list_win = SNG_CALL_LIST_WIN(window);
    return SNG_TABLE(call_list_win->tb_calls);
}

static void
call_list_win_constructed(GObject *object)
{
    CallListWindow *call_list_win = SNG_CALL_LIST_WIN(object);

    // Create menu bar entries
    call_list_win->menu_bar = sng_menu_bar_new();

    // File Menu
    SngWidget *menu_file = sng_menu_new("File");
    SngWidget *menu_file_preferences = sng_menu_item_new("Settings");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_file_preferences), ACTION_SHOW_SETTINGS);
    g_signal_connect(menu_file_preferences, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_SETTINGS));

    SngWidget *menu_file_save = sng_menu_item_new("Save as ...");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_file_save), ACTION_SAVE);
    g_signal_connect(menu_file_save, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SAVE));

    SngWidget *menu_file_exit = sng_menu_item_new("Exit");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_file_exit), ACTION_PREV_SCREEN);
    g_signal_connect(menu_file_exit, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_PREV_SCREEN));

    // View Menu
    SngWidget *menu_view = sng_menu_new("View");
    SngWidget *menu_view_filters = sng_menu_item_new("Filters");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_view_filters), ACTION_SHOW_FILTERS);
    g_signal_connect(menu_view_filters, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_FILTERS));

    SngWidget *menu_view_protocols = sng_menu_item_new("Protocols");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_view_protocols), ACTION_SHOW_PROTOCOLS);
    g_signal_connect(menu_view_protocols, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_PROTOCOLS));

    // Call List menu
    SngWidget *menu_list = sng_menu_new("Call List");
    SngWidget *menu_list_columns = sng_menu_item_new("Configure Columns");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_list_columns), ACTION_SHOW_COLUMNS);
    g_signal_connect(menu_list_columns, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_COLUMNS));

    SngWidget *menu_list_clear = sng_menu_item_new("Clear List");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_list_clear), ACTION_CLEAR_CALLS);
    g_signal_connect(menu_list_clear, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_CLEAR_CALLS));

    SngWidget *menu_list_clear_soft = sng_menu_item_new("Clear filtered calls");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_list_clear_soft), ACTION_CLEAR_CALLS_SOFT);
    g_signal_connect(menu_list_clear_soft, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_CLEAR_CALLS_SOFT));

    SngWidget *menu_list_flow = sng_menu_item_new("Show Call Flow");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_list_flow), ACTION_SHOW_FLOW);
    g_signal_connect(menu_list_flow, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_FLOW));

    SngWidget *menu_list_flow_ex = sng_menu_item_new("Show Call Flow Extended");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_list_flow_ex), ACTION_SHOW_FLOW_EX);
    g_signal_connect(menu_list_flow, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_FLOW_EX));

    // Help Menu
    SngWidget *menu_help = sng_menu_new("Help");
    SngWidget *menu_help_about = sng_menu_item_new("About");
    sng_menu_item_set_action(SNG_MENU_ITEM(menu_help_about), ACTION_SHOW_HELP);
    g_signal_connect(menu_help_about, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_HELP));


    // Add menubar menus and items
    sng_container_add(SNG_CONTAINER(call_list_win->menu_bar), menu_file);
    sng_container_add(SNG_CONTAINER(menu_file), menu_file_preferences);
    sng_container_add(SNG_CONTAINER(menu_file), menu_file_save);
    sng_container_add(SNG_CONTAINER(menu_file), sng_menu_item_new(NULL));
    sng_container_add(SNG_CONTAINER(menu_file), menu_file_exit);
    sng_container_add(SNG_CONTAINER(call_list_win->menu_bar), menu_view);
    sng_container_add(SNG_CONTAINER(menu_view), menu_view_filters);
    sng_container_add(SNG_CONTAINER(menu_view), menu_view_protocols);
    sng_container_add(SNG_CONTAINER(call_list_win->menu_bar), menu_list);
    sng_container_add(SNG_CONTAINER(menu_list), menu_list_columns);
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_item_new(NULL));
    sng_container_add(SNG_CONTAINER(menu_list), menu_list_clear);
    sng_container_add(SNG_CONTAINER(menu_list), menu_list_clear_soft);
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_item_new(NULL));
    sng_container_add(SNG_CONTAINER(menu_list), menu_list_flow);
    sng_container_add(SNG_CONTAINER(menu_list), menu_list_flow_ex);
    sng_container_add(SNG_CONTAINER(call_list_win->menu_bar), menu_help);
    sng_container_add(SNG_CONTAINER(menu_help), menu_help_about);
    sng_container_add(SNG_CONTAINER(call_list_win), call_list_win->menu_bar);

    // First header line
    SngWidget *header_first = sng_box_new_full(BOX_ORIENTATION_HORIZONTAL, 8, 1);
    sng_box_pack_start(SNG_BOX(call_list_win), header_first);

    // Mode Label
    SngWidget *lb_mode = sng_label_new(NULL);
    g_signal_connect(lb_mode, "draw", G_CALLBACK(call_list_win_mode_label), NULL);
    sng_container_add(SNG_CONTAINER(header_first), lb_mode);

    // Dialog Count
    SngWidget *lb_dialog_cnt = sng_label_new(NULL);
    g_signal_connect(lb_dialog_cnt, "draw", G_CALLBACK(call_list_win_dialog_label), NULL);
    sng_container_add(SNG_CONTAINER(header_first), lb_dialog_cnt);

    // Memory usage
    if (storage_memory_limit() != 0) {
        SngWidget *lb_memory = sng_label_new(NULL);
        g_signal_connect(lb_memory, "draw", G_CALLBACK(call_list_win_memory_label), NULL);
        sng_container_add(SNG_CONTAINER(header_first), lb_memory);
    }

    // Print Open filename in Offline mode
    CaptureManager *capture = capture_manager_get_instance();
    const gchar *infile = capture_input_pcap_file(capture);
    if (infile != NULL) {
        g_autoptr(GString) file = g_string_new(NULL);
        g_string_append_printf(file, "Filename: %s", infile);
        sng_container_add(SNG_CONTAINER(header_first), sng_label_new(file->str));
    }

    // Show all labels in the line
    sng_container_show_all(SNG_CONTAINER(header_first));

    // Second header line
    SngWidget *header_second = sng_box_new_full(BOX_ORIENTATION_HORIZONTAL, 5, 1);
    sng_box_pack_start(SNG_BOX(call_list_win), header_second);

    // Show BPF filter if specified in command line
    const gchar *bpf_filter = capture_manager_filter(capture);
    if (bpf_filter != NULL) {
        g_autoptr(GString) filter = g_string_new("BPF Filter: ");
        g_string_append_printf(filter, "<yellow>%s", bpf_filter);
        sng_container_add(SNG_CONTAINER(header_second), sng_label_new(filter->str));
        sng_container_show_all(SNG_CONTAINER(header_second));
    }

    // Show Match expression label if specified in command line
    StorageMatchOpts match = storage_match_options();
    if (match.mexpr != NULL) {
        g_autoptr(GString) match_expression = g_string_new("Match Expression: ");
        g_string_append_printf(match_expression, "<yellow>%s", match.mexpr);
        sng_container_add(SNG_CONTAINER(header_second), sng_label_new(match_expression->str));
        sng_container_show_all(SNG_CONTAINER(header_second));
    }

    // Add Display filter label and entry
    SngWidget *header_third = sng_box_new_full(BOX_ORIENTATION_HORIZONTAL, 1, 1);
    sng_box_pack_start(SNG_BOX(call_list_win), header_third);

    SngWidget *lb_dfilter = sng_label_new("Display Filter:");
    sng_widget_set_hexpand(SNG_WIDGET(lb_dfilter), FALSE);
    sng_container_add(SNG_CONTAINER(header_third), lb_dfilter);
    call_list_win->en_dfilter = sng_entry_new(NULL);
    g_signal_connect(call_list_win->en_dfilter, "key-pressed",
                     G_CALLBACK(call_list_win_display_filter), NULL);
    sng_container_add(SNG_CONTAINER(header_third), call_list_win->en_dfilter);
    sng_container_show_all(SNG_CONTAINER(header_third));

    call_list_win->tb_calls = sng_table_new();
    sng_table_columns_update(SNG_TABLE(call_list_win->tb_calls));
    g_signal_connect(call_list_win->tb_calls, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_FLOW));

    sng_container_add(SNG_CONTAINER(call_list_win), call_list_win->tb_calls);

    // Bottom button bar
    SngWidget *button_bar = sng_box_new_full(BOX_ORIENTATION_HORIZONTAL, 3, 0);
    sng_widget_set_vexpand(button_bar, FALSE);
    sng_widget_set_height(button_bar, 1);
    sng_box_set_background(SNG_BOX(button_bar), COLOR_PAIR(CP_WHITE_ON_CYAN));

    // Button Quit
    g_autoptr(GString) bn_text = g_string_new(NULL);
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_PREV_SCREEN),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Quit");
    SngWidget *bn_quit = sng_button_new(bn_text->str);
    g_signal_connect(bn_quit, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_PREV_SCREEN));

    // Button Select
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SELECT),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Select");
    SngWidget *bn_select = sng_button_new(bn_text->str);
    g_signal_connect_swapped(bn_select, "activate",
                             G_CALLBACK(sng_table_select_current),
                             SNG_TABLE(call_list_win->tb_calls));

    // Button Help
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_HELP),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Help");
    SngWidget *bn_help = sng_button_new(bn_text->str);
    g_signal_connect(bn_help, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_HELP));

    // Button Help
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_DISP_FILTER),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Search");
    SngWidget *bn_search = sng_button_new(bn_text->str);
    g_signal_connect(bn_search, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_DISP_FILTER));

    // Button Extended
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_FLOW_EX),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Extended");
    SngWidget *bn_extended = sng_button_new(bn_text->str);
    g_signal_connect(bn_extended, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_FLOW_EX));

    // Button Clear
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_CLEAR_CALLS),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Clear");
    SngWidget *bn_clear = sng_button_new(bn_text->str);
    g_signal_connect_swapped(bn_clear, "activate",
                             G_CALLBACK(sng_table_clear),
                             SNG_TABLE(call_list_win->tb_calls));

    // Button Filter
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_FILTERS),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Filter");
    SngWidget *bn_filter = sng_button_new(bn_text->str);
    g_signal_connect(bn_filter, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_FILTERS));

    // Button Settings
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_SETTINGS),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Settings");
    SngWidget *bn_settings = sng_button_new(bn_text->str);
    g_signal_connect(bn_settings, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_SETTINGS));

    // Button Columns
    g_string_printf(bn_text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(ACTION_SHOW_COLUMNS),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), "Columns");
    SngWidget *bn_columns = sng_button_new(bn_text->str);
    g_signal_connect(bn_columns, "activate",
                     G_CALLBACK(call_list_win_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_COLUMNS));

    sng_box_pack_start(SNG_BOX(button_bar), bn_quit);
    sng_box_pack_start(SNG_BOX(button_bar), bn_select);
    sng_box_pack_start(SNG_BOX(button_bar), bn_help);
    sng_box_pack_start(SNG_BOX(button_bar), bn_search);
    sng_box_pack_start(SNG_BOX(button_bar), bn_extended);
    sng_box_pack_start(SNG_BOX(button_bar), bn_clear);
    sng_box_pack_start(SNG_BOX(button_bar), bn_filter);
    sng_box_pack_start(SNG_BOX(button_bar), bn_settings);
    sng_box_pack_start(SNG_BOX(button_bar), bn_columns);
    sng_container_add(SNG_CONTAINER(call_list_win), button_bar);
    sng_container_show_all(SNG_CONTAINER(button_bar));
    sng_container_show_all(SNG_CONTAINER(call_list_win));

    // Start with the call list focused
    sng_window_set_default_focus(SNG_WINDOW(call_list_win), call_list_win->tb_calls);

    // Chain-up parent constructed
    G_OBJECT_CLASS(call_list_win_parent_class)->constructed(object);
}

static void
call_list_win_class_init(CallListWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = call_list_win_constructed;

    SngAppWindowClass *app_window_class = SNG_APP_WINDOW_CLASS(klass);
    app_window_class->redraw = call_list_win_redraw;
    app_window_class->help = call_list_win_help;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->key_pressed = call_list_win_handle_key;
}

static void
call_list_win_init(G_GNUC_UNUSED CallListWindow *self)
{
}
