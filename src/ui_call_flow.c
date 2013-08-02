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
 * @todo Maybe we should merge this and Call-Flow Extended into one panel
 *
 */
#include <stdlib.h>
#include <string.h>
#include "ui_call_flow.h"
#include "ui_call_flow_ex.h"
#include "ui_call_raw.h"

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

    // Calculate available printable area
    info->linescnt = height - 10;

    // Window borders
    wattron(win, COLOR_PAIR(DETAIL_BORDER_COLOR));
    title_foot_box(win);
    mvwaddch(win, 2, 61, ACS_TTEE);
    mvwvline(win, 3, 61, ACS_VLINE, height - 6);
    mvwaddch(win, 4, 0, ACS_LTEE);
    mvwhline(win, 4, 1, ACS_HLINE, 61);
    mvwaddch(win, 4, 61, ACS_RTEE);
    mvwaddch(win, height - 3, 61, ACS_BTEE);
    wattroff(win, COLOR_PAIR(DETAIL_BORDER_COLOR));

    // Callflow box title
    mvwprintw(win, 3, 30, "Call Flow");
    mvwhline(win, 6, 11, ACS_HLINE, 20);
    mvwhline(win, 6, 40, ACS_HLINE, 20);
    mvwaddch(win, 6, 20, ACS_TTEE);
    mvwaddch(win, 6, 50, ACS_TTEE);

    mvwprintw(win, height - 2, 2, "Q/Esc: Quit");
    mvwprintw(win, height - 2, 16, "F1: Help");
    mvwprintw(win, height - 2, 27, "x: Call-Flow Extended");
    mvwprintw(win, height - 2, 52, "r: Call Raw");
    mvwprintw(win, height - 2, 67, "c: Colours");

    return panel;
}

void
call_flow_destroy(PANEL *panel)
{
    call_flow_info_t *info;

    // Hide the panel
    hide_panel(panel);

    // Free the panel information
    if ((info = (call_flow_info_t*) panel_userptr(panel))) free(info);

    // Delete panel window
    delwin(panel_window(panel));
    // Delete panel
    del_panel(panel);
}

int
call_flow_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    // Get panel information
    call_flow_info_t *info;

    // Check we have panel info
    if (!(info = (call_flow_info_t*) panel_userptr(panel))) return -1;

    // If this message belongs to first call
    if (msg->call == info->call) return 0;

    return -1;
}

int
call_flow_draw(PANEL *panel)
{
    int i, height, width, cline, msgcnt, startpos = 7;
    sip_msg_t *msg;
    //int cseq;

    // Get panel information
    call_flow_info_t *info = (call_flow_info_t*) panel_userptr(panel);

    // This panel only makes sense with a selected call
    if (!info->call) return 1;

    // Get window of main panel
    WINDOW *win = panel_window(panel);
    msgcnt = call_msg_count(info->call);

    // Get data from first message
    const char *from = call_get_attribute(info->call, SIP_ATTR_SRC);

    // Get window size
    getmaxyx(win, height, width);

    // Window title
    mvwprintw(win, 1, (width - 45) / 2, "Call Details for %s", call_get_attribute(info->call,
            SIP_ATTR_CALLID));
    // Hosts and lines in callflow
    mvwprintw(win, 5, 7, "%22s", call_get_attribute(info->call, SIP_ATTR_SRC));
    mvwprintw(win, 5, 37, "%22s", call_get_attribute(info->call, SIP_ATTR_DST));

    // Make the vertical lines for messages (2 lines per message + extra space)
    mvwvline(win, 7, 20, ACS_VLINE, info->linescnt);
    mvwvline(win, 7, 50, ACS_VLINE, info->linescnt);

    for (cline = startpos, msg = info->first_msg; msg; msg = call_get_next_msg(info->call, msg)) {
        // Print messages with differents CSeq separed by one line
        // XXX Not for now, makes harder to control selected message and scroll
        //if (cseq != msg->cseq){ 
        //    cline++;
        //    cseq = msg->cseq;
        //}

        // Check if there are still 2 spaces for this message in the list
        if (cline >= info->linescnt + startpos - 1) {
            // Draw 2 arrow to show there are more messages
            mvwaddch(win, info->linescnt + startpos - 1, 20, ACS_DARROW);
            mvwaddch(win, info->linescnt + startpos - 1, 50, ACS_DARROW);
            break;
        }

        // Print timestamp
        mvwprintw(win, cline, 2, "%s", msg_get_attribute(msg, SIP_ATTR_TIME));

        if (msg == info->cur_msg) wattron(win, A_BOLD);

        // Determine arrow color
        if (msg_get_attribute(msg, SIP_ATTR_REQUEST)) {
            wattron(win, COLOR_PAIR(OUTGOING_COLOR));
        } else {
            wattron(win, COLOR_PAIR(INCOMING_COLOR));
        }

        // Determine the message direction
        if (!strcmp(msg_get_attribute(msg, SIP_ATTR_SRC), from)) {
            mvwhline(win, cline + 1, 22, ACS_HLINE, 26);
            mvwaddch(win, cline + 1, 47, ACS_RARROW);
        } else {
            mvwhline(win, cline + 1, 22, ACS_HLINE, 26);
            mvwaddch(win, cline + 1, 22, ACS_LARROW);
        }

        // Draw message type or status and line
        mvwprintw(win, cline, 22, "%26s", "");
        int msglen = strlen(msg_get_attribute(msg, SIP_ATTR_METHOD));
        if (msglen > 24) msglen = 24;
        mvwprintw(win, cline, 22 + (24 - msglen) / 2, "%.24s", msg_get_attribute(msg, SIP_ATTR_METHOD));

        // Turn off colors
        wattroff(win, COLOR_PAIR(OUTGOING_COLOR));
        wattroff(win, COLOR_PAIR(INCOMING_COLOR));
        wattroff(win, A_BOLD);

        // One message fills 2 lines
        cline += 2;
    }

    // Clean the message area
    for (cline = 3, i = 0; i < info->linescnt + 4; i++)
        mvwprintw(win, cline++, 62, "%*s", width - 63, "");

    // Print the message payload in the right side of the screen
    for (cline = 3, i = 0; i < info->cur_msg->plines && i < info->linescnt + 4; i++)
        mvwprintw(win, cline++, 62, "%.*s", width - 63, info->cur_msg->payload[i]);

    return 0;

}

