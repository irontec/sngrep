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
 * @file ui_call_flow_ex.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_flow_ex.h
 *
 * @todo Code help screen. Please.
 * @todo Maybe we should merge this and Call-Flow into one panel
 *
 */
#include <stdlib.h>
#include <string.h>
#include "ui_call_flow.h"
#include "ui_call_flow_ex.h"
#include "ui_call_raw.h"

PANEL *
call_flow_ex_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width;
    call_flow_ex_info_t *info;

    // Create a new panel to fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));
    // Initialize Call List specific data 
    info = malloc(sizeof(call_flow_ex_info_t));
    memset(info, 0, sizeof(call_flow_ex_info_t));
    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate available printable area
    info->linescnt = height - 10;

    // Window borders
    wattron(win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    title_foot_box(win);
    mvwaddch(win, 2, 91, ACS_TTEE);
    mvwvline(win, 3, 91, ACS_VLINE, height - 6);
    mvwaddch(win, 4, 0, ACS_LTEE);
    mvwhline(win, 4, 1, ACS_HLINE, 91);
    mvwaddch(win, 4, 91, ACS_RTEE);
    mvwaddch(win, height - 3, 91, ACS_BTEE);
    wattroff(win, COLOR_PAIR(DETAIL_BORDER_COLOR));

    // Callflow box title
    mvwprintw(win, 3, 40, "Call Flow Extended");
    mvwhline(win, 6, 11, ACS_HLINE, 20);
    mvwhline(win, 6, 40, ACS_HLINE, 20);
    mvwhline(win, 6, 70, ACS_HLINE, 20);
    mvwaddch(win, 6, 20, ACS_TTEE);
    mvwaddch(win, 6, 50, ACS_TTEE);
    mvwaddch(win, 6, 80, ACS_TTEE);

    // Make the vertical lines for messages (2 lines per message + extra space)
    mvwvline(win, 7, 20, ACS_VLINE, info->linescnt);
    mvwvline(win, 7, 50, ACS_VLINE, info->linescnt);
    mvwvline(win, 7, 80, ACS_VLINE, info->linescnt);

    mvwprintw(win, height - 2, 2, "Q/Esc: Quit");
    mvwprintw(win, height - 2, 16, "F1: Help");
    mvwprintw(win, height - 2, 27, "x: Call-Flow");
    mvwprintw(win, height - 2, 42, "r: Call Raw");
    mvwprintw(win, height - 2, 57, "c: Colours");

    return panel;
}

void
call_flow_ex_destroy(PANEL *panel)
{
    call_flow_ex_info_t *info;

    // Hide the panel
    hide_panel(panel);

    // Free the panel information
    if ((info = (call_flow_ex_info_t*) panel_userptr(panel))) free(info);

    // Delete panel window
    delwin(panel_window(panel));
    // Delete panel
    del_panel(panel);
}

int
call_flow_ex_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    // Get panel information
    call_flow_ex_info_t *info;

    // Check we have panel info
    if (!(info = (call_flow_ex_info_t*) panel_userptr(panel))) return -1;

    // If this message belongs to first call
    if (msg->call == info->call) return 0;

    // If this message belongs to second call
    if (msg->call == info->call2) return 0;

    return -1;
}

