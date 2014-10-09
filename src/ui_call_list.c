/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
 **
 ** Copyright (C) 2013 Ivan Alonso (Kaian)
 ** Copyright (C) 2013 Irontec SL. All rights reserved.
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
 * @todo Recode help screen. Please.
 * @todo Replace calls structure this for a iterator at sip.h
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "option.h"
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_raw.h"

PANEL *
call_list_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width, i, colpos, collen, colcnt, attrid;
    call_list_info_t *info;
    char option[80];
    const char *field, *title;

    // Create a new panel that fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));
    // Initialize Call List specific data
    info = malloc(sizeof(call_list_info_t));
    memset(info, 0, sizeof(call_list_info_t));
    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Add configured columns
    colcnt = get_option_int_value("cl.columns");
    for (i = 0; i < colcnt; i++) {
        // Get column attribute name
        sprintf(option, "cl.column%d", i);
        field = get_option_value(option);
        // Get column width
        sprintf(option, "cl.column%d.width", i);
        collen = get_option_int_value(option);
        if (!(attrid = sip_attr_from_name(field))) continue;
        title = sip_attr_get_description(attrid);
        // Add column to the list
        call_list_add_column(panel, attrid, field, title, collen);
    }

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate available printable area
    info->list_win = subwin(win, height - 6, width, 5, 0);
    info->group = call_group_create();

    // Draw a header with APP_NAME and APP_LONG_DESC - FIXME /calculate a properly middle :P
    // mvwprintw(win, 1, (width - 45) / 2, "sngrep - SIP message interface for ngrep");
    mvwprintw(win, 1, 2, "Current Mode: %s", get_option_value("sngrep.mode"));
    if (get_option_value("sngrep.file"))
        mvwprintw(win, 1, width - strlen (get_option_value("sngrep.file")) - 11, "Filename: %s", get_option_value("sngrep.file"));
    mvwprintw(win, 3, 2, "Filter: ");

    // Draw the footer of the window
    call_list_draw_footer(panel);

    wattron(win, COLOR_PAIR(KEYBINDINGS_ACTION));
    // Draw panel title
    mvwprintw(win, 0, 0,  "%*s", width, "");
    mvwprintw(win, 0, (width - 45) / 2, "sngrep - SIP message interface for ngrep");

    // Draw columns titles
    mvwprintw(win, 4, 0,  "%*s", width, "");
    for (colpos = 6, i = 0; i < info->columncnt; i++) {
        // Get current column width
        collen = info->columns[i].width;

        // Check if the column will fit in the remaining space of the screen
        if (colpos + collen >= width) break;
        mvwprintw(win, 4, colpos, sip_attr_get_description(info->columns[i].id));
        colpos += collen + 1;
    }
    wattroff(win, COLOR_PAIR(KEYBINDINGS_ACTION));

    // Return the created panel
    return panel;
}

void
call_list_destroy(PANEL *panel)
{
    call_list_info_t *info;

    // Hide the panel
    hide_panel(panel);

    // Free its status data
    if ((info = (call_list_info_t*) panel_userptr(panel))) free(info);

    // Finally free the panel memory
    del_panel(panel);
}

void
call_list_draw_footer(PANEL *panel)
{
    const char *keybindings[] = {
        "F1",       "Help",
        "Esc/Q",    "Quit",
        "Enter",    "Show",
        "X",        "Extended",
        "C",        "Colours",
        "R",        "Raw",
        "F",        "Filter",
        "S",        "Save" };

    draw_keybindings(panel, keybindings, 16);
}

int
call_list_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    //@todo alway redraw this screen on new messages
    return 0;
}

