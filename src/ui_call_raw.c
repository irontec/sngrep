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
#include "ui_call_raw.h"
#include "sip.h"

struct sip_call *active_call;

PANEL *call_raw_create()
{
	PANEL *panel = new_panel(newwin(LINES, COLS, 0, 0));
	return panel;
}

int call_raw_draw(PANEL *panel)
{
	int w, h, ph, padpos, highlight, entries;
    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Make a pad for sip message
    WINDOW *raw_pad = newpad(get_n_msgs(active_call) * 100, COLS);

    int pline = 0, raw_line;
    struct sip_msg *msg = NULL;
    while ((msg = get_next_msg(active_call, msg))) {
        for (raw_line = 0; raw_line < msg->plines; raw_line++) {
            mvwprintw(raw_pad, pline, 0, "%s", msg->payload[raw_line]);
            pline++;
        }
        pline++;
        pline++;
    }
    copywin(raw_pad, win, highlight - 1, 0, 0, 0, LINES - 1, COLS - 1, false);
    delwin(raw_pad);

    // Update the last line
    entries = pline - LINES;

    return 0;
}

int call_raw_handle_key(PANEL *panel, char key) 
{
	return 0;
}

int call_raw_help(PANEL * ppanel)
{
	return 0;
}
