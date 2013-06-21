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
#include "ui_call_flow_ex.h"

extern struct sip_call *active_call;

PANEL *call_flow_ex_create()
{
	PANEL *panel = new_panel(newwin(LINES, COLS, 0, 0));
	return panel;
}

int call_flow_ex_draw(PANEL *panel)
{
    int w, h, fw, fh, rw, rh, ph;
    int msgcnt = 0;
    int padpos, highlight, entries;

    // This panel only makes sense with a selected call
    if (!active_call) return 1;

    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get other call data
    struct sip_call *call = active_call;
    struct sip_call *call2 = get_ex_call(call);

    if (!call2) return 1;

    const char *from, *to, *via;
    const char *callid1, *callid2;

    const struct sip_msg *first = get_next_msg_ex(call, NULL);
    if (!strcmp(call->callid, first->call->callid)) {
        from = call->messages->ip_from;
        via = call->messages->ip_to;
        to = call2->messages->ip_to;
        callid1 = call->callid;
        callid2 = call2->callid;
    } else {
        from = call2->messages->ip_from;
        via = call2->messages->ip_to;
        to = call->messages->ip_to;
        callid1 = call2->callid;
        callid2 = call->callid;
    }

    // Get window size
    getmaxyx(win, h, w);
    // Get flow size
    fw = 95;
    fh = h - 3 - 3;
    // Get the raw size
    rw = w - fw - 2;
    rh = h - 3 - 3;

    // Window borders
    wattron(win,COLOR_PAIR(DETAIL_BORDER_COLOR));
    title_foot_box(win);
    mvwaddch(win, 2, fw, ACS_TTEE);
    mvwvline(win, 3, fw, ACS_VLINE, fh);
    mvwaddch(win, 4, 0, ACS_LTEE);
    mvwhline(win, 4, 1, ACS_HLINE, fw);
    mvwaddch(win, 4, fw, ACS_RTEE);
    mvwaddch(win, 3+fh, fw, ACS_BTEE);
    wattroff(win,COLOR_PAIR(DETAIL_BORDER_COLOR));

    // Window title
    mvwprintw(win, 1, (w - 45) / 2, "Call Details for %s", call->callid);
    // Callflow box title
    mvwprintw(win, 3, 40, "Call Flow Extended");

    // Hosts and lines in callflow
    mvwprintw(win, 5, 13, "%-22s", from);
    mvwprintw(win, 5, 42, "%-22s", via);
    mvwprintw(win, 5, 72, "%-22s", to);
    mvwhline(win, 6, 11, ACS_HLINE, 20);
    mvwhline(win, 6, 40, ACS_HLINE, 20);
    mvwhline(win, 6, 70, ACS_HLINE, 20);
    mvwaddch(win, 6, 20, ACS_TTEE);
    mvwaddch(win, 6, 50, ACS_TTEE);
    mvwaddch(win, 6, 80, ACS_TTEE);
    mvwprintw(win, h - 2, 2,
            "Q: Quit    C: Toggle color    F: Show raw messages     X: Show Call-Flow");

    int msgcntex = get_n_msgs(call) + get_n_msgs(call2);

    // Make the pad long enough to contain all messages and some extra space
    WINDOW *flow_pad = newpad(fh + msgcntex * 2, fw);
    mvwvline(flow_pad, 0, 20, ACS_VLINE, fh+msgcntex*2);
    mvwvline(flow_pad, 0, 50, ACS_VLINE, fh+msgcntex*2);
    mvwvline(flow_pad, 0, 80, ACS_VLINE, fh+msgcntex*2);

    // Make a pad for sip message
    WINDOW *raw_pad = newpad(rh, rw);

    struct sip_msg *msg = NULL;

    int cline = 0;
    while ((msg = get_next_msg_ex(call, msg))) {
        msgcnt++;

        // Set raw content for selected text
        if (msgcnt == highlight) {
            int raw_line = 0, pcount = 0;
            for (raw_line = 0; raw_line < msg->plines; raw_line++, pcount++) {
                mvwprintw(raw_pad, pcount, 0, "%.*s", rw, msg->payload[raw_line]);
            }
        }

        // Print timestamp
        mvwprintw(flow_pad, cline, 2, "%s", msg->time);

        if (msgcnt == highlight) wattron(flow_pad,A_REVERSE);

        int msglen = strlen(msg->type);
        if (msglen > 24) msglen = 24;

        // Determine the message direction
        if (!strcmp(msg->call->callid, callid1) && !strcmp(msg->ip_from, from)) {
            wattron(flow_pad,COLOR_PAIR(OUTGOING_COLOR));
            mvwprintw(flow_pad, cline, 22, "%26s", "");
            mvwprintw(flow_pad, cline++, 22 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(flow_pad, cline, 22, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline++, 47, ACS_RARROW);
        } else if (!strcmp(msg->call->callid, callid1) && !strcmp(msg->ip_to, from)) {
            wattron(flow_pad,COLOR_PAIR(INCOMING_COLOR));
            mvwprintw(flow_pad, cline, 22, "%26s", "");
            mvwprintw(flow_pad, cline++, 22 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(flow_pad, cline, 22, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline++, 22, ACS_LARROW);
        } else if (!strcmp(msg->call->callid, callid2) && !strcmp(msg->ip_from, via)) {
            wattron(flow_pad,COLOR_PAIR(OUTGOING_COLOR));
            mvwprintw(flow_pad, cline, 52, "%26s", "");
            mvwprintw(flow_pad, cline++, 54 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(flow_pad, cline, 52, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline++, 77, ACS_RARROW);
        } else {
            wattron(flow_pad,COLOR_PAIR(INCOMING_COLOR));
            mvwprintw(flow_pad, cline, 52, "%26s", "");
            mvwprintw(flow_pad, cline++, 54 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(flow_pad, cline, 52, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline++, 52, ACS_LARROW);
        }

        wattroff(flow_pad,COLOR_PAIR(OUTGOING_COLOR));
        wattroff(flow_pad,COLOR_PAIR(INCOMING_COLOR));
        wattroff(flow_pad, A_REVERSE);
    }

    /* Calculate the space the pad will be covering in the screen */
    ph = fh - 3 /* CF header */- 2 /* Addresses */;
    /* Make it even */
    ph -= ph % 2;

    /* Calculate the highlight position */
    // The highlight position is below the last displayed position?
    if (highlight * 2 > ph + padpos) {
        // Scrolling down 
        padpos += 2;
        // The highlight position is above the first displayed position?
    } else if (highlight * 2 <= padpos) {
        // Scroll up
        padpos -= 2;
    }

    /* Draw some fancy arrow to indicate scrolling */
    if (padpos > 0) {
        mvwaddch(flow_pad, padpos, 20, ACS_UARROW);
        mvwaddch(flow_pad, padpos, 50, ACS_UARROW);
        mvwaddch(flow_pad, padpos, 80, ACS_UARROW);
    }
    if (msgcntex * 2 > ph + padpos) {
        mvwaddch(flow_pad, ph+padpos-1, 20, ACS_DARROW);
        mvwaddch(flow_pad, ph+padpos-1, 50, ACS_DARROW);
        mvwaddch(flow_pad, ph+padpos-1, 80, ACS_DARROW);
    }

    // Copy the callflow into the screen
    copywin(flow_pad, win, padpos, 1, 3 + 2 + 2, 1, 6 + ph, fw - 1, false);
    delwin(flow_pad);
    // Copy the rawmessage into the screen
    copywin(raw_pad, win, 0, 0, 3, fw + 1, rh, fw + rw, false);
    delwin(raw_pad);

    return 0;

}

int call_flow_ex_handle_key(PANEL *panel, char key) 
{
	return 0;
}


int call_flow_ex_help(PANEL *panel)
{
	return 0;
}
