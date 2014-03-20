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
 * @file ui_call_flow.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_flow.h
 *
 * @todo Code help screen. Please.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "ui_call_flow.h"
#include "ui_call_raw.h"
#include "option.h"

/***
 *
 * Some basic ascii art of this panel.
 *
 * +--------------------------------------------------------+
 * |                     Title                              |
 * +------------------------------+-------------------------+
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
 * +--------------------------------------------------------+
 * | Usefull hotkeys                                        |
 * +--------------------------------------------------------+
 *
 * Some values we have in mind, stored in the info structure of the panel:
 *
 *  - msgs_height: this represents the area's height where we can draw message arrows.
 *       It's calculated from the available screen height minus the space of title and
 *       hotkeys and headers.
 *  - msgs_width: this represents the area's width where we can draw message arrows.
 *       It's calculated from the available screen width minus the space of the borders
 *       and the raw message (if visible).
 *  - raw_width: this represents the raw message preview width if visible.
 */

PANEL *
call_flow_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    call_flow_info_t *info;

    // Create a new panel to fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));
    // Initialize Call List specific data
    info = malloc(sizeof(call_flow_info_t));
    memset(info, 0, sizeof(call_flow_info_t));
    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate available printable area for messages
    info->msgs_height = height /* top_bar */- 3 /* bottom_bar */- 3 /* Addrs headers */- 2;
    info->msgs_width = width /* borders */- 2;
    info->raw_width = 0; // calculated with the available space after drawing columns

    return panel;
}

call_flow_info_t *
call_flow_info(PANEL *panel)
{
    return (call_flow_info_t*) panel_userptr(panel);
}

void
call_flow_destroy(PANEL *panel)
{
    call_flow_info_t *info;
    // Hide the panel
    hide_panel(panel);
    // Free the panel information
    if ((info = call_flow_info(panel))) free(info);
    // Delete panel window
    delwin(panel_window(panel));
    // Delete panel
    del_panel(panel);
}

int
call_flow_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    int i;
    // Get panel information
    call_flow_info_t *info;

    // Check we have panel info
    if (!(info = call_flow_info(panel))) return -1;

    // Check if the owner of the message is in the displayed group
    for (i = 0; i < info->group->callcnt; i++) {
        if (info->group->calls[i] == msg->call) {
            if (!msg->parsed) msg_parse(msg);
            call_flow_column_add(panel, CALLID(msg), SRC(msg));
            call_flow_column_add(panel, CALLID(msg), DST(msg));
            return 0;
        }
    }

    return -1;
}