int
call_flow_ex_draw(PANEL *panel)
{
    int i, height, width, cline, msgcnt, startpos = 7;
    sip_msg_t *msg;
    sip_call_t *call, *call2;

    // Get panel information
    call_flow_ex_info_t *info = (call_flow_ex_info_t*) panel_userptr(panel);
    call = info->call;
    call2 = info->call2;

    // This panel only makes sense with a selected call with extended xcallid
    if (!call || !call2) return 1;

    // Get window of main panel
    WINDOW *win = panel_window(panel);
    msgcnt = call_msg_count(call) + call_msg_count(call2);

    // Get data from first message
    const char *from, *to, *via;
    const char *callid1, *callid2;

    const struct sip_msg *first = call_get_next_msg_ex(call, NULL);
    if (!strcmp(call->callid, first->call->callid)) {
        from = call_get_attribute(call, "src");
        via = call_get_attribute(call, "dst");
        to = call_get_attribute(call2, "dst");
        callid1 = call_get_attribute(call, "callid");
        callid2 = call_get_attribute(call2, "callid");
    } else {
        from = call_get_attribute(call2, "src");
        via = call_get_attribute(call2, "dst");
        to = call_get_attribute(call, "dst");
        callid1 = call_get_attribute(call2, "callid");
        callid2 = call_get_attribute(call, "callid");
    }

    // Get window size
    getmaxyx(win, height, width);

    // Window title
    mvwprintw(win, 1, (width - 45) / 2, "Call Details for %s -> %s", call->callid, call2->callid);
    // Hosts and lines in callflow
    mvwprintw(win, 5, 7, "%22s", from);
    mvwprintw(win, 5, 37, "%22s", via);
    mvwprintw(win, 5, 67, "%22s", to);

    for (cline = startpos, msg = info->first_msg; msg; msg = call_get_next_msg_ex(info->call, msg)) {
        // Check if there are still 2 spaces for this message in the list
        if (cline >= info->linescnt + startpos) {
            // Draw 2 arrow to show there are more messages
            mvwaddch(win, info->linescnt + startpos - 1, 20, ACS_DARROW);
            mvwaddch(win, info->linescnt + startpos - 1, 50, ACS_DARROW);
            break;
        }

        // Print timestamp
        mvwprintw(win, cline, 2, "%s", msg->time);

        if (msg == info->cur_msg) wattron(win, A_REVERSE);

        // Draw message type or status and line
        int msglen = strlen(msg->type);
        if (msglen > 24) msglen = 24;

        // Determine the message direction
        if (!strcmp(msg->call->callid, callid1) && !strcmp(msg->ip_from, from)) {
            wattron(win, COLOR_PAIR(OUTGOING_COLOR));
            mvwprintw(win, cline, 22, "%26s", "");
            mvwprintw(win, cline, 22 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(win, cline + 1, 22, ACS_HLINE, 26);
            mvwaddch(win, cline + 1, 47, ACS_RARROW);
        } else if (!strcmp(msg->call->callid, callid1) && !strcmp(msg->ip_to, from)) {
            wattron(win, COLOR_PAIR(INCOMING_COLOR));
            mvwprintw(win, cline, 22, "%26s", "");
            mvwprintw(win, cline, 22 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(win, cline + 1, 22, ACS_HLINE, 26);
            mvwaddch(win, cline + 1, 22, ACS_LARROW);
        } else if (!strcmp(msg->call->callid, callid2) && !strcmp(msg->ip_from, via)) {
            wattron(win, COLOR_PAIR(OUTGOING_COLOR));
            mvwprintw(win, cline, 52, "%26s", "");
            mvwprintw(win, cline, 54 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(win, cline + 1, 52, ACS_HLINE, 26);
            mvwaddch(win, cline + 1, 77, ACS_RARROW);
        } else {
            wattron(win, COLOR_PAIR(INCOMING_COLOR));
            mvwprintw(win, cline, 52, "%26s", "");
            mvwprintw(win, cline, 54 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(win, cline + 1, 52, ACS_HLINE, 26);
            mvwaddch(win, cline + 1, 52, ACS_LARROW);
        }

        // Turn off colors
        wattroff(win, COLOR_PAIR(OUTGOING_COLOR));
        wattroff(win, COLOR_PAIR(INCOMING_COLOR));
        wattroff(win, A_REVERSE);

        // One message fills 2 lines
        cline += 2;
    }

    // Clean the message area
    for (cline = 3, i = 0; i < info->linescnt + 4; i++)
        mvwprintw(win, cline++, 92, "%*s", width - 93, "");

    // Print the message payload in the right side of the screen
    for (cline = 3, i = 0; i < info->cur_msg->plines && i < info->linescnt + 4; i++)
        mvwprintw(win, cline++, 92, "%.*s", width - 93, info->cur_msg->payload[i]);

    return 0;

}

int
call_flow_ex_handle_key(PANEL *panel, int key)
{
    int i, rnpag_steps = 4;
    call_flow_ex_info_t *info = (call_flow_ex_info_t*) panel_userptr(panel);
    sip_msg_t *next = NULL, *prev = NULL;
    ui_t *next_panel;

    // Sanity check, this should not happen
    if (!info) return -1;

    switch (key) {
    case KEY_DOWN:
        // Check if there is a call below us
        if (!(next = call_get_next_msg_ex(info->call, info->cur_msg))) break;
        info->cur_msg = next;
        info->cur_line += 2;
        // If we are out of the bottom of the displayed list
        // refresh it starting in the next call
        if (info->cur_line > info->linescnt) {
            info->first_msg = call_get_next_msg_ex(info->call, info->first_msg);
            info->cur_line = info->linescnt;
        }
        break;
    case KEY_UP:
        // FIXME We start searching from the fist one
        // FIXME This wont work well with a lot of msg
        while ((next = call_get_next_msg_ex(info->call, next))) {
            if (next == info->cur_msg) break;
            prev = next;
        }
        // We're at the first message already
        if (!prev) break;
        info->cur_msg = prev;
        info->cur_line -= 2;
        if (info->cur_line <= 0) {
            info->first_msg = info->cur_msg;
            info->cur_line = 1;
        }
        break;
    case KEY_NPAGE:
        // Next page => N key down strokes 
        for (i = 0; i < rnpag_steps; i++)
            call_flow_ex_handle_key(panel, KEY_DOWN);
        break;
    case KEY_PPAGE:
        // Prev page => N key up strokes
        for (i = 0; i < rnpag_steps; i++)
            call_flow_ex_handle_key(panel, KEY_UP);
        break;
    case 'x':
        if (!info->call) return -1;
        // KEY_ENTER , Display current call flow
        next_panel = ui_create(ui_find_by_type(DETAILS_PANEL));
        call_flow_set_call(info->call);
        ui_set_replace(ui_find_by_panel(panel), next_panel);
        break;
    case 'r':
        // KEY_R, display current call in raw mode
        next_panel = ui_create(ui_find_by_type(RAW_PANEL));
        call_raw_set_call(info->call);
        wait_for_input(next_panel);
    default:
        return -1;
    }

    return 0;
}

int
call_flow_ex_help(PANEL *panel)
{
    return 0;
}

int
call_flow_ex_set_call(sip_call_t *call)
{
    PANEL *panel;
    call_flow_ex_info_t *info;

    if (!call) return -1;

    if (!(panel = ui_get_panel(ui_find_by_type(DETAILS_EX_PANEL)))) return -1;

    if (!(info = (call_flow_ex_info_t*) panel_userptr(panel))) return -1;

    info->call = call;
    info->call2 = call_get_xcall(call);
    info->cur_msg = info->first_msg = call_get_next_msg_ex(call, NULL);
    info->cur_line = 1;
    return 0;
}
