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
#include <stdlib.h>
#include "ui_manager.h"
#include "ui_call_raw.h"

PANEL *call_raw_create()
{
    PANEL *panel;
    call_raw_info_t *info;

    // Create a new panel to fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));
    // Initialize Call List specific data 
    info = malloc(sizeof(call_raw_info_t));
    memset(info, 0, sizeof(call_raw_info_t));
    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    return panel;
}

int call_raw_draw(PANEL *panel)
{
    struct sip_msg *msg = NULL;
    int pline = 0, raw_line;

    // Get panel information
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);
    // Get window of main panel
    WINDOW *win = panel_window(panel);

    mvwprintw(win, pline++, 0, "%d", info->scrollpos);

    while ((msg = get_next_msg(info->call, msg))) {
        for (raw_line = 0; raw_line < msg->plines; raw_line++) {
            mvwprintw(win, pline, 0, "%s", msg->payload[raw_line]);
            pline++;
        }
        pline++;
        pline++;
    }
    return 0;
}

int call_raw_handle_key(PANEL *panel, int key) 
{
    int i, rnpag_steps = 5;
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);

    // Sanity check, this should not happen
    if (!info) return -1;

    mvwprintw(panel_window(panel), 0, 10, "%d", key);

    switch (key) {
    case KEY_DOWN:
        info->scrollpos++;
        break;
    case KEY_UP:
        info->scrollpos--;
        break;
    case KEY_NPAGE:
        // Next page => N key down strokes 
        for (i=0; i < rnpag_steps; i++)
            call_raw_handle_key(panel, KEY_DOWN);
        break;
    case KEY_PPAGE:
        // Prev page => N key up strokes
        for (i=0; i < rnpag_steps; i++)
            call_raw_handle_key(panel, KEY_UP);
        break;
    default:
        return -1;
    }
    return 0;
}

int call_raw_help(PANEL * ppanel)
{
    return 0;
}

int call_raw_set_call(sip_call_t *call) {
    ui_t *raw_panel;
    PANEL *panel;
    call_raw_info_t *info;

    if (!call)
        return -1;

    if (!(raw_panel = ui_find_by_type(RAW_PANEL)))
        return -1;

    if (!(panel = raw_panel->panel))
        return -1;

    if (!(info = (call_raw_info_t*) panel_userptr(panel)))
        return -1;

    info->call = call;
    info->scrollpos = 0;
    return 0;
}
