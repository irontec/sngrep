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
    info->linescnt = height - 11;
    info->group = call_group_create();

    // Draw a box arround the window
    title_foot_box(win);

    // Draw a header with APP_NAME and APP_LONG_DESC - FIXME /calculate a properly middle :P
    mvwprintw(win, 1, (width - 45) / 2, "sngrep - SIP message interface for ngrep");
    mvwprintw(win, 3, 2, "Current Mode: %s", get_option_value("running.mode"));
    if (get_option_value("running.file")) {
        mvwprintw(win, 4, 2, "Filename: %s", get_option_value("running.file"));
    }
    mvwaddch(win, 5, 0, ACS_LTEE);
    mvwhline(win, 5, 1, ACS_HLINE, width - 2);
    mvwaddch(win, 5, width - 1, ACS_RTEE);
    mvwaddch(win, 7, 0, ACS_LTEE);
    mvwhline(win, 7, 1, ACS_HLINE, width - 2);
    mvwaddch(win, 7, width - 1, ACS_RTEE);
    mvwprintw(win, height - 2, 2, "Q/Esc: Quit");
    mvwprintw(win, height - 2, 16, "F1: Help");
    mvwprintw(win, height - 2, 27, "x: Call-Flow Extended");
    mvwprintw(win, height - 2, 52, "r: Call Raw");
    mvwprintw(win, height - 2, 67, "c: Colours");

    // Draw columns titles
    for (colpos = 6, i = 0; i < info->columncnt; i++) {
        // Get current column width
        collen = info->columns[i].width;

        // Check if the column will fit in the remaining space of the screen
        if (colpos + collen >= width) break;
        mvwprintw(win, 6, colpos, sip_attr_get_description(info->columns[i].id));
        colpos += collen + 1;
    }
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

int
call_list_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    //@todo alway redraw this screen on new messages
    return 0;
}

int
call_list_draw(PANEL *panel)
{
    int height, width, i, colpos, collen, startline = 8;
    struct sip_call *call;
    int callcnt;
    const char *call_attr, *ouraddr;

    // Get panel info
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    if (!info) return -1;

    // Get available calls counter (we'll use it here a couple of times)
    if (!(callcnt = sip_calls_count())) return 0;

    // If no active call, use the fist one (if exists)
    if (!info->first_call && call_get_next(NULL)) {
        info->cur_call = info->first_call = call_get_next(NULL);
        info->cur_line = info->first_line = 1;
    }

    // Get window of call list panel
    WINDOW *win = panel_window(panel);
    getmaxyx(win, height, width);

    // Fill the call list
    int cline = startline;
    for (call = info->first_call; call; call = call_get_next(call)) {
        // Stop if we have reached the bottom of the list
        if (cline >= info->linescnt + startline) break;

        // We only print calls with messages (In fact, all call should have msgs)
        if (!call_msg_count(call)) continue;
        if ((ouraddr = get_option_value("address"))) {
            if (!strcasecmp(ouraddr, call_get_attribute(call, SIP_ATTR_SRC))) {
                wattron(win, COLOR_PAIR(OUTGOING_COLOR));
            } else if (!strcasecmp(ouraddr, call_get_attribute(call, SIP_ATTR_DST))) {
                wattron(win, COLOR_PAIR(INCOMING_COLOR));
            }
        }

        if (call_group_exists(info->group, call)) {
            wattron(win, A_BOLD);
            wattron(win, COLOR_PAIR(SELECTED_COLOR));
        }

        // Highlight active call
        if (call == info->cur_call) {
            wattron(win, COLOR_PAIR(HIGHLIGHT_COLOR));
        }
        mvwprintw(win, cline, 5, "%*s", width - 6, "");

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

    // Clean scroll information
    mvwprintw(win, startline, 2, " ");
    mvwprintw(win, startline + info->linescnt - 2, 1, "   ");
    mvwprintw(win, startline + info->linescnt - 1, 2, " ");

    // Update the scroll information
    if (info->first_line > 1) mvwaddch(win, startline, 2, ACS_UARROW);
    if (callcnt > info->first_line + info->linescnt) {
        mvwaddch(win, startline + info->linescnt - 1, 2, ACS_DARROW);
    }

    // Set the current line % if we have more calls that available lines
    int percentage = (info->first_line + info->cur_line) * 100 / callcnt;
    if (callcnt > info->linescnt && percentage < 100) {
        mvwprintw(win, startline + info->linescnt - 2, 1, "%2d%%", percentage);
    }
    return 0;
}

int
call_list_handle_key(PANEL *panel, int key)
{
    int i, rnpag_steps = get_option_int_value("cl.scrollstep");
    call_list_info_t *info = (call_list_info_t*) panel_userptr(panel);
    ui_t *next_panel;
    sip_call_group_t *group;

    // Sanity check, this should not happen
    if (!info) return -1;

    switch (key) {
    case KEY_DOWN:
        // Check if there is a call below us
        if (!info->cur_call || !call_get_next(info->cur_call)) break;
        info->cur_call = call_get_next(info->cur_call);
        info->cur_line++;
        // If we are out of the bottom of the displayed list
        // refresh it starting in the next call
        if (info->cur_line > info->linescnt) {
            info->first_call = call_get_next(info->first_call);
            info->first_line++;
            info->cur_line = info->linescnt;
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
        // KEY_X , Display current call flow (extended)
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
    case ' ':
        if (!info->cur_call) return -1;
        if (call_group_exists(info->group, info->cur_call)) {
            call_group_del(info->group, info->cur_call);
        } else {
            call_group_add(info->group, info->cur_call);
        }
        break;
    default:
        return -1;
    }

    return 0;
}

int
call_list_help(PANEL *panel)
{
    WINDOW *help_win;
    PANEL *help_panel;

    // Create a new panel and show centered
    help_win = newwin(20, 65, (LINES - 20) / 2, (COLS - 65) / 2);
    help_panel = new_panel(help_win);

    // Set the window title
    mvwprintw(help_win, 1, 25, "Call List Help");

    // Write border and boxes around the window
    wattron(help_win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    box(help_win, 0, 0);
    mvwhline(help_win, 2, 1, ACS_HLINE, 63);
    mvwhline(help_win, 7, 1, ACS_HLINE, 63);
    mvwhline(help_win, 17, 1, ACS_HLINE, 63);
    mvwaddch(help_win, 2, 0, ACS_LTEE);
    mvwaddch(help_win, 7, 0, ACS_LTEE);
    mvwaddch(help_win, 17, 0, ACS_LTEE);
    mvwaddch(help_win, 2, 64, ACS_RTEE);
    mvwaddch(help_win, 7, 64, ACS_RTEE);
    mvwaddch(help_win, 17, 64, ACS_RTEE);

    // Set the window footer (nice blue?)
    mvwprintw(help_win, 18, 20, "Press any key to continue");

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

    // Press any key to close
    wgetch(help_win);
    update_panels();
    doupdate();

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