int
call_list_draw(PANEL *panel)
{
    int height, width, i, colpos, collen, cline = 0;
    struct sip_call *call;
    int callcnt;
    const char *call_attr, *ouraddr;

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info) return -1;

    // Get window of call list panel
    WINDOW *win = info->list_win;
    getmaxyx(win, height, width);

    // Get available calls counter (we'll use it here a couple of times)
    if (!(callcnt = sip_calls_count())) return 0;

    // If no active call, use the fist one (if exists)
    if (!info->first_call && call_get_next(NULL)) {
        info->cur_call = info->first_call = call_get_next(NULL);
        info->cur_line = info->first_line = 1;
    }

    // Fill the call list
    for (call = info->first_call; call; call = call_get_next(call)) {
        // Stop if we have reached the bottom of the list
        if (cline == height) break;

        // We only print calls with messages (In fact, all call should have msgs)
        if (!call_msg_count(call)) continue;

        // TODO Useless feature :|
        if ((ouraddr = get_option_value("address"))) {
            if (!strcasecmp(ouraddr, call_get_attribute(call, SIP_ATTR_SRC))) {
                wattron(win, COLOR_PAIR(OUTGOING_COLOR));
            } else if (!strcasecmp(ouraddr, call_get_attribute(call, SIP_ATTR_DST))) {
                wattron(win, COLOR_PAIR(INCOMING_COLOR));
            }
        }

        // Show bold selected rows
        if (call_group_exists(info->group, call)) {
            wattron(win, A_BOLD);
            wattron(win, COLOR_PAIR(SELECTED_COLOR));
        }

        // Highlight active call
        if (call == info->cur_call) {
            wattron(win, COLOR_PAIR(HIGHLIGHT_COLOR));
        }

        // Set current line background
        mvwprintw(win, cline, 0, "%*s", width - 2, "");
        // Set current line selection box
        mvwprintw(win, cline, 2, call_group_exists(info->group, call)?"[*]":"[ ]");

        // Print requested columns
        for (colpos = 6, i = 0; i < info->columncnt; i++) {
            // Get current column width
            collen = info->columns[i].width;
            // Check if the column will fit in the remaining space of the screen
            if (colpos + collen >= width) break;
            // Get call attribute for current column
            if ((call_attr = call_get_attribute(call, info->columns[i].id))) {
                mvwprintw(win, cline, colpos, "%.*s", collen, call_attr);
            }
            colpos += collen + 1;
        }
        wattroff(win, COLOR_PAIR(SELECTED_COLOR));
        wattroff(win, COLOR_PAIR(HIGHLIGHT_COLOR));
        wattroff(win, A_BOLD);
        cline++;
    }

    // Draw scrollbar to the right
    draw_vscrollbar(win, info->first_line, callcnt, false);
    wrefresh(info->list_win);

    return 0;
}

