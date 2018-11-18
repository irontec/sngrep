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
 * @file ui_call_list.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_list.h
 *
 */
#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "glib-utils.h"
#include "option.h"
#include "filter.h"
#include "capture/capture_pcap.h"
#ifdef USE_HEP
#include "capture/capture_hep.h"
#endif
#include "curses/ui_manager.h"
#include "curses/screens/ui_call_list.h"
#include "curses/screens/ui_call_flow.h"
#include "curses/screens/ui_call_raw.h"
#include "curses/screens/ui_filter.h"
#include "curses/screens/ui_save.h"
#include "storage.h"

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param window UI structure pointer
 * @return a pointer to info structure of given panel
 */
/* @FIXME static */
CallListInfo *
call_list_info(Window *window)
{
    return (CallListInfo*) panel_userptr(window->panel);
}

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
call_list_move(Window *window, guint line)
{
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    // Already in this position?
    if (info->cur_call == line)
        return;

    // Moving down or up?
    bool move_down = (info->cur_call < line);

    if (move_down) {
        while (info->cur_call < line) {
            // Check if there is a call below us
            if (info->cur_call == g_ptr_array_len(info->dcalls) - 1)
                break;

            // Increase current call position
            info->cur_call++;

            // If we are out of the bottom of the displayed list
            // refresh it starting in the next call
            if ((gint)(info->cur_call - info->scroll.pos) == getmaxy(info->list_win)) {
                info->scroll.pos++;
            }
        }
    } else {
        while (info->cur_call > line) {
            // Check if there is a call above us
            if (info->cur_call == 0)
                break;

            // If we are out of the top of the displayed list
            // refresh it starting in the previous (in fact current) call
            if (info->cur_call == info->scroll.pos) {
                info->scroll.pos--;
            }
            // Move current call position
            info->cur_call--;
        }
    }
}

static void
call_list_move_up(Window *window, guint times)
{
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    gint newpos = info->cur_call - times;
    if (newpos < 0)
        newpos = 0;

    call_list_move(window, (guint) newpos);
}

static void
call_list_move_down(Window *window, guint times)
{
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    guint newpos = info->cur_call + times;
    if (newpos >= g_ptr_array_len(info->dcalls))
        newpos = g_ptr_array_len(info->dcalls) - 1;

    call_list_move(window, newpos);
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
    int maxx, maxy;

    // Get panel info
    CallListInfo *info = call_list_info(window);
    g_return_val_if_fail(info != NULL, -1);

    // Get current screen dimensions
    getmaxyx(stdscr, maxy, maxx);

    // Change the main window size
    wresize(window->win, maxy, maxx);

    // Store new size
    window->width = maxx;
    window->height = maxy;

    // Calculate available printable area
    wresize(info->list_win, maxy - 6, maxx); //-4
    // Force list redraw
    call_list_clear(window);

    return 0;
}

/**
 * @brief Draw panel header
 *
 * This funtion will draw Call list header
 *
 * @param window UI structure pointer
 */
