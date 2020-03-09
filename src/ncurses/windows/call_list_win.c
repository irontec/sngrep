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
#include "ncurses/manager.h"
#include "ncurses/window.h"
#include "ncurses/dialog.h"
#include "ncurses/windows/call_list_win.h"
#include "ncurses/windows/call_flow_win.h"
#include "ncurses/windows/call_raw_win.h"
#include "ncurses/windows/save_win.h"
#include "ncurses/windows/column_select_win.h"

G_DEFINE_TYPE(CallListWindow, call_list, NCURSES_TYPE_WINDOW)

/**
 * @brief Move selected cursor to given line
 *
 * This function will move the cursor to given line, taking into account
 * selected line and scrolling position.
 *
 * @param window UI structure pointer
 * @param line Position to move the cursor
 */
static void
call_list_move(CallListWindow *self, gint line)
{
    // Already in this position?
    if (self->cur_idx == line)
        return;

    // Moving down or up?
    gboolean move_down = (self->cur_idx < line);

    if (move_down) {
        gint listh = getmaxy(self->list_win);
        // Remove one line of header
        listh--;

        if (scrollbar_visible(self->hscroll))
            listh--;

        while (self->cur_idx < line) {
            // Check if there is a call below us
            if (self->cur_idx == (gint) g_ptr_array_len(self->dcalls) - 1)
                break;

            // Increase current call position
            self->cur_idx++;

            // If we are out of the bottom of the displayed list
            // refresh it starting in the next call
            if ((gint) (self->cur_idx - self->vscroll.pos) == listh) {
                self->vscroll.pos++;
            }
        }
    } else {
        while (self->cur_idx > line) {
            // Check if there is a call above us
            if (self->cur_idx == 0)
                break;

            // If we are out of the top of the displayed list
            // refresh it starting in the previous (in fact current) call
            if (self->cur_idx == self->vscroll.pos) {
                self->vscroll.pos--;
            }
            // Move current call position
            self->cur_idx--;
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
call_list_move_up(CallListWindow *self, guint times)
{
    gint newpos = self->cur_idx - times;
    if (newpos < 0) newpos = 0;

    call_list_move(self, (guint) newpos);
}

/**
 * @brief Move column position right N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move right
 */
static void
call_list_move_right(CallListWindow *self, guint times)
{
    // Nothing to scroll
    if (!scrollbar_visible(self->hscroll))
        return;

    gint listw = getmaxx(self->hscroll.win);

    gint newpos = self->hscroll.pos + times;
    if (newpos >= self->hscroll.max - listw)
        newpos = self->hscroll.max - listw;

    self->hscroll.pos = newpos;
}

/**
 * @brief Move column position left N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move left
 */
static void
call_list_move_left(CallListWindow *self, guint times)
{
    gint newpos = self->hscroll.pos - times;
    if (newpos < 0)
        newpos = 0;

    self->hscroll.pos = (guint) newpos;
}

/**
 * @brief Move selection cursor down N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move down
 */
static void
call_list_move_down(CallListWindow *self, guint times)
{
    guint newpos = self->cur_idx + times;
    if (newpos >= g_ptr_array_len(self->dcalls))
        newpos = g_ptr_array_len(self->dcalls) - 1;

    call_list_move(self, newpos);
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
call_list_redraw(G_GNUC_UNUSED Window *window)
{
    return storage_calls_changed();
}

/**
 * @brief Resize the windows of Call List
 *
 * This function will be invoked when the ui size has changed
 *
 * @param window UI structure pointer
 * @return 0 if the panel has been resized, -1 otherwise
 */
static int
call_list_resize(Window *window)
{
    CallListWindow *self = NCURSES_CALL_LIST(window);

    // Get current screen dimensions
    gint maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);

    // Change the main window size
    wresize(window_get_ncurses_window(window), maxy, maxx);

    // Store new size
    window_set_width(window, maxx);
    window_set_height(window, maxy);

    // Calculate available printable area
    wresize(self->list_win, maxy - 6, maxx);

    // Force list redraw
    call_list_win_clear(window);

    return 0;
}

/**
 * @brief Draw panel header
 *
 * This function will draw Call list header
 *
 * @param window UI structure pointer
 */
static void
call_list_draw_header(CallListWindow *self)
{
    const char *infile;
    const char *countlb;
    const char *device, *filterbpf;

    // Draw panel title
    window_set_title(NCURSES_WINDOW(self), "sngrep - SIP messages flow viewer");

    // Draw a Panel header lines
    window_clear_line(NCURSES_WINDOW(self), 1);

    WINDOW *win = window_get_ncurses_window(NCURSES_WINDOW(self));

    mvwprintw(win, 1, 2, "Current Mode: ");
    if (capture_is_online(capture_manager_get_instance())) {
        wattron(win, COLOR_PAIR(CP_GREEN_ON_DEF));
    } else {
        wattron(win, COLOR_PAIR(CP_RED_ON_DEF));
    }
    wprintw(win, "%s ", capture_status_desc(capture_manager_get_instance()));

    if (!capture_is_online(capture_manager_get_instance())) {
        guint progress = capture_manager_load_progress(capture_manager_get_instance());
        if (progress > 0 && progress < 100) {
            wprintw(win, "[%d%%]", progress);
        }
    }

    // Get online mode capture device
    if ((device = capture_input_pcap_device(capture_manager_get_instance())))
        wprintw(win, "[%s]", device);

#ifdef USE_HEP
    const char *eep_port;
    if ((eep_port = capture_output_hep_port(capture_manager_get_instance()))) {
        wprintw(win, "[H:%s]", eep_port);
    }
    if ((eep_port = capture_input_hep_port(capture_manager_get_instance()))) {
        wprintw(win, "[L:%s]", eep_port);
    }
#endif

    wattroff(win, COLOR_PAIR(CP_GREEN_ON_DEF));
    wattroff(win, COLOR_PAIR(CP_RED_ON_DEF));

    // Label for Display filter
    mvwprintw(win, 3, 2, "Display Filter: ");

    mvwprintw(win, 2, 2, "BPF Filter: ");
    wattron(win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    if ((filterbpf = capture_manager_filter(capture_manager_get_instance()))) {
        wprintw(win, "%s", filterbpf);
    }
    wattroff(win, COLOR_PAIR(CP_YELLOW_ON_DEF));

    StorageMatchOpts match = storage_match_options();

    if (match.mexpr) {
        wprintw(win, "%s", "        Match Expression: ");
        wattron(win, COLOR_PAIR(CP_YELLOW_ON_DEF));
        wprintw(win, "%s", match.mexpr);
        wattroff(win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    }

    // Reverse colors on monochrome terminals
    if (!has_colors())
        wattron(win, A_REVERSE);

    // Print Dialogs or Calls in label depending on calls filter
    StorageMatchOpts storageMatchOpts = storage_match_options();
    if (storageMatchOpts.invite) {
        countlb = "Calls";
    } else {
        countlb = "Dialogs";
    }

    // Print calls count (also filtered)
    StorageStats stats = storage_calls_stats();
    mvwprintw(win, 1, 45, "%*s", 30, "");
    if (stats.total != stats.displayed) {
        mvwprintw(win, 1, 45, "%s: %d (%d displayed)", countlb, stats.total, stats.displayed);
    } else {
        mvwprintw(win, 1, 45, "%s: %d", countlb, stats.total);
    }

    if (storage_memory_limit() > 0) {
        g_autofree const gchar *usage = g_format_size_full(
            storage_memory_usage(),
            G_FORMAT_SIZE_IEC_UNITS
        );
        g_autofree const gchar *limit = g_format_size_full(
            storage_memory_limit(),
            G_FORMAT_SIZE_IEC_UNITS
        );

        mvwprintw(win, 1, 65, "Memory: %s / %s", usage, limit);
    }

    // Print Open filename in Offline mode
    if (!capture_is_online(capture_manager_get_instance()) &&
        (infile = capture_input_pcap_file(capture_manager_get_instance()))) {
        mvwprintw(win, 1, 98, "Filename: %s", infile);
    }

    if (self->menu_active) {
        // Draw separator line
        wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
        mvwprintw(win, 4, 0, "Sort by     ");
        wattroff(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    }
}

/**
 * @brief Draw panel footer
 *
 * This function will draw Call list footer that contains
 * keybinginds
 *
 * @param window UI structure pointer
 */
static void
call_list_draw_footer(CallListWindow *self)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Quit",
        key_action_key_str(ACTION_SELECT), "Select",
        key_action_key_str(ACTION_SHOW_HELP), "Help",
        key_action_key_str(ACTION_SAVE), "Save",
        key_action_key_str(ACTION_DISP_FILTER), "Search",
        key_action_key_str(ACTION_SHOW_FLOW_EX), "Extended",
        key_action_key_str(ACTION_CLEAR_CALLS), "Clear",
        key_action_key_str(ACTION_SHOW_FILTERS), "Filter",
        key_action_key_str(ACTION_SHOW_SETTINGS), "Settings",
        key_action_key_str(ACTION_SHOW_COLUMNS), "Columns"
    };

    window_draw_bindings(NCURSES_WINDOW(self), keybindings, 20);
}

static gint
call_list_columns_width(CallListWindow *self, guint columns)
{
    // More requested columns that existing columns??
    if (columns > g_ptr_array_len(self->columns)) {
        columns = g_ptr_array_len(self->columns);
    }

    // If requested column is 0, count all columns
    guint columncnt = (columns == 0) ? g_ptr_array_len(self->columns) : columns;

    // Add extra width for spaces between columns + selection box
    gint width = 5 + columncnt;

    // Sum all column widths
    for (guint i = 0; i < columncnt; i++) {
        CallListColumn *column = g_ptr_array_index(self->columns, i);
        width += column->width;
    }

    return width;
}

/**
 * @brief Draw panel list contents
 *
 * This function will draw Call list dialogs list
 *
 * @param window UI structure pointer
 */
static void
call_list_draw_list(CallListWindow *self)
{
    gint listh, listw, cline = 0;
    gint color;

    // Get window of call list panel
    WINDOW *list_win = self->list_win;
    getmaxyx(list_win, listh, listw);

    // Get the list of calls that are going to be displayed
    g_ptr_array_free(self->dcalls, TRUE);
    self->dcalls = g_ptr_array_copy_filtered(storage_calls(), (GEqualFunc) filter_check_call, NULL);

    // If autoscroll is enabled, select the last dialog
    if (self->autoscroll) {
        StorageSortOpts sort = storage_sort_options();
        if (sort.asc) {
            call_list_move(self, g_ptr_array_len(self->dcalls) - 1);
        } else {
            call_list_move(self, 0);
        }
    }

    // Clear call list before redrawing
    werase(list_win);

    // Create a new pad for configured columns
    gint padw = call_list_columns_width(self, 0);
    if (padw < listw) padw = listw;

    WINDOW *pad = newpad(listh + 1, padw);

    // Get configured sorting options
    StorageSortOpts sort = storage_sort_options();

    // Draw columns titles
    wattron(pad, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    mvwprintw(pad, 0, 0, "%*s", padw, "");

    // Draw columns
    gint colpos = 6;
    for (guint i = 0; i < g_ptr_array_len(self->columns); i++) {
        CallListColumn *column = g_ptr_array_index(self->columns, i);
        // Get current column title
        const gchar *coldesc = attr_title(column->id);

        // Print sort column indicator
        if (column->id == sort.by) {
            wattron(pad, A_BOLD | COLOR_PAIR(CP_YELLOW_ON_CYAN));
            gchar sortind = (gchar) ((sort.asc) ? '^' : 'v');
            mvwprintw(pad, 0, colpos, "%c%.*s", sortind, column->width, coldesc);
            wattron(pad, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
        } else {
            mvwprintw(pad, 0, colpos, "%.*s", column->width, coldesc);
        }
        colpos += column->width + 1;
    }
    wattroff(pad, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));

    // Fill the call list
    cline = 1;
    for (guint i = (guint) self->vscroll.pos; i < g_ptr_array_len(self->dcalls); i++) {
        Call *call = g_ptr_array_index(self->dcalls, i);
        g_return_if_fail(call != NULL);

        // Get first call message attributes
        Message *msg = g_ptr_array_first(call->msgs);
        g_return_if_fail(msg != NULL);

        // Stop if we have reached the bottom of the list
        if (cline == listh)
            break;

        // Show bold selected rows
        if (call_group_exists(self->group, call))
            wattron(pad, A_BOLD | COLOR_PAIR(CP_DEFAULT));

        // Highlight active call
        if (self->cur_idx == (gint) i) {
            wattron(pad, COLOR_PAIR(CP_WHITE_ON_BLUE));
        }

        // Set current line background
        mvwprintw(pad, cline, 0, "%*s", padw, "");
        // Set current line selection box
        mvwprintw(pad, cline, 2, call_group_exists(self->group, call) ? "[*]" : "[ ]");

        // Print requested columns
        colpos = 6;
        for (guint j = 0; j < g_ptr_array_len(self->columns); j++) {
            CallListColumn *column = g_ptr_array_index(self->columns, j);

            // Get call attribute for current column
            const gchar *coltext = NULL;
            if ((coltext = msg_get_attribute(msg, column->id)) == NULL) {
                colpos += column->width + 1;
                continue;
            }

            // Enable attribute color (if not current one)
            color = 0;
            if (self->cur_idx != (gint) i) {
                if ((color = attr_color(column->id, coltext)) > 0) {
                    wattron(pad, color);
                }
            }

            // Add the column text to the existing columns
            mvwprintw(pad, cline, colpos, "%.*s", column->width, coltext);
            colpos += column->width + 1;

            // Disable attribute color
            if (color > 0)
                wattroff(pad, color);
        }
        cline++;

        wattroff(pad, COLOR_PAIR(CP_DEFAULT));
        wattroff(pad, COLOR_PAIR(CP_DEF_ON_BLUE));
        wattroff(pad, A_BOLD | A_REVERSE);
    }

    // Copy the pad into list win
    copywin(pad, self->list_win, 0, self->hscroll.pos, 0, 0,
            listh - 1, listw - 1, 0);

    // Copy fixed columns
    guint fixed_width = call_list_columns_width(self, (guint) setting_get_intvalue(SETTING_CL_FIXEDCOLS));
    copywin(pad, self->list_win, 0, 0, 0, 0, listh - 1, fixed_width, 0);

    // Setup horizontal scrollbar
    self->hscroll.max = call_list_columns_width(self, 0);
    self->hscroll.preoffset = 1;    // Leave first column for vscroll

    // Setup vertical scrollbar
    self->vscroll.max = g_ptr_array_len(self->dcalls) - 1;
    self->vscroll.preoffset = 1;    // Leave first row for titles
    if (scrollbar_visible(self->hscroll)) {
        self->vscroll.postoffset = 1;
    }   // Leave last row for hscroll

    // Draw scrollbars if required
    scrollbar_draw(self->hscroll);
    scrollbar_draw(self->vscroll);

    // Free the list pad
    delwin(pad);

    // Print Autoscroll indicator
    if (self->autoscroll) {
        wattron(self->list_win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
        mvwprintw(self->list_win, 0, 0, "A");
        wattroff(self->list_win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    }

    // Refresh the list
    if (!self->menu_active) {
        wnoutrefresh(self->list_win);
    }
}

/**
 * @brief Draw the Call list panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param window UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
static int
call_list_draw(Window *window)
{
    int cury, curx;

    CallListWindow *self = NCURSES_CALL_LIST(window);

    // Store cursor position
    getyx(window_get_ncurses_window(window), cury, curx);

    // Draw the header
    call_list_draw_header(self);
    // Draw the footer
    call_list_draw_footer(self);
    // Draw the list content
    call_list_draw_list(self);

    // Restore cursor position
    wmove(window_get_ncurses_window(window), cury, curx);

    return 0;
}

/**
 * @brief Enable/Disable Panel form focus
 *
 * Enable or disable form fields focus so the next input will be
 * handled by call_list_handle_key or call_list_handle_form_key
 * This will also set properties in fields to show them as focused
 * and show/hide the cursor
 *
 * @param window UI structure pointer
 * @param active Enable/Disable flag
 */
static void
call_list_form_activate(CallListWindow *self, gboolean active)
{
    // Store form state
    self->form_active = active;

    if (active) {
        set_current_field(self->form, self->fields[FLD_LIST_FILTER]);
        // Show cursor
        curs_set(1);
        // Change current field background
        set_field_back(self->fields[FLD_LIST_FILTER], A_REVERSE);
    } else {
        set_current_field(self->form, NULL);
        // Hide cursor
        curs_set(0);
        // Change current field background
        set_field_back(self->fields[FLD_LIST_FILTER], A_NORMAL);
    }
    post_form(self->form);
    form_driver(self->form, REQ_END_LINE);
}

/**
 * @brief Get List line from the given call
 *
 * Get the list line of the given call to display in the list
 * This line is built using the configured columns and sizes
 *
 * @param window UI structure pointer
 * @param call Call to get data from
 * @param text Text pointer to store the generated line
 * @return A pointer to text
 */
const char *
call_list_win_line_text(Window *window, Call *call)
{
    CallListWindow *self = NCURSES_CALL_LIST(window);

    // Get first call message
    Message *msg = g_ptr_array_first(call->msgs);
    g_return_val_if_fail(msg != NULL, NULL);

    // Print requested columns
    GString *out = g_string_new(NULL);
    for (guint i = 0; i < g_ptr_array_len(self->columns); i++) {
        CallListColumn *column = g_ptr_array_index(self->columns, i);

        // Get call attribute for current column
        const gchar *call_attr = NULL;
        if ((call_attr = msg_get_attribute(msg, column->id)) != NULL) {
            g_string_append(out, call_attr);
        }
    }

    return out->str;
}

/**
 * @brief Select column to sort by
 *
 * This function will display a lateral menu to select the column to sort
 * the list for. The menu will be filled with current displayed columns.
 *
 * @param window UI structure pointer
 */
static void
call_list_select_sort_attribute(CallListWindow *self)
{
    // Get current sort options
    StorageSortOpts sort = storage_sort_options();

    // Activate sorting menu
    self->menu_active = 1;

    // Move call list to the right
    gint height = window_get_height(NCURSES_WINDOW(self));
    gint width = window_get_width(NCURSES_WINDOW(self));
    WINDOW *win = window_get_ncurses_window(NCURSES_WINDOW(self));

    wresize(self->list_win, height - 5, width - 12);
    mvderwin(self->list_win, 4, 12);

    // Create menu entries
    ITEM *selected = NULL;
    for (guint i = 0; i < g_ptr_array_len(self->columns); i++) {
        CallListColumn *column = g_ptr_array_index(self->columns, i);
        self->items[i] = new_item(attr_name(column->id), 0);
        if (column->id == sort.by) {
            selected = self->items[i];
        }
    }

    self->items[g_ptr_array_len(self->columns)] = NULL;
    // Create the columns menu and post it
    self->menu = new_menu(self->items);

    // Set main window and sub window
    set_menu_win(self->menu, win);
    set_menu_sub(self->menu, derwin(win, 20, 15, 5, 0));
    werase(menu_win(self->menu));
    set_menu_format(self->menu, height, 1);
    set_menu_mark(self->menu, "");
    set_menu_fore(self->menu, COLOR_PAIR(CP_DEF_ON_BLUE));
    set_current_item(self->menu, selected);
    menu_opts_off(self->menu, O_ONEVALUE);
    post_menu(self->menu);
}

/**
 * @brief Handle Forms entries key strokes
 *
 * This function will manage the custom keybindings of the panel form.
 *
 * @param window UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
static int
call_list_handle_form_key(CallListWindow *self, int key)
{
    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                // If this is a normal character on input field, print it
                form_driver(self->form, key);
                break;
            case ACTION_PREV_SCREEN:
            case ACTION_NEXT_FIELD:
            case ACTION_CONFIRM:
            case ACTION_SELECT:
            case ACTION_UP:
            case ACTION_DOWN:
                // Activate list
                call_list_form_activate(self, 0);
                break;
            case ACTION_RIGHT:
                form_driver(self->form, REQ_RIGHT_CHAR);
                break;
            case ACTION_LEFT:
                form_driver(self->form, REQ_LEFT_CHAR);
                break;
            case ACTION_BEGIN:
                form_driver(self->form, REQ_BEG_LINE);
                break;
            case ACTION_END:
                form_driver(self->form, REQ_END_LINE);
                break;
            case ACTION_CLEAR:
                form_driver(self->form, REQ_BEG_LINE);
                form_driver(self->form, REQ_CLR_EOL);
                break;
            case ACTION_DELETE:
                form_driver(self->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                form_driver(self->form, REQ_DEL_PREV);
                break;
            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Filter has changed, re-apply filter to displayed calls
    if (action == ACTION_PRINTABLE || action == ACTION_BACKSPACE ||
        action == ACTION_DELETE || action == ACTION_CLEAR) {
        // Updated displayed results
        call_list_win_clear(NCURSES_WINDOW(self));
        // Reset filters on each key stroke
        filter_reset_calls();
    }

    // Validate all input data
    form_driver(self->form, REQ_VALIDATION);

    // Store dfilter input
    gsize field_len = strlen(field_buffer(self->fields[FLD_LIST_FILTER], 0));
    gchar *dfilter = g_malloc0(field_len + 1);
    strncpy(dfilter, field_buffer(self->fields[FLD_LIST_FILTER], 0), field_len);
    g_strstrip(dfilter);    // Trim any trailing spaces

    // Set display filter
    filter_set(FILTER_CALL_LIST, strlen(dfilter) ? dfilter : NULL);
    g_free(dfilter);

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

/**
 * @brief Handle Sort menu key strokes
 *
 * This function will manage the custom keybidnings for sort menu
 *
 * @param window UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
static int
call_list_handle_menu_key(CallListWindow *self, int key)
{
    MENU *menu;
    int i;
    StorageSortOpts sort;
    enum AttributeId id;

    menu = self->menu;

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                menu_driver(menu, REQ_DOWN_ITEM);
                break;
            case ACTION_UP:
                menu_driver(menu, REQ_UP_ITEM);
                break;
            case ACTION_NPAGE:
                menu_driver(menu, REQ_SCR_DPAGE);
                break;
            case ACTION_PPAGE:
                menu_driver(menu, REQ_SCR_UPAGE);
                break;
            case ACTION_CONFIRM:
            case ACTION_SELECT:
                // Change sort attribute
                sort = storage_sort_options();
                id = attr_find_by_name(item_name(current_item(self->menu)));
                if (sort.by == id) {
                    sort.asc = (sort.asc) ? FALSE : TRUE;
                } else {
                    sort.by = id;
                }
                storage_set_sort_options(sort);
                /* fall-thru */
            case ACTION_PREV_SCREEN:
                // Deactivate sorting menu
                self->menu_active = FALSE;

                // Remove menu
                unpost_menu(self->menu);
                free_menu(self->menu);

                // Remove items
                for (i = 0; i < ATTR_COUNT; i++) {
                    if (!self->items[i])
                        break;
                    free_item(self->items[i]);
                }

                // Restore list position
                mvderwin(self->list_win, 4, 0);
                // Restore list window size
                wresize(self->list_win,
                        window_get_height(NCURSES_WINDOW(self)) - 5,
                        window_get_width(NCURSES_WINDOW(self))
                );
                break;
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
 * @brief Handle Call list key strokes
 *
 * This function will manage the custom keybindings of the panel.
 * This function return one of the values defined in @key_handler_ret
 *
 * @param window UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
static int
call_list_handle_key(Window *window, int key)
{
    guint rnpag_steps = (guint) setting_get_intvalue(SETTING_CL_SCROLLSTEP);
    CallGroup *group;
    Call *call;
    StorageSortOpts sort;

    CallListWindow *self = NCURSES_CALL_LIST(window);

    // Handle form key
    if (self->form_active)
        return call_list_handle_form_key(self, key);

    if (self->menu_active)
        return call_list_handle_menu_key(self, key);

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RIGHT:
                call_list_move_right(self, 3);
                break;
            case ACTION_LEFT:
                call_list_move_left(self, 3);
                break;
            case ACTION_DOWN:
                call_list_move_down(self, 1);
                break;
            case ACTION_UP:
                call_list_move_up(self, 1);
                break;
            case ACTION_HNPAGE:
                call_list_move_down(self, rnpag_steps / 2);
                break;
            case ACTION_NPAGE:
                call_list_move_down(self, rnpag_steps);
                break;
            case ACTION_HPPAGE:
                call_list_move_up(self, rnpag_steps / 2);
                break;
            case ACTION_PPAGE:
                call_list_move_up(self, rnpag_steps);
                break;
            case ACTION_BEGIN:
                // Move to first list entry
                call_list_move(self, 0);
                break;
            case ACTION_END:
                call_list_move(self, g_ptr_array_len(self->dcalls) - 1);
                break;
            case ACTION_DISP_FILTER:
                // Activate Form
                call_list_form_activate(self, 1);
                break;
            case ACTION_SHOW_FLOW:
            case ACTION_SHOW_FLOW_EX:
            case ACTION_SHOW_RAW:
                // Check we have calls in the list
                if (g_ptr_array_empty(self->dcalls))
                    break;

                // Create a new group of calls
                group = call_group_clone(self->group);

                // If not selected call, show current call flow
                if (call_group_count(group) == 0)
                    call_group_add(group, g_ptr_array_index(self->dcalls, self->cur_idx));

                // Add xcall to the group
                if (action == ACTION_SHOW_FLOW_EX) {
                    call = g_ptr_array_index(self->dcalls, self->cur_idx);
                    call_group_add_calls(group, call->xcalls);
                    group->callid = call->callid;
                }

                if (action == ACTION_SHOW_RAW) {
                    // Create a Call raw panel
                    call_raw_win_set_group(ncurses_create_window(WINDOW_CALL_RAW), group);
                } else {
                    // Display current call flow (normal or extended)
                    call_flow_win_set_group(ncurses_create_window(WINDOW_CALL_FLOW), group);
                }
                break;
            case ACTION_SHOW_FILTERS:
                ncurses_create_window(WINDOW_FILTER);
                break;
            case ACTION_SHOW_COLUMNS:
                column_select_win_set_columns(ncurses_create_window(WINDOW_COLUMN_SELECT), self->columns);
                break;
            case ACTION_SHOW_STATS:
                ncurses_create_window(WINDOW_STATS);
                break;
            case ACTION_SAVE:
                save_set_group(ncurses_create_window(WINDOW_SAVE), self->group);
                break;
            case ACTION_CLEAR:
                // Clear group calls
                call_group_remove_all(self->group);
                break;
            case ACTION_CLEAR_CALLS:
                // Remove all stored calls
                storage_calls_clear();
                // Clear List
                call_list_win_clear(window);
                break;
            case ACTION_CLEAR_CALLS_SOFT:
                // Remove stored calls, keeping the currently displayed calls
                storage_calls_clear_soft();
                // Clear List
                call_list_win_clear(window);
                break;
            case ACTION_AUTOSCROLL:
                self->autoscroll = (self->autoscroll) ? 0 : 1;
                break;
            case ACTION_SHOW_SETTINGS:
                ncurses_create_window(WINDOW_SETTINGS);
                break;
            case ACTION_SELECT:
                // Ignore on empty list
                if (g_ptr_array_len(self->dcalls) == 0) {
                    break;
                }

                call = g_ptr_array_index(self->dcalls, self->cur_idx);
                if (call_group_exists(self->group, call)) {
                    call_group_remove(self->group, call);
                } else {
                    call_group_add(self->group, call);
                }
                break;
            case ACTION_SORT_SWAP:
                // Change sort order
                sort = storage_sort_options();
                sort.asc = (sort.asc) ? FALSE : TRUE;
                storage_set_sort_options(sort);
                break;
            case ACTION_SORT_NEXT:
            case ACTION_SORT_PREV:
                call_list_select_sort_attribute(self);
                break;
            case ACTION_PREV_SCREEN:
                // Handle quit from this screen unless requested
                if (setting_enabled(SETTING_EXITPROMPT)) {
                    if (dialog_confirm("Confirm exit", "Are you sure you want to quit?", "Yes,No") == 0) {
                        return KEY_DESTROY;
                    }
                } else {
                    return KEY_DESTROY;
                }
                return KEY_HANDLED;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Disable autoscroll on some key pressed
    switch (action) {
        case ACTION_DOWN:
        case ACTION_UP:
        case ACTION_HNPAGE:
        case ACTION_HPPAGE:
        case ACTION_NPAGE:
        case ACTION_PPAGE:
        case ACTION_BEGIN:
        case ACTION_END:
        case ACTION_DISP_FILTER:
            self->autoscroll = 0;
            break;
        default:
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
call_list_help(G_GNUC_UNUSED Window *window)
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

    // Some brief explanation abotu what window shows
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

static gint
call_list_column_sorter(const CallListColumn **a, const CallListColumn **b)
{
    if ((*a)->position > (*b)->position) return 1;
    if ((*a)->position < (*b)->position) return -1;
    return 0;
}

/**
 * @brief Add a column the Call List
 *
 * This function will add a new column to the Call List panel
 *
 * @param window UI structure pointer
 * @param id SIP call attribute id
 * @param attr SIP call attribute name
 * @param title SIP call attribute description
 * @param width Column Width
 * @return 0 if column has been successufly added to the list, -1 otherwise
 */
static void
call_list_add_column(CallListWindow *self, enum AttributeId id, const char *attr,
                     const char *title, gint position, int width)
{
    CallListColumn *column = g_malloc0(sizeof(CallListColumn));
    column->id = id;
    column->attr = attr;
    column->title = title;
    column->position = position;
    column->width = width;
    g_ptr_array_add(self->columns, column);
}

void
call_list_win_clear(Window *window)
{
    CallListWindow *self = NCURSES_CALL_LIST(window);

    // Initialize structures
    self->vscroll.pos = self->cur_idx = 0;
    call_group_remove_all(self->group);

    // Clear Displayed lines
    werase(self->list_win);
    wnoutrefresh(self->list_win);
}

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param window UI structure pointer
 */
static void
call_list_finalize(GObject *object)
{
    CallListWindow *self = NCURSES_CALL_LIST(object);

    // Deallocate forms data
    if (self->form) {
        unpost_form(self->form);
        free_form(self->form);
        free_field(self->fields[FLD_LIST_FILTER]);
    }

    // Deallocate window private data
    call_group_free(self->group);
    g_ptr_array_free(self->columns, TRUE);
    g_ptr_array_free(self->dcalls, TRUE);
    delwin(self->list_win);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(call_list_parent_class)->finalize(object);
}

Window *
call_list_win_new()
{
    Window *window = g_object_new(
        WINDOW_TYPE_CALL_LIST,
        "height", getmaxy(stdscr),
        "width", getmaxx(stdscr),
        NULL
    );
    return window;
}

static void
call_list_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(call_list_parent_class)->constructed(object);

    CallListWindow *self = NCURSES_CALL_LIST(object);

    // Add configured columns
    self->columns = g_ptr_array_new_with_free_func(g_free);
    for (guint attr_id = 0; attr_id < ATTR_COUNT; attr_id++) {
        // Get column attribute name from options
        gint position = setting_column_pos(attr_id);
        // Check column is visible
        if (position == -1) continue;

        // Get column information
        gint collen = setting_column_width(attr_id);
        const gchar *title = attr_title(attr_id);
        const gchar *field = attr_name(attr_id);

        // Add column to the list
        call_list_add_column(self, attr_id, field, title, position, collen);
    }
    g_ptr_array_sort(self->columns, (GCompareFunc) call_list_column_sorter);

    gint width = window_get_width(NCURSES_WINDOW(self));
    gint height = window_get_height(NCURSES_WINDOW(self));
    WINDOW *win = window_get_ncurses_window(NCURSES_WINDOW(self));

    // Initialize the fields
    self->fields[FLD_LIST_FILTER] = new_field(1, width - 19, 3, 18, 0, 0);
    self->fields[FLD_LIST_COUNT] = NULL;

    // Create the form and post it
    self->form = new_form(self->fields);
    set_form_sub(self->form, win);

    // Form starts inactive
    call_list_form_activate(self, 0);
    self->menu_active = FALSE;

    // Calculate available printable area
    self->list_win = subwin(win, height - 5, width, 4, 0);
    self->vscroll = window_set_scrollbar(self->list_win, SB_VERTICAL, SB_LEFT);
    self->hscroll = window_set_scrollbar(self->list_win, SB_HORIZONTAL, SB_BOTTOM);

    // Set autoscroll default status
    self->autoscroll = setting_enabled(SETTING_CL_AUTOSCROLL);

    // Apply initial configured filters
    filter_method_from_setting(setting_get_value(SETTING_FILTER_METHODS));
    filter_payload_from_setting(setting_get_value(SETTING_FILTER_PAYLOAD));
}

static void
call_list_class_init(CallListWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = call_list_constructed;
    object_class->finalize = call_list_finalize;

    WindowClass *window_class = NCURSES_WINDOW_CLASS(klass);
    window_class->redraw = call_list_redraw;
    window_class->draw = call_list_draw;
    window_class->resize = call_list_resize;
    window_class->handle_key = call_list_handle_key;
    window_class->help = call_list_help;
}

static void
call_list_init(G_GNUC_UNUSED CallListWindow *self)
{
    // Set parent attributes
    window_set_window_type(NCURSES_WINDOW(self), WINDOW_CALL_LIST);

    // Initialize Call List attributes
    self->group = call_group_new();
    self->dcalls = g_ptr_array_new();
}