int
call_list_handle_key(PANEL *panel, int key)
{
    int i, height, width, rnpag_steps = get_option_int_value("cl.scrollstep");
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    ui_t *next_panel;
    sip_call_group_t *group;

    // Sanity check, this should not happen
    if (!info) return -1;

    // Get window of call list panel
    WINDOW *win = info->list_win;
    getmaxyx(win, height, width);

    switch (key) {
    case KEY_DOWN:
        // Check if there is a call below us
        if (!info->cur_call || !call_get_next(info->cur_call)) break;
        info->cur_call = call_get_next(info->cur_call);
        info->cur_line++;
        // If we are out of the bottom of the displayed list
        // refresh it starting in the next call
        if (info->cur_line > height) {
            info->first_call = call_get_next(info->first_call);
            info->first_line++;
            info->cur_line = height;
        }
        break;
    case KEY_UP:
        // Check if there is a call above us
        if (!info->cur_call || !call_get_prev(info->cur_call)) break;
        info->cur_call = call_get_prev(info->cur_call);
        info->cur_line--;
        // If we are out of the top of the displayed list
        // refresh it starting in the previous (in fact current) call
        if (info->cur_line <= 0) {
            info->first_call = info->cur_call;
            info->first_line--;
            info->cur_line = 1;
        }
        break;
    case KEY_NPAGE:
        // Next page => N key down strokes
        for (i = 0; i < rnpag_steps; i++)
            call_list_handle_key(panel, KEY_DOWN);
        break;
    case KEY_PPAGE:
        // Prev page => N key up strokes
        for (i = 0; i < rnpag_steps; i++)
            call_list_handle_key(panel, KEY_UP);
        break;
    case 10:
        if (!info->cur_call) return -1;
        // KEY_ENTER , Display current call flow
        next_panel = ui_create(ui_find_by_type(DETAILS_PANEL));
        if (info->group->callcnt) {
            group = info->group;
        } else {
            if (!info->cur_call) return -1;
            group = call_group_create();
            call_group_add(group, info->cur_call);
        }
        call_flow_set_group(group);
        wait_for_input(next_panel);
        break;
    case 'x':
        // KEY_X , Display current call flow (extended)
        next_panel = ui_create(ui_find_by_type(DETAILS_PANEL));
        if (info->group->callcnt) {
            group = info->group;
        } else {
            if (!info->cur_call) return -1;
            group = call_group_create();
            call_group_add(group, info->cur_call);
            call_group_add(group, call_get_xcall(info->cur_call));
        }
        call_flow_set_group(group);
        wait_for_input(next_panel);
        break;
    case 'r':
    case 'R':
        // KEY_R , Display current call flow (extended)
        next_panel = ui_create(ui_find_by_type(RAW_PANEL));
        if (info->group->callcnt) {
            group = info->group;
        } else {
            if (!info->cur_call) return -1;
            group = call_group_create();
            call_group_add(group, info->cur_call);
        }
        call_raw_set_group(group);
        wait_for_input(next_panel);
        break;
    case 'f':
    case 'F':
        // KEY_F, Display filter panel
        next_panel = ui_create(ui_find_by_type(FILTER_PANEL));
        wait_for_input(next_panel);
        call_list_filter_update(panel);
        break;
    case 's':
    case 'S':
        if (!is_option_disabled("sngrep.tmpfile")) {
            // KEY_S, Display save panel
            next_panel = ui_create(ui_find_by_type(SAVE_PANEL));
            wait_for_input(next_panel);
        }
        break;
    case KEY_F(5):
        // Remove all stored calls
        sip_calls_clear();
        // Clear List
        call_list_clear(panel);
        break;
    case ' ':
        if (!info->cur_call) return -1;
        if (call_group_exists(info->group, info->cur_call)) {
            call_group_del(info->group, info->cur_call);
        } else {
            call_group_add(info->group, info->cur_call);
        }
        break;
    case 'q':
    case 27: /* KEY_ESC */
        // Handle quit from this screen unless requested
        if (!is_option_enabled("cl.noexitprompt")) {
            return call_list_exit_confirm(panel);
        }
        break;
    default:
        return key;
    }

    return 0;
}