int
call_flow_draw(PANEL *panel)
{
    call_flow_info_t *info;
    sip_msg_t *msg;
    WINDOW *win;
    int height, width, cline;

    // Get panel information
    info = call_flow_info(panel);

    // Get window of main panel
    win = panel_window(panel);
    getmaxyx(win, height, width);
    werase(win);
    wattron(win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    title_foot_box(win);
    wattroff(win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    mvwprintw(win, height - 2, 2, "Q/Esc: Quit");
    mvwprintw(win, height - 2, 16, "F1: Help");
    mvwprintw(win, height - 2, 27, "x: Call-Flow");
    mvwprintw(win, height - 2, 42, "r: Call Raw");
    mvwprintw(win, height - 2, 57, "cC: Colours");
    mvwprintw(win, height - 2, 72, "t: Toggle Raw");
    mvwprintw(win, height - 2, 87, "s: Split callid");

    // Redraw columns
    call_flow_draw_columns(panel);

    // Let's start from the first displayed message (not the first in the call group)
    cline = 5;
    for (msg = info->first_msg; msg; msg = call_group_get_next_msg(info->group, msg)) {
        if (call_flow_draw_message(panel, msg, cline) != 0) {
            mvwaddch(win, 4 + info->msgs_height, 20, ACS_DARROW);
            break;
        }
        // One message fills 2 lines
        cline += 2;
    }

    // If there are only three columns, then draw the raw message on this panel
    //if ((20 + info->columns->colpos * 30 + get_option_int_value("cf.rawminwidth") < width)
	if (is_option_enabled("cf.forceraw")) {
        call_flow_draw_raw(panel, info->cur_msg);
    }

    return 0;

}

int
call_flow_draw_columns(PANEL *panel)
{
    call_flow_info_t *info;
    call_flow_column_t *column;
    WINDOW *win;
    sip_msg_t *msg;

    // Get panel information
    info = call_flow_info(panel);
    // Get window of main panel
    win = panel_window(panel);

    // Load columns if not loaded
    if (!info->columns) {
        for (msg = call_group_get_next_msg(info->group, NULL); msg; msg = call_group_get_next_msg(
                info->group, msg)) {
            call_flow_column_add(panel, CALLID(msg), SRC(msg));
            call_flow_column_add(panel, CALLID(msg), DST(msg));
        }
    }

    // Draw vertical columns lines
    for (column = info->columns; column; column = column->next) {
        mvwvline(win, 5, 20 + 30 * column->colpos, ACS_VLINE, info->msgs_height);
        mvwhline(win, 4, 10 + 30 * column->colpos, ACS_HLINE, 20);
        mvwaddch(win, 4, 20 + 30 * column->colpos, ACS_TTEE);
        mvwprintw(win, 3, 7 + 30 * column->colpos, "%22s", column->addr);
    }

    return 0;
}

int
call_flow_draw_message(PANEL *panel, sip_msg_t *msg, int cline)
{
    call_flow_info_t *info;
    WINDOW *win;
    const char *msg_time;
    const char *msg_callid;
    const char *msg_method;
    const char *msg_from;
    const char *msg_to;
    const char *msg_src;
    const char *msg_dst;
    char method[80];

    // Get panel information
    info = call_flow_info(panel);
    // Get the messages window
    win = panel_window(panel);
    // Get the current line in the win
    if (cline > info->msgs_height + 3) return 1;

    // Get message attributes
    msg_time = msg_get_attribute(msg, SIP_ATTR_TIME);
    msg_callid = msg_get_attribute(msg, SIP_ATTR_CALLID);
    msg_method = msg_get_attribute(msg, SIP_ATTR_METHOD);
    msg_from = msg_get_attribute(msg, SIP_ATTR_SIPFROM);
    msg_to = msg_get_attribute(msg, SIP_ATTR_SIPTO);
    msg_src = msg_get_attribute(msg, SIP_ATTR_SRC);
    msg_dst = msg_get_attribute(msg, SIP_ATTR_DST);

    // Print timestamp
    mvwprintw(win, cline, 2, "%s", msg_time);

    // Get Message method (include extra info)
    memset(method, 0, sizeof(method));
    sprintf(method, "%s", msg_method);
    if (msg_get_attribute(msg, SIP_ATTR_SDP))
        sprintf(method, "%s (SDP)", method);

    // Draw message type or status and line
    int msglen = strlen(method);
    if (msglen > 24) msglen = 24;

    // Get origin and destiny column
    call_flow_column_t *column1 = call_flow_column_get(panel, msg_callid, msg_src);
    call_flow_column_t *column2 = call_flow_column_get(panel, msg_callid, msg_dst);

    call_flow_column_t *tmp;
    if (column1->colpos > column2->colpos) {
        tmp = column1;
        column1 = column2;
        column2 = tmp;
    }

    int startpos = 20 + 30 * column1->colpos;
    int endpos = 20 + 30 * column2->colpos;
    int distance = abs(endpos - startpos) - 3;

    if (msg == info->cur_msg) wattron(win, A_BOLD);

    if (is_option_enabled("color.callid")) {
        wattron(win, COLOR_PAIR(call_group_color(info->group, msg->call)));
    } else {
        // Determine arrow color
        if (msg_get_attribute(msg, SIP_ATTR_REQUEST)) {
            wattron(win, COLOR_PAIR(OUTGOING_COLOR));
        } else {
            wattron(win, COLOR_PAIR(INCOMING_COLOR));
        }
    }

    mvwprintw(win, cline, startpos + 2, "%.*s", distance, "");
    mvwprintw(win, cline, startpos + distance / 2 - msglen / 2 + 2, "%.26s", method);
    mvwhline(win, cline + 1, startpos + 2, ACS_HLINE, distance);
    // Write the arrow at the end of the message (two arros if this is a retrans)
    if (!strcasecmp(msg_src, column1->addr)) {
        mvwaddch(win, cline + 1, endpos - 2, ACS_RARROW);
        if (msg_is_retrans(msg))
            mvwaddch(win, cline + 1, endpos - 3, ACS_RARROW);
    } else {
        mvwaddch(win, cline + 1, startpos + 2, ACS_LARROW);
        if (msg_is_retrans(msg))
            mvwaddch(win, cline + 1, startpos + 2, ACS_LARROW);
    }

    // Turn off colors
    wattroff(win, COLOR_PAIR(OUTGOING_COLOR));
    wattroff(win, COLOR_PAIR(INCOMING_COLOR));
    wattroff(win, COLOR_PAIR(CALLID1_COLOR));
    wattroff(win, COLOR_PAIR(CALLID2_COLOR));
    wattroff(win, A_BOLD);

    return 0;
}

int
call_flow_draw_raw(PANEL *panel, sip_msg_t *msg)
{
    call_flow_info_t *info;
    WINDOW *win, *raw_win;
    int raw_width, raw_height, raw_line, raw_char, column, line, height, width;

    // Get panel information
    info = call_flow_info(panel);

    // Get window of main panel
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate the raw data width (width - used columns for flow - vertical lines)
    raw_width = width - (31 + 30 * info->columns->colpos) - 2;
    // We can define a mininum size for rawminwidth
    if (raw_width < get_option_int_value("cf.rawminwidth")) {
        raw_width = get_option_int_value("cf.rawminwidth");
    }
    // We can configure an exact raw size
    if (get_option_int_value("cf.rawfixedwidth") != -1) {
        raw_width = get_option_int_value("cf.rawfixedwidth");
    } else {
        // Consider the calculated sized as the fixed one
        set_option_int_value("cf.rawfixedwidth", raw_width);
    }
    // Height of raw window is always available size minus 6 lines for header/footer
    raw_height = height - 3;

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
            wclear(raw_win);
        }
    } else {
        // Create the raw window of required size
        info->raw_win = raw_win = newwin(raw_height, raw_width, 0, 0);
    }

    // Draw raw box lines
    wattron(win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    mvwaddch(win, 2, width - raw_width - 2 , ACS_TTEE);
    mvwvline(win, 3, width - raw_width - 2 , ACS_VLINE, height - 6);
    mvwaddch(win, height - 3, width - raw_width - 2, ACS_BTEE);
    wattroff(win, COLOR_PAIR(DETAIL_BORDER_COLOR));

    // Print msg payload
    for (line = 0, raw_line = 0; raw_line < msg->plines; raw_line++) {
        // Add character by character
        for (column = 0, raw_char = 0; raw_char < strlen(msg->payload[raw_line]); raw_char++) {
            // Wrap at the end of the window
            if (column == raw_width) {
                line++;
                column = 0;
            }
            // Don't write out of the window
            if (line >= raw_height) break;
            // Put next character in position
            mvwaddch(raw_win, line, column++, msg->payload[raw_line][raw_char]);
        }
        // Done with this payload line, go to the next one
        line++;
    }

    // Copy the raw_win contents into the panel
    copywin(raw_win, win, 0, 0, 3, width - raw_width - 1, raw_height - 1, width - 2, 0);

    return 0;
}

int
call_flow_handle_key(PANEL *panel, int key)
{
    int i, rnpag_steps = 4, raw_width;
    call_flow_info_t *info = call_flow_info(panel);
    sip_msg_t *next = NULL, *prev = NULL;
    ui_t *next_panel;
    sip_call_group_t *group;

    // Sanity check, this should not happen
    if (!info) return -1;

    switch (key) {
    case KEY_DOWN:
        // Check if there is a call below us
        if (!(next = call_group_get_next_msg(info->group, info->cur_msg))) break;
        info->cur_msg = next;
        info->cur_line += 2;
        // If we are out of the bottom of the displayed list
        // refresh it starting in the next call
        if (info->cur_line >= info->msgs_height) {
            info->first_msg = call_group_get_next_msg(info->group, info->first_msg);
            info->cur_line -= 2;
        }
        break;
    case KEY_UP:
        // FIXME We start searching from the fist one
        // FIXME This wont work well with a lot of msg
        while ((next = call_group_get_next_msg(info->group, next))) {
            if (next == info->cur_msg) break;
            prev = next;
        }
        // We're at the first message already
        if (!prev) break;
        info->cur_msg = prev;
        info->cur_line -= 2;
        if (info->cur_line <= 0) {
            info->first_msg = info->cur_msg;
            info->cur_line += 2;
        }
        break;
    case KEY_NPAGE:
        // Next page => N key down strokes
        for (i = 0; i < rnpag_steps; i++)
            call_flow_handle_key(panel, KEY_DOWN);
        break;
    case KEY_PPAGE:
        // Prev page => N key up strokes
        for (i = 0; i < rnpag_steps; i++)
            call_flow_handle_key(panel, KEY_UP);
        break;
    case 'x':
        wclear(panel_window(panel));
        // KEY_X , Display current call flow
        if (info->group->callcnt == 1) {
            group = call_group_create();
            call_group_add(group, info->group->calls[0]);
            call_group_add(group, call_get_xcall(info->group->calls[0]));
            call_flow_set_group(group);
        } else {
            group = call_group_create();
            call_group_add(group, info->group->calls[0]);
            call_flow_set_group(group);
        }
        break;
    case 'r':
        // KEY_R, display current call in raw mode
        next_panel = ui_create(ui_find_by_type(RAW_PANEL));
        // TODO
        call_raw_set_group(info->group);
        wait_for_input(next_panel);
        break;
    case '0':
        raw_width = get_option_int_value("cf.rawfixedwidth");
        if (raw_width - 2 > 1) {
            set_option_int_value("cf.rawfixedwidth", raw_width - 2);
        }
        break;
    case '9':
        raw_width = get_option_int_value("cf.rawfixedwidth");
        if (raw_width + 2 < COLS - 1) {
            set_option_int_value("cf.rawfixedwidth", raw_width + 2);
        }
        break;
    case 't':
        set_option_value("cf.forceraw", is_option_enabled("cf.forceraw") ? "off" : "on");
        break;
    case 's':
        set_option_value("cf.splitcallid", is_option_enabled("cf.splitcallid") ? "off" : "on");
        // Force columns reload
        info->columns = NULL;
        break;
    case 10:
        // KEY_ENTER, display current message in raw mode
        next_panel = ui_create(ui_find_by_type(RAW_PANEL));
        // TODO
        call_raw_set_group(info->group);
        call_raw_set_msg(info->cur_msg);
        wait_for_input(next_panel);
        break;
    default:
        return key;
    }

    return 0;
}

int
call_flow_help(PANEL *panel)
{
    WINDOW *help_win;
    PANEL *help_panel;

    // Create a new panel and show centered
    help_win = newwin(20, 65, (LINES - 20) / 2, (COLS - 65) / 2);
    help_panel = new_panel(help_win);

    // Set the window title
    mvwprintw(help_win, 1, 18, "Call Flow Help");

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
    mvwprintw(help_win, 3, 2, "This window shows the messages from a call and its relative");
    mvwprintw(help_win, 4, 2, "ordered by sent or received time.");
    mvwprintw(help_win, 5, 2, "This panel is mosly used when capturing at proxy systems that");
    mvwprintw(help_win, 6, 2, "manages incoming and outgoing request between calls.");
    wattroff(help_win, COLOR_PAIR(HELP_COLOR));

    // A list of available keys in this window
    mvwprintw(help_win, 8, 2, "Available keys:");
    mvwprintw(help_win, 9, 2, "F1          Show this screen.");
    mvwprintw(help_win, 10, 2, "q/Esc       Go back to Call list window.");
    mvwprintw(help_win, 11, 2, "c           Turn on/off window colours.");
    mvwprintw(help_win, 12, 2, "C           Turn on/off colour by Call-ID.");
    mvwprintw(help_win, 13, 2, "Up/Down     Move to previous/next message.");
    mvwprintw(help_win, 14, 2, "x           Show call-flow (Normal) for original call.");
    mvwprintw(help_win, 15, 2, "r           Show original call messages in raw mode.");

    // Press any key to close
    wgetch(help_win);
    update_panels();
    doupdate();

    return 0;
}

int
call_flow_set_group(sip_call_group_t *group)
{
    PANEL *panel;
    call_flow_info_t *info;

    if (!(panel = ui_get_panel(ui_find_by_type(DETAILS_PANEL)))) return -1;

    if (!(info = call_flow_info(panel))) return -1;

    info->group = group;
    info->cur_msg = info->first_msg = call_group_get_next_msg(group, NULL);
    info->cur_line = 1;
    info->columns = NULL;

    return 0;
}

void
call_flow_column_add(PANEL *panel, const char *callid, const char *addr)
{
    call_flow_info_t *info;
    call_flow_column_t *column;
    int colpos = 0;

    if (!(info = call_flow_info(panel))) return;

    if (call_flow_column_get(panel, callid, addr)) return;

    column = info->columns;
    while (column) {
        if (!strcasecmp(addr, column->addr) && column->colpos != 0 && !column->callid2) {
            column->callid2 = callid;
            return;
        }
        column = column->next;
    }

    if (info->columns) colpos = info->columns->colpos + 1;

    column = malloc(sizeof(call_flow_column_t));
    memset(column, 0, sizeof(call_flow_column_t));
    column->callid = callid;
    column->addr = addr;
    column->colpos = colpos;
    column->next = info->columns;
    info->columns = column;
}

call_flow_column_t *
call_flow_column_get(PANEL *panel, const char *callid, const char *addr)
{
    call_flow_info_t *info;
    call_flow_column_t *columns;

    if (!(info = call_flow_info(panel))) return NULL;

    columns = info->columns;
    while (columns) {
        if (!strcasecmp(addr, columns->addr)) {
            if (is_option_enabled("cf.splitcallid")) return columns;
            if (columns->callid && !strcasecmp(callid, columns->callid)) return columns;
            if (columns->callid2 && !strcasecmp(callid, columns->callid2)) return columns;
        }
        columns = columns->next;
    }
    return NULL;
}