int
call_flow_handle_key(PANEL *panel, int key)
{
    int i, rnpag_steps = 4;
    call_flow_info_t *info = (call_flow_info_t*) panel_userptr(panel);
    sip_msg_t *next = NULL, *prev = NULL;
    ui_t *next_panel;

    // Sanity check, this should not happen
    if (!info) return -1;

    switch (key) {
    case KEY_DOWN:
        // Check if there is a call below us
        if (!(next = call_get_next_msg(info->call, info->cur_msg))) break;
        info->cur_msg = next;
        info->cur_line += 2;
        // If we are out of the bottom of the displayed list
        // refresh it starting in the next call
        if (info->cur_line >= info->linescnt) {
            info->first_msg = call_get_next_msg(info->call, info->first_msg);
            info->cur_line -= 2;
        }
        break;
    case KEY_UP:
        // FIXME We start searching from the fist one
        // FIXME This wont work well with a lot of msg
        while ((prev = call_get_next_msg(info->call, prev))) {
            if (prev->next == info->cur_msg) break;
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
        if (!info->call) return -1;
        if (!call_get_xcall(info->call)) return -1;
        // KEY_ENTER , Display current call flow
        next_panel = ui_create(ui_find_by_type(DETAILS_EX_PANEL));
        call_flow_ex_set_call(info->call);
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
call_flow_help(PANEL *panel)
{
    WINDOW *help_win;
    PANEL *help_panel;

    // Create a new panel and show centered
    help_win = newwin(20, 65, (LINES - 20) / 2, (COLS - 65) / 2);
    help_panel = new_panel(help_win);

    // Set the window title
    mvwprintw(help_win, 1, 25, "Call Flow Help");

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
    mvwprintw(help_win, 3, 2, "This window shows the messages from a single call ordered");
    mvwprintw(help_win, 4, 2, "by sent or received time. There is also a preview of");
    mvwprintw(help_win, 5, 2, "selected message in the right side panel. If you need to");
    mvwprintw(help_win, 6, 2, "copy message content, use raw panel instead (see keys below).");
    wattroff(help_win, COLOR_PAIR(HELP_COLOR));

    // A list of available keys in this window
    mvwprintw(help_win, 8,  2, "Available keys:");
    mvwprintw(help_win, 10, 2, "F1          Show this screen.");
    mvwprintw(help_win, 11, 2, "q/Esc       Go back to Call list window.");
    mvwprintw(help_win, 12, 2, "c           Turn on/off window colours.");
    mvwprintw(help_win, 13, 2, "Up/Down     Move to previous/next message.");
    mvwprintw(help_win, 14, 2, "x           Show call-flow (Extended) for this call.");
    mvwprintw(help_win, 15, 2, "r           Show call messages in raw mode.");

    // Press any key to close
    wgetch(help_win);
    update_panels();
    doupdate();

    return 0;
}

int
call_flow_set_call(sip_call_t *call)
{
    ui_t *flow_panel;
    PANEL *panel;
    call_flow_info_t *info;

    if (!call) return -1;

    if (!(flow_panel = ui_find_by_type(DETAILS_PANEL))) return -1;

    if (!(panel = flow_panel->panel)) return -1;

    if (!(info = (call_flow_info_t*) panel_userptr(panel))) return -1;

    info->call = call;
    info->cur_msg = info->first_msg = call_get_next_msg(call, NULL);
    info->cur_line = 1;
    return 0;
}
