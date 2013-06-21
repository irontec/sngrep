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
#include "ui_call_list.h"
#include "sip.h"

extern struct sip_call *calls;
struct sip_call *active_call;

PANEL *call_list_create()
{
	PANEL *panel = new_panel(newwin(LINES, COLS, 0, 0));
	return panel;
}

int call_list_draw(PANEL *panel)
{
    int y = 1, x = 5;
    int w, h, ph, padpos, highlight, entries;

    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get panel info
    entries = get_n_calls();

    // Get window size
    getmaxyx(win, h, w);

    title_foot_box(win);
    mvwprintw(win, y, (w - 45) / 2, "sngrep - SIP message interface for ngrep");
    mvwprintw(win, y + 2, 2, "Current Mode: %s", "Online");
    mvwaddch(win, y+4, 0, ACS_LTEE);
    mvwhline(win, y+4, 1, ACS_HLINE, w - 2);
    mvwaddch(win, y+4, w-1, ACS_RTEE);
    mvwprintw(win, y + 5, x + 2, "From SIP");
    mvwprintw(win, y + 5, x + 51, "To SIP");
    mvwprintw(win, y + 5, x + 109, "Msg");
    mvwprintw(win, y + 5, x + 116, "From");
    mvwprintw(win, y + 5, x + 136, "To");
    mvwprintw(win, y + 5, x + 155, "Starting");
    mvwaddch(win, y+6, 0, ACS_LTEE);
    mvwhline(win, y+6, 1, ACS_HLINE, w - 2);
    mvwaddch(win, y+6, w-1, ACS_RTEE);

    WINDOW *main_pad = newpad(get_n_calls() + h, w);

    struct sip_call *call = calls;
    int cline = 0, callcnt = 1;
    while (call) {
        if (callcnt == highlight) {
            active_call = call;
            wattron(main_pad,COLOR_PAIR(HIGHLIGHT_COLOR));
        }

        mvwprintw(main_pad, cline, x + 2, "%*s", w - x * 2 - 4, ""); /* Highlight all the line */
        mvwprintw(main_pad, cline, x, "%.50s", call->messages->sip_from);
        mvwprintw(main_pad, cline, x + 50, "%.50s", call->messages->sip_to);
        mvwprintw(main_pad, cline, x + 109, "%d", get_n_msgs(call));
        mvwprintw(main_pad, cline, x + 115, "%s", call->messages->ip_from);
        mvwprintw(main_pad, cline, x + 135, "%s", call->messages->ip_to);
        mvwprintw(main_pad, cline, x + 155, "%s", call->messages->type);
        wattroff(main_pad, COLOR_PAIR(HIGHLIGHT_COLOR));
        cline++;
        callcnt++;
        call = call->next;
    }

    /* Calculate the space the pad will be covering in the screen */
    ph = h - 2 /* Title */- 5 /* Header */- 2 /* Footer */;

    /* Calculate the highlight position */
    // The highlight position is below the last displayed position?
    if (highlight > ph + padpos - 2) {
        // Scrolling down 
        padpos++;
        // The highlight position is above the first displayed position?
    } else if (highlight <= padpos) {
        // Scroll up
        padpos--;
    }

    /* Draw some fancy arrow to indicate scrolling */
    if (padpos > 0) {
        mvwaddch(main_pad, padpos, 3, ACS_UARROW);
    }
    if (get_n_calls() > ph + padpos) {
        mvwaddch(main_pad, ph+padpos-3, 3, ACS_DARROW);
    }

    // Copy the rawmessage into the screen
    copywin(main_pad, win, padpos, 1, 2 + 5 + 1, 1, 5 + ph, w - 2, false);
    delwin(main_pad);

    mvwprintw(
            win,
            h - 2,
            2,
            "Q: Quit    C: Toggle color    F: Show raw messages     H: Help    ENTER: Show Call-flow    X: Show Extended Call-Flow");

    return 0;

}

int call_list_handle_key(PANEL *panel, char key) 
{
	return 0;
}

int call_list_help(PANEL * ppanel)
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
    mvwaddch(win, cline++, width-1, ACS_RTEE);
    wattron(win,COLOR_PAIR(HELP_COLOR));
    mvwprintw(win, cline, 3, "F1/h:");
    mvwprintw(win, cline + 1, 3, "ESC/q:");
    mvwprintw(win, cline + 2, 3, "Up:");
    mvwprintw(win, cline + 3, 3, "Down:");
    mvwprintw(win, cline + 4, 3, "Enter:");
    wattroff(win,COLOR_PAIR(HELP_COLOR));
    mvwprintw(win, cline, 15, "Show this screen :)");
    mvwprintw(win, cline + 1, 15, "Exit sngrep");
    mvwprintw(win, cline + 2, 15, "Select Previous dialog");
    mvwprintw(win, cline + 3, 15, "Select Next dialog");
    mvwprintw(win, cline + 4, 15, "Show dialog details");

	return 0;
}