static void
call_list_draw_header(Window *window)
{
    const char *infile, *coldesc;
    int colpos, collen;
    char sortind;
    const char *countlb;
    const char *device, *filterbpf;

    // Get panel info
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    // Draw panel title
    ui_set_title(window, "sngrep - SIP messages flow viewer");

    // Draw a Panel header lines
    ui_clear_line(window, 1);

    // Print Open filename in Offline mode
    if (!capture_is_online(capture_manager()) && (infile = capture_input_pcap_file(capture_manager())))
        mvwprintw(window->win, 1, 77, "Filename: %s", infile);

    mvwprintw(window->win, 1, 2, "Current Mode: ");
    if (capture_is_online(capture_manager())) {
        wattron(window->win, COLOR_PAIR(CP_GREEN_ON_DEF));
    } else {
        wattron(window->win, COLOR_PAIR(CP_RED_ON_DEF));
    }
    wprintw(window->win, "%s ", capture_status_desc(capture_manager()));

    // Get online mode capture device
    if ((device = capture_input_pcap_device(capture_manager())))
        wprintw(window->win, "[%s]", device);

#ifdef USE_HEP
    const char *eep_port;
    if ((eep_port = capture_output_hep_port(capture_manager()))) {
        wprintw(ui->win, "[H:%s]", eep_port);
    }
    if ((eep_port = capture_input_hep_port(capture_manager()))) {
        wprintw(ui->win, "[L:%s]", eep_port);
    }
#endif

    wattroff(window->win, COLOR_PAIR(CP_GREEN_ON_DEF));
    wattroff(window->win, COLOR_PAIR(CP_RED_ON_DEF));

    // Label for Display filter
    mvwprintw(window->win, 3, 2, "Display Filter: ");

    mvwprintw(window->win, 2, 2, "Match Expression: ");

    wattron(window->win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    StorageMatchOpts match = storage_match_options();
    if (match.mexpr)
        wprintw(window->win, "%s", match.mexpr);
    wattroff(window->win, COLOR_PAIR(CP_YELLOW_ON_DEF));

    mvwprintw(window->win, 2, 45, "BPF Filter: ");
    wattron(window->win, COLOR_PAIR(CP_YELLOW_ON_DEF));
    if ((filterbpf = capture_manager_filter(capture_manager())))
        wprintw(window->win, "%s", filterbpf);
    wattroff(window->win, COLOR_PAIR(CP_YELLOW_ON_DEF));

    // Reverse colors on monochrome terminals
    if (!has_colors())
        wattron(window->win, A_REVERSE);

    // Get configured sorting options
    StorageSortOpts sort = storage_sort_options();

    // Draw columns titles
    wattron(window->win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    ui_clear_line(window, 4);

    // Draw separator line
    if (info->menu_active) {
        colpos = 18;
        mvwprintw(window->win, 4, 0, "Sort by");
        wattron(window->win, A_BOLD | COLOR_PAIR(CP_CYAN_ON_BLACK));
        mvwprintw(window->win, 4, 11, "%*s", 1, "");
        wattron(window->win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    } else {
        colpos = 6;
    }

    for (guint i = 0; i < info->columncnt; i++) {
        // Get current column width
        collen = info->columns[i].width;
        // Get current column title
        coldesc = sip_attr_get_title(info->columns[i].id);

        // Check if the column will fit in the remaining space of the screen
        if (colpos + strlen(coldesc) >= (guint) window->width)
            break;

        // Print sort column indicator
        if (info->columns[i].id == sort.by) {
            wattron(window->win, A_BOLD | COLOR_PAIR(CP_YELLOW_ON_CYAN));
            sortind = (sort.asc) ? '^' : 'v';
            mvwprintw(window->win, 4, colpos, "%c%.*s", sortind, collen, coldesc);
            wattron(window->win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
        } else {
            mvwprintw(window->win, 4, colpos, "%.*s", collen, coldesc);
        }
        colpos += collen + 1;
    }
    // Print Autoscroll indicator
    if (info->autoscroll)
        mvwprintw(window->win, 4, 0, "A");
    wattroff(window->win, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_CYAN));

    // Print Dialogs or Calls in label depending on calls filter
    if (setting_enabled(SETTING_SIP_CALLS)) {
        countlb = "Calls";
    } else {
        countlb = "Dialogs";
    }

    // Print calls count (also filtered)
    sip_stats_t stats = storage_calls_stats();
    mvwprintw(window->win, 1, 45, "%*s", 30, "");
    if (stats.total != stats.displayed) {
        mvwprintw(window->win, 1, 45, "%s: %d (%d displayed)", countlb, stats.total, stats.displayed);
    } else {
        mvwprintw(window->win, 1, 45, "%s: %d", countlb, stats.total);
    }

}

/**
 * @brief Draw panel footer
 *
 * This funtion will draw Call list footer that contains
 * keybinginds
 *
 * @param window UI structure pointer
 */
static void
call_list_draw_footer(Window *window)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Quit",
        key_action_key_str(ACTION_SHOW_FLOW), "Show",
        key_action_key_str(ACTION_SELECT), "Select",
        key_action_key_str(ACTION_SHOW_HELP), "Help",
        key_action_key_str(ACTION_SAVE), "Save",
        key_action_key_str(ACTION_DISP_FILTER), "Search",
        key_action_key_str(ACTION_SHOW_FLOW_EX), "Extended",
        key_action_key_str(ACTION_CLEAR_CALLS), "Clear",
        key_action_key_str(ACTION_SHOW_FILTERS), "Filter",
        key_action_key_str(ACTION_SHOW_SETTINGS), "Settings",
        key_action_key_str(ACTION_CLEAR_CALLS_SOFT), "Clear with Filter",
        key_action_key_str(ACTION_SHOW_COLUMNS), "Columns"
    };

    ui_draw_bindings(window, keybindings, 23);
}

/**
 * @brief Draw panel list contents
 *
 * This funtion will draw Call list dialogs list
 *
 * @param window UI structure pointer
 */
static void
call_list_draw_list(Window *window)
{
    WINDOW *list_win;
    int listh, listw, cline = 0;
    SipCall *call = NULL;
    int collen;
    char coltext[SIP_ATTR_MAXLEN];
    int colid;
    int colpos;
    int color;

    // Get panel info
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    // Get window of call list panel
    list_win = info->list_win;
    getmaxyx(list_win, listh, listw);

    // Get the list of calls that are goint to be displayed
    g_ptr_array_free(info->dcalls, FALSE);
    info->dcalls = g_ptr_array_copy_filtered(storage_calls(), filter_check_call, NULL);

    // Empty list, nothing to draw
    if (g_ptr_array_len(info->dcalls) == 0)
        return;

    // If autoscroll is enabled, select the last dialog
    if (info->autoscroll)  {
        StorageSortOpts sort = storage_sort_options();
        if (sort.asc) {
            call_list_move(window, g_ptr_array_len(info->dcalls) - 1);
        } else {
            call_list_move(window, 0);
        }
    }

    // Clear call list before redrawing
    werase(list_win);

    // Fill the call list
    for (guint i = info->scroll.pos; i < g_ptr_array_len(info->dcalls); i++) {
        call = g_ptr_array_index(info->dcalls, i);

        // Stop if we have reached the bottom of the list
        if (cline == listh)
            break;

        // Show bold selected rows
        if (call_group_exists(info->group, call))
            wattron(list_win, A_BOLD | COLOR_PAIR(CP_DEFAULT));

        // Highlight active call
        if (info->cur_call == i) {
            wattron(list_win, COLOR_PAIR(CP_WHITE_ON_BLUE));
        }

        // Set current line background
        mvwprintw(list_win, cline, 0, "%*s", listw, "");
        // Set current line selection box
        mvwprintw(list_win, cline, 2, call_group_exists(info->group, call) ? "[*]" : "[ ]");

        // Print requested columns
        colpos = 6;
        for (guint j = 0; j < info->columncnt; j++) {
            // Get current column id
            colid = info->columns[j].id;
            // Get current column width
            collen = info->columns[j].width;

            // Check if next column fits on window width
            if (colpos + collen >= listw)
                break;

            // Initialize column text
            memset(coltext, 0, sizeof(coltext));

            // Get call attribute for current column
            if (!call_get_attribute(call, colid, coltext)) {
                colpos += collen + 1;
                continue;
            }

            // Enable attribute color (if not current one)
            color = 0;
            if (info->cur_call !=  i) {
                if ((color = sip_attr_get_color(colid, coltext)) > 0) {
                    wattron(list_win, color);
                }
            }

            // Add the column text to the existing columns
            mvwprintw(list_win, cline, colpos, "%.*s", collen, coltext);
            colpos += collen + 1;

            // Disable attribute color
            if (color > 0)
                wattroff(list_win, color);
        }
        cline++;

        wattroff(list_win, COLOR_PAIR(CP_DEFAULT));
        wattroff(list_win, COLOR_PAIR(CP_DEF_ON_BLUE));
        wattroff(list_win, A_BOLD | A_REVERSE);
    }

    // Draw scrollbar to the right
    info->scroll.max = g_ptr_array_len(info->dcalls) - 1;
    ui_scrollbar_draw(info->scroll);

    // Refresh the list
    if (!info->menu_active)
        wnoutrefresh(info->list_win);
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

    // Store cursor position
    getyx(window->win, cury, curx);

    // Draw the header
    call_list_draw_header(window);
    // Draw the footer
    call_list_draw_footer(window);
    // Draw the list content
    call_list_draw_list(window);

    // Restore cursor position
    wmove(window->win, cury, curx);

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
call_list_form_activate(Window *window, int active)
{
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    // Store form state
    info->form_active = active;

    if (active) {
        set_current_field(info->form, info->fields[FLD_LIST_FILTER]);
        // Show cursor
        curs_set(1);
        // Change current field background
        set_field_back(info->fields[FLD_LIST_FILTER], A_REVERSE);
    } else {
        set_current_field(info->form, NULL);
        // Hide cursor
        curs_set(0);
        // Change current field background
        set_field_back(info->fields[FLD_LIST_FILTER], A_NORMAL);
    }
    post_form(info->form);
    form_driver(info->form, REQ_END_LINE);
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
/* FIXME static */
const char *
call_list_line_text(Window *window, SipCall *call, char *text)
{
    int collen;
    char call_attr[SIP_ATTR_MAXLEN];
    char coltext[SIP_ATTR_MAXLEN];
    int colid;

    // Get panel info
    CallListInfo *info = call_list_info(window);
    g_return_val_if_fail(info != NULL, text);

    // Print requested columns
    for (guint i = 0; i < info->columncnt; i++) {

        // Get current column id
        colid = info->columns[i].id;

        // Get current column width
        collen = info->columns[i].width;

        // Check if next column fits on window width
        if ((gint)(strlen(text) + collen) >= window->width)
            collen = window->width - strlen(text);

        // If no space left on the screen stop processing columns
        if (collen <= 0)
            break;

        // Initialize column text
        memset(coltext, 0, sizeof(coltext));
        memset(call_attr, 0, sizeof(call_attr));

        // Get call attribute for current column
        if (call_get_attribute(call, colid, call_attr)) {
            sprintf(coltext, "%.*s", collen, call_attr);
        }
        // Add the column text to the existing columns
        sprintf(text + strlen(text), "%-*s ", collen, coltext);
    }

    return text;
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
call_list_select_sort_attribute(Window *window)
{
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    // Activete sorting menu
    info->menu_active = 1;

    wresize(info->list_win, window->height - 6, window->width - 12);
    mvderwin(info->list_win, 5, 12);

    // Create menu entries
    for (guint i = 0; i < info->columncnt; i++) {
        info->items[i] = new_item(sip_attr_get_name(info->columns[i].id), 0);
    }

    info->items[info->columncnt] = NULL;
    // Create the columns menu and post it
    info->menu = new_menu(info->items);

    // Set main window and sub window
    set_menu_win(info->menu, window->win);
    set_menu_sub(info->menu, derwin(window->win, 20, 15, 5, 0));
    werase(menu_win(info->menu));
    set_menu_format(info->menu, window->height, 1);
    set_menu_mark(info->menu, "");
    set_menu_fore(info->menu, COLOR_PAIR(CP_DEF_ON_BLUE));
    menu_opts_off(info->menu, O_ONEVALUE);
    post_menu(info->menu);
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
call_list_handle_form_key(Window *window, int key)
{
    char *dfilter;
    int action = -1;

    // Get panel information
    CallListInfo *info = call_list_info(window);
    g_return_val_if_fail(info != NULL, -1);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                // If this is a normal character on input field, print it
                form_driver(info->form, key);
                break;
            case ACTION_PREV_SCREEN:
            case ACTION_NEXT_FIELD:
            case ACTION_CONFIRM:
            case ACTION_SELECT:
            case ACTION_UP:
            case ACTION_DOWN:
                // Activate list
                call_list_form_activate(window, 0);
                break;
            case ACTION_RIGHT:
                form_driver(info->form, REQ_RIGHT_CHAR);
                break;
            case ACTION_LEFT:
                form_driver(info->form, REQ_LEFT_CHAR);
                break;
            case ACTION_BEGIN:
                form_driver(info->form, REQ_BEG_LINE);
                break;
            case ACTION_END:
                form_driver(info->form, REQ_END_LINE);
                break;
            case ACTION_CLEAR:
                form_driver(info->form, REQ_BEG_LINE);
                form_driver(info->form, REQ_CLR_EOL);
                break;
            case ACTION_DELETE:
                form_driver(info->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                form_driver(info->form, REQ_DEL_PREV);
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
        call_list_clear(window);
        // Reset filters on each key stroke
        filter_reset_calls();
    }

    // Validate all input data
    form_driver(info->form, REQ_VALIDATION);

    // Store dfilter input
    int field_len = strlen(field_buffer(info->fields[FLD_LIST_FILTER], 0));
    dfilter = malloc(field_len + 1);
    memset(dfilter, 0, field_len + 1);
    strncpy(dfilter, field_buffer(info->fields[FLD_LIST_FILTER], 0), field_len);
    // Trim any trailing spaces
    g_strstrip(dfilter);

    // Set display filter
    filter_set(FILTER_CALL_LIST, strlen(dfilter) ? dfilter : NULL);
    free(dfilter);

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
call_list_handle_menu_key(Window *window, int key)
{
    MENU *menu;
    int i;
    int action = -1;
    StorageSortOpts sort;
    enum sip_attr_id id;

    // Get panel information
    CallListInfo *info = call_list_info(window);
    g_return_val_if_fail(info != NULL, -1);

    menu = info->menu;

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
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
                id = sip_attr_from_name(item_name(current_item(info->menu)));
                if (sort.by == id) {
                    sort.asc = (sort.asc) ? false : true;
                } else {
                    sort.by = id;
                }
                storage_set_sort_options(sort);
                /* fall-thru */
            case ACTION_PREV_SCREEN:
                // Desactive sorting menu
                info->menu_active = 0;

                // Remove menu
                unpost_menu(info->menu);
                free_menu(info->menu);

                // Remove items
                for (i = 0; i < SIP_ATTR_COUNT; i++) {
                    if (!info->items[i])
                        break;
                    free_item(info->items[i]);
                }

                // Restore list position
                mvderwin(info->list_win, 5, 0);
                // Restore list window size
                wresize(info->list_win, window->height - 6, window->width);
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
    int rnpag_steps = setting_get_intvalue(SETTING_CL_SCROLLSTEP);
    Window *next_ui;
    SipCallGroup *group;
    int action = -1;
    SipCall *call;
    StorageSortOpts sort;

    CallListInfo *info = call_list_info(window);
    g_return_val_if_fail(info != NULL, -1);

    // Handle form key
    if (info->form_active)
        return call_list_handle_form_key(window, key);

    if (info->menu_active)
        return call_list_handle_menu_key(window, key);

    // Check actions for this key
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                call_list_move_down(window, 1);
                break;
            case ACTION_UP:
                call_list_move_up(window, 1);
                break;
            case ACTION_HNPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* fall-thru */
            case ACTION_NPAGE:
                // Next page => N key down strokes
                call_list_move_down(window, rnpag_steps);
                break;
            case ACTION_HPPAGE:
                rnpag_steps = rnpag_steps / 2;
                /* fall-thru */
            case ACTION_PPAGE:
                // Prev page => N key up strokes
                call_list_move_up(window, rnpag_steps);
                break;
            case ACTION_BEGIN:
                // Move to first list entry
                call_list_move(window, 0);
                break;
            case ACTION_END:
                call_list_move(window, g_ptr_array_len(info->dcalls) - 1);
                break;
            case ACTION_DISP_FILTER:
                // Activate Form
                call_list_form_activate(window, 1);
                break;
            case ACTION_SHOW_FLOW:
            case ACTION_SHOW_FLOW_EX:
            case ACTION_SHOW_RAW:
                // Check we have calls in the list
                if (g_ptr_array_len(info->dcalls) == 0)
                    break;

                // Create a new group of calls
                group = call_group_clone(info->group);

                // If not selected call, show current call flow
                if (call_group_count(info->group) == 0)
                    call_group_add(group, g_ptr_array_index(info->dcalls, info->cur_call));

                // Add xcall to the group
                if (action == ACTION_SHOW_FLOW_EX) {
                    call = g_ptr_array_index(info->dcalls, info->cur_call);
                    call_group_add_calls(group, call->xcalls);
                    group->callid = call->callid;
                }

                if (action == ACTION_SHOW_RAW) {
                    // Create a Call Flow panel
                    ncurses_create_window(PANEL_CALL_RAW);
                    call_raw_set_group(group);
                } else {
                    // Display current call flow (normal or extended)
                    ncurses_create_window(PANEL_CALL_FLOW);
                    call_flow_set_group(group);
                }
                break;
            case ACTION_SHOW_FILTERS:
                ncurses_create_window(PANEL_FILTER);
                break;
            case ACTION_SHOW_COLUMNS:
                ncurses_create_window(PANEL_COLUMN_SELECT);
                break;
            case ACTION_SHOW_STATS:
                ncurses_create_window(PANEL_STATS);
                break;
            case ACTION_SAVE:
                if (capture_sources_count(capture_manager()) > 1) {
                    dialog_run("Saving is not possible when multiple input sources are specified.");
                    break;
                }
                next_ui = ncurses_create_window(PANEL_SAVE);
                save_set_group(next_ui, info->group);
                break;
            case ACTION_CLEAR:
                // Clear group calls
                call_group_remove_all(info->group);
                break;
            case ACTION_CLEAR_CALLS:
                // Remove all stored calls
                storage_calls_clear();
                // Clear List
                call_list_clear(window);
                break;
            case ACTION_CLEAR_CALLS_SOFT:
                // Remove stored calls, keeping the currently displayed calls
                storage_calls_clear_soft();
                // Clear List
                call_list_clear(window);
                break;
            case ACTION_AUTOSCROLL:
                info->autoscroll = (info->autoscroll) ? 0 : 1;
                break;
            case ACTION_SHOW_SETTINGS:
                ncurses_create_window(PANEL_SETTINGS);
                break;
            case ACTION_SELECT:
                call = g_ptr_array_index(info->dcalls, info->cur_call);
                if (call_group_exists(info->group, call)) {
                    call_group_remove(info->group, call);
                } else {
                    call_group_add(info->group, call);
                }
                break;
            case ACTION_SORT_SWAP:
                // Change sort order
                sort = storage_sort_options();
                sort.asc = (sort.asc) ? false : true;
                storage_set_sort_options(sort);
                break;
            case ACTION_SORT_NEXT:
            case ACTION_SORT_PREV:
                call_list_select_sort_attribute(window);
                break;
            case ACTION_PREV_SCREEN:
                // Handle quit from this screen unless requested
                if (setting_enabled(SETTING_EXITPROMPT)) {
                    if (dialog_confirm("Confirm exit", "Are you sure you want to quit?", "Yes,No") == 0) {
                        ui_destroy(window);
                    }
                } else {
                    ui_destroy(window);
                }
                return KEY_HANDLED;
                break;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }

    // Disable autoscroll on some key pressed
    switch(action) {
        case ACTION_DOWN:
        case ACTION_UP:
        case ACTION_HNPAGE:
        case ACTION_HPPAGE:
        case ACTION_NPAGE:
        case ACTION_PPAGE:
        case ACTION_BEGIN:
        case ACTION_END:
        case ACTION_DISP_FILTER:
            info->autoscroll = 0;
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

/**
 * @brief Add a column the Call List
 *
 * This function will add a new column to the Call List panel
 * @todo Columns are not configurable yet.
 *
 * @param window UI structure pointer
 * @param id SIP call attribute id
 * @param attr SIP call attribute name
 * @param title SIP call attribute description
 * @param width Column Width
 * @return 0 if column has been successufly added to the list, -1 otherwise
 */
void
call_list_add_column(Window *window, enum sip_attr_id id, const char* attr,
                     const char *title, int width)
{
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    info->columns[info->columncnt].id = id;
    info->columns[info->columncnt].attr = attr;
    info->columns[info->columncnt].title = title;
    info->columns[info->columncnt].width = width;
    info->columncnt++;
}

void
call_list_clear(Window *window)
{
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    // Initialize structures
    info->scroll.pos = info->cur_call = 0;
    call_group_remove_all(info->group);

    // Clear Displayed lines
    werase(info->list_win);
    wnoutrefresh(info->list_win);
}
/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param window UI structure pointer
 */
static void
call_list_free(Window *window)
{
    CallListInfo *info = call_list_info(window);
    g_return_if_fail(info != NULL);

    // Deallocate forms data
    if (info->form) {
        unpost_form(info->form);
        free_form(info->form);
        free_field(info->fields[FLD_LIST_FILTER]);
    }

    // Deallocate group data
    call_group_free(info->group);
    g_ptr_array_free(info->dcalls, FALSE);

    // Deallocate panel windows
    delwin(info->list_win);
    g_free(info);

    ui_panel_destroy(window);
}

Window *
call_list_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_CALL_LIST;
    window->destroy     = call_list_free;
    window->redraw      = call_list_redraw;
    window->draw        = call_list_draw;
    window->resize      = call_list_resize;
    window->handle_key  = call_list_handle_key;
    window->help        = call_list_help;

    int i, attrid, collen;
    char option[80];
    const char *field, *title;

    // Create a new panel that fill all the screen
    window_init(window, getmaxy(stdscr), getmaxx(stdscr));

    // Initialize Call List specific data
    CallListInfo *info = g_malloc0(sizeof(CallListInfo));
    set_panel_userptr(window->panel, (void*) info);

    // Add configured columns
    for (i = 0; i < SIP_ATTR_COUNT; i++) {
        // Get column attribute name from options
        sprintf(option, "cl.column%d", i);
        if ((field = get_option_value(option))) {
            if ((attrid = sip_attr_from_name(field)) == -1)
                continue;
            // Get column width from options
            sprintf(option, "cl.column%d.width", i);
            if ((collen = get_option_int_value(option)) == -1)
                collen = sip_attr_get_width(attrid);
            // Get column title
            title = sip_attr_get_title(attrid);
            // Add column to the list
            call_list_add_column(window, attrid, field, title, collen);
        }
    }

    // Initialize the fields
    info->fields[FLD_LIST_FILTER] = new_field(1, window->width - 19, 3, 18, 0, 0);
    info->fields[FLD_LIST_COUNT] = NULL;

    // Create the form and post it
    info->form = new_form(info->fields);
    set_form_sub(info->form, window->win);

    // Form starts inactive
    call_list_form_activate(window, 0);
    info->menu_active = 0;

    // Calculate available printable area
    info->list_win = subwin(window->win, window->height - 6, window->width, 5, 0);
    info->scroll = ui_set_scrollbar(info->list_win, SB_VERTICAL, SB_LEFT);

    // Group of selected calls
    info->group = call_group_new();

    // Get current call list
    info->dcalls = g_ptr_array_new();

    // Set autoscroll default status
    info->autoscroll = setting_enabled(SETTING_CL_AUTOSCROLL);

    // Apply initial configured filters
    filter_method_from_setting(setting_get_value(SETTING_FILTER_METHODS));
    filter_payload_from_setting(setting_get_value(SETTING_FILTER_PAYLOAD));

    return window;
}
