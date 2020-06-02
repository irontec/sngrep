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
#include "glib-extra/glib.h"
#include "setting.h"
#include "storage/filter.h"
#ifdef USE_HEP
#include "capture/capture_hep.h"
#endif
#include "tui/tui.h"
#include "tui/windows/call_list_win.h"
#include "tui/windows/call_flow_win.h"
#include "tui/windows/call_raw_win.h"
#include "tui/windows/save_win.h"

G_DEFINE_TYPE(CallListWindow, call_list_win, SNG_TYPE_APP_WINDOW)

SngWidget *
call_list_win_new()
{
    return g_object_new(
        SNG_TYPE_CALL_LIST_WIN,
        "height", getmaxy(stdscr),
        "width", getmaxx(stdscr),
        NULL
    );
}

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
        save_win_set_group(SNG_SAVE_WIN(save_win), group);
    }

    sng_window_update(save_win);
}

static gboolean
call_list_win_show_flow_win(CallListWindow *call_list_win, SngAction action)
{
    // Create a new group of calls
    SngTable *table = SNG_TABLE(call_list_win->tb_calls);
    CallGroup *group = call_group_clone(sng_table_get_call_group(table));

    // If no call is selected, use current call
    if (call_group_count(group) == 0) {
        Call *call = sng_table_get_current(table);
        if (call != NULL) {
            call_group_add(group, call);
        }
    }

    // No calls to display
    if (call_group_count(group) == 0) {
        return FALSE;
    }

    // Add xcall to the group
    if (action == ACTION_SHOW_FLOW_EX) {
        Call *call = sng_table_get_current(table);
        call_group_add_calls(group, call->xcalls);
        group->callid = call->callid;
    }

    // Create a new Call Flow window
    SngAppWindow *flow_win = call_flow_win_new();
    call_flow_win_set_group(SNG_CALL_FLOW(flow_win), group);
    sng_window_update(SNG_WINDOW(flow_win));

    return TRUE;
}

static gboolean
call_list_win_show_filters_win(CallListWindow *call_list_win)
{
    tui_create_app_window(SNG_WINDOW_TYPE_FILTER);
    return TRUE;
}

static gboolean
call_list_win_show_raw_win(CallListWindow *call_list_win)
{
    return TRUE;
}

static gboolean
call_list_win_show_protocols_win(CallListWindow *call_list_win)
{
    tui_create_app_window(SNG_WINDOW_TYPE_PROTOCOL_SELECT);
    return TRUE;
}

static gboolean
call_list_win_show_columns_win(CallListWindow *call_list_win)
{
    tui_create_app_window(SNG_WINDOW_TYPE_COLUMN_SELECT);
    sng_table_columns_update(SNG_TABLE(call_list_win->tb_calls));
    return TRUE;
}

static gboolean
call_list_win_show_stats_win(CallListWindow *call_list_win)
{
    tui_create_app_window(SNG_WINDOW_TYPE_STATS);
    return TRUE;
}

static gboolean
call_list_win_show_settings_win(CallListWindow *call_list_win)
{
    tui_create_app_window(SNG_WINDOW_TYPE_SETTINGS);
    return TRUE;
}