int
call_list_help(PANEL *panel)
{
    WINDOW *help_win;
    PANEL *help_panel;
    int height, width;

    // Create a new panel and show centered
    height = 23; width = 65;
    help_win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    help_panel = new_panel(help_win);

    // Set the window title
    mvwprintw(help_win, 1, 25, "Call List Help");

    // Write border and boxes around the window
    wattron(help_win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    box(help_win, 0, 0);
    mvwhline(help_win, 2, 1, ACS_HLINE, width-2);
    mvwhline(help_win, 7, 1, ACS_HLINE, width-2);
    mvwhline(help_win, height-3, 1, ACS_HLINE, width-2);
    mvwaddch(help_win, 2, 0, ACS_LTEE);
    mvwaddch(help_win, 7, 0, ACS_LTEE);
    mvwaddch(help_win, height-3, 0, ACS_LTEE);
    mvwaddch(help_win, 2, 64, ACS_RTEE);
    mvwaddch(help_win, 7, 64, ACS_RTEE);
    mvwaddch(help_win, height-3, 64, ACS_RTEE);

    // Set the window footer (nice blue?)
    mvwprintw(help_win, height-2, 20, "Press any key to continue");

    // Some brief explanation abotu what window shows
    wattron(help_win, COLOR_PAIR(HELP_COLOR));
    mvwprintw(help_win, 3, 2, "This windows show the list of parsed calls from a pcap file ");
    mvwprintw(help_win, 4, 2, "(Offline) or a live capture with ngrep (Online).");
    mvwprintw(help_win, 5, 2, "You can configure the columns shown in this screen and some");
    mvwprintw(help_win, 6, 2, "static filters using sngreprc resource file.");
    wattroff(help_win, COLOR_PAIR(HELP_COLOR));

    // A list of available keys in this window
    mvwprintw(help_win, 8, 2, "Available keys:");
    mvwprintw(help_win, 10, 2, "F1          Show this screen.");
    mvwprintw(help_win, 11, 2, "q/Esc       Exit sngrep.");
    mvwprintw(help_win, 12, 2, "c           Turn on/off window colours.");
    mvwprintw(help_win, 13, 2, "Up/Down     Move to previous/next call.");
    mvwprintw(help_win, 14, 2, "Enter       Show selected call-flow.");
    mvwprintw(help_win, 15, 2, "x           Show selected call-flow (Extended) if available.");
    mvwprintw(help_win, 16, 2, "r           Show selected call messages in raw mode.");
    mvwprintw(help_win, 17, 2, "p           Pause. Stop parsing captured packages");
    mvwprintw(help_win, 18, 2, "f/F         Show filter options");
    mvwprintw(help_win, 19, 2, "s/S         Save captured packages to a file.");

    // Press any key to close
    wgetch(help_win);
    update_panels();
    doupdate();

    return 0;
}

int
call_list_exit_confirm(PANEL *panel)
{
    WINDOW *exit_win;
    PANEL *exit_panel;
    int c;
    // Initial exit status
    int exit = get_option_int_value("cl.defexitbutton") ;

    // Create a new panel and show centered
    exit_win = newwin(8, 40, (LINES - 8) / 2, (COLS - 40) / 2);
    exit_panel = new_panel(exit_win);
    keypad(exit_win, TRUE);

    // Set the window title
    mvwprintw(exit_win, 1, 13, "Confirm exit");

    // Write border and boxes around the window
    wattron(exit_win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    box(exit_win, 0, 0);
    mvwhline(exit_win, 2, 1, ACS_HLINE, 40);
    mvwhline(exit_win, 5, 1, ACS_HLINE, 40);
    mvwaddch(exit_win, 2, 0, ACS_LTEE);
    mvwaddch(exit_win, 5, 0, ACS_LTEE);
    mvwaddch(exit_win, 2, 39, ACS_RTEE);
    mvwaddch(exit_win, 5, 39, ACS_RTEE);

    // Exit confirmation message message
    wattron(exit_win, COLOR_PAIR(HELP_COLOR));
    mvwprintw(exit_win, 3, 2, "Are you sure you want to quit?");
    wattroff(exit_win, COLOR_PAIR(HELP_COLOR));

    for(;;) {
        // A list of available keys in this window
        if (exit) wattron(exit_win, A_REVERSE);
        mvwprintw(exit_win, 6, 10, "[  Yes  ]");
        wattroff(exit_win, A_REVERSE);
        if (!exit) wattron(exit_win, A_REVERSE);
        mvwprintw(exit_win, 6, 20, "[  No   ]");
        wattroff(exit_win, A_REVERSE);

        update_panels();
        doupdate();

        c = wgetch(exit_win);
        switch (c) {
        case KEY_RIGHT:
            exit = 0;
            break;
        case KEY_LEFT:
            exit = 1;
            break;
       case 9:
            exit = (exit)?0:1;
            break;
        case 10:
            // If we return ESC, we let ui_manager to handle this
            // key and exit sngrep gracefully
            return (exit)?27:0;
        }
    }

    return 0;
}

int
call_list_add_column(PANEL *panel, enum sip_attr_id id, const char* attr, const char *title,
        int width)
{
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info) return 1;

    info->columns[info->columncnt].id = id;
    info->columns[info->columncnt].attr = attr;
    info->columns[info->columncnt].title = title;
    info->columns[info->columncnt].width = width;
    info->columncnt++;
    return 0;
}

void
call_list_filter_update(PANEL *panel)
{

    WINDOW *win = panel_window(panel);

    // Clear list
    call_list_clear(panel);
}

void
call_list_clear(PANEL *panel)
{
    WINDOW *win = panel_window(panel);

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info) return;

    // Initialize structures
    info->first_call = info->cur_call = NULL;
    info->first_line = info->cur_line = 0;
    info->group->callcnt = 0;

    // Clear Displayed lines
    werase(info->list_win);
}

