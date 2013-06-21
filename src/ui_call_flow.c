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
#include <string.h>
#include "ui_manager.h"
#include "ui_call_flow.h"

/**
 * @brief Call flow status information
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
PANEL *call_flow_create()
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
    wattron(win,COLOR_PAIR(DETAIL_BORDER_COLOR));
    title_foot_box(win);
    mvwaddch(win, 2, 61, ACS_TTEE);
    mvwvline(win, 3, 61, ACS_VLINE, height - 6);
    mvwaddch(win, 4, 0, ACS_LTEE);
    mvwhline(win, 4, 1, ACS_HLINE, 61);
    mvwaddch(win, 4, 61, ACS_RTEE);
    mvwaddch(win, height-3, 61, ACS_BTEE);
    wattroff(win,COLOR_PAIR(DETAIL_BORDER_COLOR));

    // Callflow box title
    mvwprintw(win, 3, 30, "Call Flow");
    mvwhline(win, 6, 11, ACS_HLINE, 20);
    mvwhline(win, 6, 40, ACS_HLINE, 20);
    mvwaddch(win, 6, 20, ACS_TTEE);
    mvwaddch(win, 6, 50, ACS_TTEE);

    mvwprintw(win, height - 2, 2,  "Q/Esc: Quit");
    mvwprintw(win, height - 2, 16, "F1: Show help");
    mvwprintw(win, height - 2, 32, "X: Show Extendend");

	return panel;
}

int call_flow_draw(PANEL *panel)
{
    int i, height, width, cline, cseq, msgcnt, startpos = 7;
	sip_msg_t *msg;

	// Get panel information
	call_flow_info_t *info = (call_flow_info_t*) panel_userptr(panel);

    // This panel only makes sense with a selected call
    if (!info->call) return 1;

    // Get window of main panel
    WINDOW *win = panel_window(panel);
	msgcnt = get_n_msgs(info->call);

    // Get data from first message
    const char *from = info->call->messages->ip_from;

    // Get window size
    getmaxyx(win, height, width);

    // Window title
    mvwprintw(win, 1, (width - 45) / 2, "Call Details for %s", info->call->callid);
    // Hosts and lines in callflow
    mvwprintw(win, 5, 7, "%22s", info->call->messages->ip_from);
    mvwprintw(win, 5, 37, "%22s", info->call->messages->ip_to);

    // Make the vertical lines for messages (2 lines per message + extra space)
    mvwvline(win, 7, 20, ACS_VLINE, info->linescnt);
    mvwvline(win, 7, 50, ACS_VLINE, info->linescnt);

	for (cline = startpos, msg = info->first_msg; msg; msg = get_next_msg(info->call, msg)) {
		// Print messages with differents CSeq separed by one line
		//if (cseq != msg->cseq){ 
		//	cline++;
		//	cseq = msg->cseq;
		//}

		// Check if there are still 2 spaces for this message in the list
		if (cline >= info->linescnt + startpos ) {
			// Draw 2 arrow to show there are more messages
            mvwaddch(win, info->linescnt + startpos - 1, 20, ACS_DARROW);
            mvwaddch(win, info->linescnt + startpos - 1, 50, ACS_DARROW);
			break;
		}

        // Print timestamp
        mvwprintw(win, cline, 2, "%s", msg->time);

        if (msg == info->cur_msg) wattron(win,A_REVERSE);

        // Determine the message direction
        if (!strcmp(msg->ip_from, from)) {
            wattron(win, COLOR_PAIR(OUTGOING_COLOR));
            mvwhline(win, cline + 1, 22, ACS_HLINE, 26);
            mvwaddch(win, cline + 1, 47, ACS_RARROW);
        } else {
            wattron(win, COLOR_PAIR(INCOMING_COLOR));
            mvwhline(win, cline + 1, 22, ACS_HLINE, 26);
            mvwaddch(win, cline + 1, 22, ACS_LARROW);
        }

        // Draw message type or status and line
        mvwprintw(win, cline, 22, "%26s", "");
        int msglen = strlen(msg->type);
        if (msglen > 24) msglen = 24;
        mvwprintw(win, cline, 22 + (24 - msglen) / 2, "%.24s", msg->type);

        // Turn off colors
        wattroff(win, COLOR_PAIR(OUTGOING_COLOR));
        wattroff(win, COLOR_PAIR(INCOMING_COLOR));
        wattroff(win, A_REVERSE);

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

int call_flow_handle_key(PANEL *panel, int key) 
{
    int i, rnpag_steps = 4;
    call_flow_info_t *info = (call_flow_info_t*) panel_userptr(panel);
	sip_msg_t *next = NULL, *prev = NULL;

    // Sanity check, this should not happen
    if (!info) return -1;

    switch (key) {
    case KEY_DOWN:
        // Check if there is a call below us
        if (!(next = get_next_msg(info->call, info->cur_msg)))
            break;
        info->cur_msg = next;
        info->cur_line += 2;
        // If we are out of the bottom of the displayed list
        // refresh it starting in the next call
        if (info->cur_line > info->linescnt) {
            info->first_msg = get_next_msg(info->call, info->first_msg);
            info->cur_line = info->linescnt;
        }
        break;
    case KEY_UP:
		// FIXME We start searching from the fist one
		// FIXME This wont work well with a lot of msg
		while ((prev = get_next_msg(info->call, prev))){
			if (prev->next == info->cur_msg)
				break;
		}	
		// We're at the first message already
		if (!prev) break;
		info->cur_msg = prev;
		info->cur_line -= 2;
		if ( info->cur_line <= 0 ){
			info->first_msg = info->cur_msg;
			info->cur_line = 1;	
		}
        break;
    case KEY_NPAGE:
        // Next page => N key down strokes 
        for (i=0; i < rnpag_steps; i++)
            call_flow_handle_key(panel, KEY_DOWN);
        break;
    case KEY_PPAGE:
        // Prev page => N key up strokes
        for (i=0; i < rnpag_steps; i++)
            call_flow_handle_key(panel, KEY_UP);
        break;
    case 10:
        break;
    default:
        return -1;
    }

    return 0;

}


int call_flow_help(PANEL *panel)
{
	return 0;
}

int call_flow_set_call(sip_call_t *call) {
	ui_panel_t *flow_panel;
	PANEL *panel;
	call_flow_info_t *info;

	if (!call) 
		return -1;

	if (!(flow_panel = ui_find_element_by_type(DETAILS_PANEL)))
		return -1;
	
	if (!(panel = flow_panel->panel))
		return -1;

	if (!(info = (call_flow_info_t*) panel_userptr(panel)))
		return -1;

	info->call = call;
	info->cur_msg = info->first_msg = call->messages;
	info->cur_line = 1;
	return 0;
}