static gboolean
call_list_win_confirm_exit(SngWidget *widget, G_GNUC_UNUSED SngAction action)
{
    SngWidget *exit_dialog = NULL;

    // Handle quit from this screen unless requested
    if (setting_enabled(SETTING_TUI_EXITPROMPT)) {
        exit_dialog = sng_dialog_new(
            SNG_DIALOG_QUESTION,
            SNG_BUTTONS_YES_NO,
            "Confirm exit",
            "<cyan> Are you sure you want to quit?"
        );
        if (sng_dialog_run(SNG_DIALOG(exit_dialog)) == SNG_RESPONSE_YES) {
            sng_widget_destroy(widget);
        }
        sng_widget_destroy(exit_dialog);
    } else {
        sng_widget_destroy(widget);
    }

    return TRUE;
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
    const gchar *eep_port = NULL;
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

static void
call_list_win_display_filter(SngWidget *widget)
{
    const gchar *text = sng_entry_get_text(SNG_ENTRY(widget));
    // Reset filters on each key stroke
    filter_reset_calls();
    filter_set(FILTER_CALL_LIST, strlen(text) ? text : NULL);
}

SngTable *
call_list_win_get_table(SngAppWindow *window)
{
    CallListWindow *call_list_win = SNG_CALL_LIST_WIN(window);
    return SNG_TABLE(call_list_win->tb_calls);
}

static void
call_list_win_setup_menu(CallListWindow *call_list_win)
{
    // File Menu
    SngWidget *menu_file = sng_menu_new("File");
    sng_container_add(SNG_CONTAINER(menu_file), sng_menu_item_new("Settings", ACTION_SHOW_SETTINGS));
    sng_container_add(SNG_CONTAINER(menu_file), sng_menu_item_new("Save as ...", ACTION_SAVE));
    sng_container_add(SNG_CONTAINER(menu_file), sng_menu_separator());
    sng_container_add(SNG_CONTAINER(menu_file), sng_menu_item_new("Exit", ACTION_CLOSE));
    sng_app_window_add_menu(SNG_APP_WINDOW(call_list_win), SNG_MENU(menu_file));

    // View Menu
    SngWidget *menu_view = sng_menu_new("View");
    sng_container_add(SNG_CONTAINER(menu_view), sng_menu_item_new("Filters", ACTION_SHOW_FILTERS));
    sng_container_add(SNG_CONTAINER(menu_view), sng_menu_item_new("Protocols", ACTION_SHOW_PROTOCOLS));
    sng_app_window_add_menu(SNG_APP_WINDOW(call_list_win), SNG_MENU(menu_view));

    // Call List menu
    SngWidget *menu_list = sng_menu_new("Call List");
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_item_new("Configure Columns", ACTION_SHOW_COLUMNS));
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_separator());
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_item_new("Clear List", ACTION_CLEAR_CALLS));
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_item_new("Clear filtered calls", ACTION_CLEAR_CALLS_SOFT));
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_separator());
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_item_new("Show Call Flow", ACTION_SHOW_FLOW));
    sng_container_add(SNG_CONTAINER(menu_list), sng_menu_item_new("Show Call Flow Extended", ACTION_SHOW_FLOW_EX));
    sng_app_window_add_menu(SNG_APP_WINDOW(call_list_win), SNG_MENU(menu_list));

    // Help Menu
    SngWidget *menu_help = sng_menu_new("Help");
    sng_container_add(SNG_CONTAINER(menu_help), sng_menu_item_new("About", ACTION_SHOW_HELP));
    sng_app_window_add_menu(SNG_APP_WINDOW(call_list_win), SNG_MENU(menu_help));
}

static void
call_list_win_button_bar(CallListWindow *call_list_win)
{
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Quit", ACTION_CLOSE);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Show", ACTION_SHOW_FLOW);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Select", ACTION_SELECT);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Help", ACTION_SHOW_HELP);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Save", ACTION_SAVE);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Search", ACTION_DISP_FILTER);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Extended", ACTION_SHOW_FLOW_EX);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Clear", ACTION_CLEAR_CALLS);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Filter", ACTION_SHOW_FILTERS);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Settings", ACTION_SHOW_SETTINGS);
    sng_app_window_add_button(SNG_APP_WINDOW(call_list_win), "Columns", ACTION_SHOW_COLUMNS);
}

