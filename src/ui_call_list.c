/**************************************************************************
 **
 **  sngrep - Ncurses ngrep interface for SIP
 **
 **   Copyright (C) 2013 Ivan Alonso (Kaian)
 **   Copyright (C) 2013 Irontec SL. All rights reserved.
 **
 **   This program is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   This program is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_flow_ex.h"
#include "ui_call_raw.h"
#include "sip.h"

// FIXME create a getter function for this at sip.c
extern struct sip_call *calls;

PANEL *
call_list_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width, i, colpos;
    call_list_info_t *info;

    // Create a new panel that fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));
    // Initialize Call List specific data 
    info = malloc(sizeof(call_list_info_t));
    memset(info, 0, sizeof(call_list_info_t));
    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Set default columns. One day this will be configurable
    call_list_add_column(panel, 0, "From SIP", 40);
    call_list_add_column(panel, 1, "To SIP", 40);
    call_list_add_column(panel, 2, "Msg", 5);
    call_list_add_column(panel, 3, "From", 22);
    call_list_add_column(panel, 4, "To", 22);
    call_list_add_column(panel, 5, "Starting", 15);

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate available printable area
    info->linescnt = height - 11;

    // Draw a box arround the window
    title_foot_box(win);

    // Draw a header with APP_NAME and APP_LONG_DESC - FIXME /calculate a properly middle :P
    mvwprintw(win, 1, (width - 45) / 2, "sngrep - SIP message interface for ngrep");
    mvwprintw(win, 3, 2, "Current Mode: %s", "Online");
    mvwaddch(win, 5, 0, ACS_LTEE);
    mvwhline(win, 5, 1, ACS_HLINE, width - 2);
    mvwaddch(win, 5, width - 1, ACS_RTEE);
    mvwaddch(win, 7, 0, ACS_LTEE);
    mvwhline(win, 7, 1, ACS_HLINE, width - 2);
    mvwaddch(win, 7, width - 1, ACS_RTEE);
    mvwprintw(win, height - 2, 2, "Q/Esc: Quit");
    mvwprintw(win, height - 2, 16, "F1: Help");
    mvwprintw(win, height - 2, 27, "X: Call-Flow Extended");

    // Draw columns titles
    for (colpos = 5, i = 0; i < info->columncnt; i++) {
        // Check if the column will fit in the remaining space of the screen
        if (colpos + info->columns[i].width >= width) break;
        mvwprintw(win, 6, colpos, info->columns[i].title);
        colpos += info->columns[i].width;
    }
    // Return the created panel
    return panel;
}

int
call_list_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    return 0;
}

int
call_list_draw(PANEL *panel)
{
    int height, width, i, colpos, collen, startline = 8;
    struct sip_call *call;
    int callcnt;

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info) return -1;

    // Get available calls counter (we'll use it here a couple of times)
    if (!(callcnt = get_n_calls())) return 0;

    // If no active call, use the fist one (if exists)
    if (!info->first_call && calls) {
        info->cur_call = info->first_call = calls;
        info->cur_line = info->first_line = 1;
    }

    // Get window of call list panel
    WINDOW *win = panel_window(panel);
    getmaxyx(win, height, width);

    // Fill the call list
    int cline = startline;
    for (call = info->first_call; call; call = call->next) {
        // Stop if we have reached the bottom of the list
        if (cline >= info->linescnt + startline) break;

        // We only print calls with messages (In fact, all call should have msgs)
        if (!get_n_msgs(call)) continue;

        // Highlight active call
        if (call == info->cur_call) {
            wattron(win, COLOR_PAIR(HIGHLIGHT_COLOR));
        }
        mvwprintw(win, cline, 4, "%*s", width - 6, "");

        // Print requested columns
        for (colpos = 5, i = 0; i < info->columncnt; i++) {
            collen = info->columns[i].width;
            // Check if the column will fit in the remaining space of the screen
            if (colpos + collen >= width) break;

            // Display each column with it's data
            switch (info->columns[i].id) {
            case 0:
                mvwprintw(win, cline, colpos, "%.*s", collen, call->messages->sip_from);
                break;
            case 1:
                mvwprintw(win, cline, colpos, "%.*s", collen, call->messages->sip_to);
                break;
            case 2:
                mvwprintw(win, cline, colpos, "%d", get_n_msgs(call));
                break;
            case 3:
                mvwprintw(win, cline, colpos, "%.*s", collen, call->messages->ip_from);
                break;
            case 4:
                mvwprintw(win, cline, colpos, "%.*s", collen, call->messages->ip_to);
                break;
            case 5:
                mvwprintw(win, cline, colpos, "%.*s", collen, call->messages->type);
                break;
            }
            colpos += collen;
        }
        wattroff(win, COLOR_PAIR(HIGHLIGHT_COLOR));
        cline++;
    }

    // Clean scroll information
    mvwprintw(win, startline, 2, " ");
    mvwprintw(win, startline + info->linescnt - 2, 2, "   ");
    mvwprintw(win, startline + info->linescnt - 1, 2, " ");

    // Update the scroll information
    if (info->first_line > 1) mvwaddch(win, startline, 2, ACS_UARROW);
    if (callcnt > info->first_line + info->linescnt) {
        mvwaddch(win, startline + info->linescnt - 1, 2, ACS_DARROW);
    }

    // Set the current line % if we have more calls that available lines
    if (callcnt > info->linescnt) mvwprintw(win, startline + info->linescnt - 2, 1, "%2d%%",
            (info->first_line + info->cur_line) * 100 / callcnt);
    return 0;
}

int
call_list_handle_key(PANEL *panel, int key)
{
    int i, rnpag_steps = 10;
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    ui_t *next_panel;

    // Sanity check, this should not happen
    if (!info) return -1;

    switch (key) {
    case KEY_DOWN:
        // Check if there is a call below us
        if (!info->cur_call || !info->cur_call->next) break;
        info->cur_call = info->cur_call->next;
        info->cur_line++;
        // If we are out of the bottom of the displayed list
        // refresh it starting in the next call
        if (info->cur_line > info->linescnt) {
            info->first_call = info->first_call->next;
            info->first_line++;
            info->cur_line = info->linescnt;
        }
        break;
    case KEY_UP:
        // Check if there is a call above us
        if (!info->cur_call || !info->cur_call->prev) break;
        info->cur_call = info->cur_call->prev;
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
        call_flow_set_call(info->cur_call);
        wait_for_input(next_panel);
        break;
    case 'x':
        if (!info->cur_call) return -1;
        // KEY_ENTER , Display current call flow
        next_panel = ui_create(ui_find_by_type(DETAILS_EX_PANEL));
        call_flow_ex_set_call(info->cur_call);
        wait_for_input(next_panel);
        break;
    case 'r':
    case 'R':
        if (!info->cur_call) return -1;
        // KEY_R, display current call in raw mode
        next_panel = ui_create(ui_find_by_type(RAW_PANEL));
        call_raw_set_call(info->cur_call);
        wait_for_input(next_panel);
        break;

    default:
        return -1;
    }

    return 0;
}

int
call_list_help(PANEL * ppanel)
{
    int cline = 1;
    int width, height;

    PANEL *panel = new_panel(newwin(20, 50, LINES / 4, COLS / 4));
    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get window size
    getmaxyx(win, height, width);

    box(win, 0, 0);
    mvwprintw(win, cline++, 15, "Help - Dialogs Window ");
    mvwaddch(win, cline, 0, ACS_LTEE);
    mvwhline(win, cline, 1, ACS_HLINE, width - 2);
    mvwaddch(win, cline++, width - 1, ACS_RTEE);
    wattron(win, COLOR_PAIR(HELP_COLOR));
    mvwprintw(win, cline, 3, "F1/h:");
    mvwprintw(win, cline + 1, 3, "ESC/q:");
    mvwprintw(win, cline + 2, 3, "Up:");
    mvwprintw(win, cline + 3, 3, "Down:");
    mvwprintw(win, cline + 4, 3, "Enter:");
    wattroff(win, COLOR_PAIR(HELP_COLOR));
    mvwprintw(win, cline, 15, "Show this screen :)");
    mvwprintw(win, cline + 1, 15, "Exit sngrep");
    mvwprintw(win, cline + 2, 15, "Select Previous dialog");
    mvwprintw(win, cline + 3, 15, "Select Next dialog");
    mvwprintw(win, cline + 4, 15, "Show dialog details");
    return 0;
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

int
call_list_add_column(PANEL *panel, int id, char *title, int width)
{
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info) return -1;
    info->columns[info->columncnt].id = id;
    info->columns[info->columncnt].title = title;
    info->columns[info->columncnt].width = width;
    info->columncnt++;
    return 0;
}