static void
call_list_win_constructed(GObject *object)
{
    CallListWindow *call_list_win = SNG_CALL_LIST_WIN(object);
    SngWidget *widget = SNG_WIDGET(call_list_win);

    // Setup Menu Bar
    call_list_win_setup_menu(call_list_win);

    // Setup Button bar
    call_list_win_button_bar(call_list_win);

    // Set main window contents
    SngWidget *content = sng_app_window_get_content(SNG_APP_WINDOW(call_list_win));
    sng_box_set_orientation(SNG_BOX(content), SNG_ORIENTATION_VERTICAL);

    // First header line
    SngWidget *header_first = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 8, 1);

    // Mode Label
    SngWidget *lb_mode = sng_label_new(NULL);
    g_signal_connect(lb_mode, "update", G_CALLBACK(call_list_win_mode_label), NULL);
    sng_container_add(SNG_CONTAINER(header_first), lb_mode);

    // Dialog Count
    SngWidget *lb_dialog_cnt = sng_label_new(NULL);
    g_signal_connect(lb_dialog_cnt, "update", G_CALLBACK(call_list_win_dialog_label), NULL);
    sng_container_add(SNG_CONTAINER(header_first), lb_dialog_cnt);

    // Memory usage
    if (storage_memory_limit() != 0) {
        SngWidget *lb_memory = sng_label_new(NULL);
        g_signal_connect(lb_memory, "update", G_CALLBACK(call_list_win_memory_label), NULL);
        sng_container_add(SNG_CONTAINER(header_first), lb_memory);
    }

    // Print Open filename in Offline mode
    CaptureManager *capture = capture_manager_get_instance();
    const gchar *infile = capture_input_pcap_file(capture);
    if (infile != NULL) {
        sng_container_add(SNG_CONTAINER(header_first), sng_label_new("Filename: %s", infile));
    }

    // Second header line
    SngWidget *header_second = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 5, 1);

    // Show BPF filter if specified in command line
    const gchar *bpf_filter = capture_manager_filter(capture);
    if (bpf_filter != NULL) {
        sng_container_add(SNG_CONTAINER(header_second), sng_label_new("BPF Filter: <yellow>%s", bpf_filter));
    }

    // Show Match expression label if specified in command line
    StorageMatchOpts match = storage_match_options();
    if (match.mexpr != NULL) {
        sng_container_add(SNG_CONTAINER(header_second), sng_label_new("Match Expression: <yellow>%s", match.mexpr));
    }

    // Add Display filter label and entry
    SngWidget *header_third = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 1, 1);
    sng_box_pack_start(SNG_BOX(header_third), sng_label_new("Display Filter:"));
    call_list_win->en_dfilter = sng_entry_new(NULL);
    sng_container_add(SNG_CONTAINER(header_third), call_list_win->en_dfilter);
    g_signal_connect(call_list_win->en_dfilter, "key-pressed",
                     G_CALLBACK(call_list_win_display_filter), NULL);

    call_list_win->tb_calls = sng_table_new();
    sng_table_columns_update(SNG_TABLE(call_list_win->tb_calls));
    g_signal_connect(call_list_win->tb_calls, "activate",
                     G_CALLBACK(sng_window_handle_action),
                     GINT_TO_POINTER(ACTION_SHOW_FLOW));

    sng_box_pack_start(SNG_BOX(content), header_first);
    sng_box_pack_start(SNG_BOX(content), header_second);
    sng_box_pack_start(SNG_BOX(content), header_third);
    sng_container_add(SNG_CONTAINER(content), call_list_win->tb_calls);

    // Bind widget actions
    sng_widget_bind_action(widget, ACTION_CLOSE,
                           G_CALLBACK(call_list_win_confirm_exit), NULL);
    sng_widget_bind_action(widget, ACTION_SHOW_FLOW,
                           G_CALLBACK(call_list_win_show_flow_win), NULL);
    sng_widget_bind_action(widget, ACTION_SHOW_FLOW_EX,
                           G_CALLBACK(call_list_win_show_flow_win), NULL);
    sng_widget_bind_action(widget, ACTION_SHOW_RAW,
                           G_CALLBACK(call_list_win_show_raw_win), NULL);
    sng_widget_bind_action(widget, ACTION_SHOW_PROTOCOLS,
                           G_CALLBACK(call_list_win_show_protocols_win), NULL);
    sng_widget_bind_action(widget, ACTION_SHOW_COLUMNS,
                           G_CALLBACK(call_list_win_show_columns_win), NULL);
    sng_widget_bind_action(widget, ACTION_SHOW_STATS,
                           G_CALLBACK(call_list_win_show_stats_win), NULL);
    sng_widget_bind_action(widget, ACTION_SHOW_SETTINGS,
                           G_CALLBACK(call_list_win_show_settings_win), NULL);
    sng_widget_bind_action(widget, ACTION_SHOW_FILTERS,
                           G_CALLBACK(call_list_win_show_filters_win), NULL);
    sng_widget_bind_action(widget, ACTION_SAVE,
                           G_CALLBACK(call_list_win_show_save_win), NULL);
    sng_widget_bind_action_swapped(widget, ACTION_TOGGLE_PAUSE,
                                   G_CALLBACK(capture_manager_toggle_pause), capture);
    sng_widget_bind_action_swapped(widget, ACTION_DISP_FILTER,
                                   G_CALLBACK(sng_widget_grab_focus), call_list_win->en_dfilter);
    sng_widget_bind_action_swapped(widget, ACTION_CLEAR_CALLS,
                                   G_CALLBACK(sng_table_clear), call_list_win->tb_calls);

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
}

static void
call_list_win_init(G_GNUC_UNUSED CallListWindow *self)
{
}
