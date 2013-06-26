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
 * @file ui_call_raw.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_call_raw.h
 *
 * @todo Code help screen. Please.
 * @todo Replace the panel refresh. Wclear sucks on high latency conections.
 *
 */
#include <string.h>
#include <stdlib.h>
#include "ui_call_raw.h"

PANEL *
call_raw_create()
{
    PANEL *panel;
    WINDOW *win;
    call_raw_info_t *info;
    int height, width;

    // Create a new panel to fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));
    // Initialize Call List specific data 
    info = malloc(sizeof(call_raw_info_t));
    memset(info, 0, sizeof(call_raw_info_t));
    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate available printable area
    info->linescnt = height;

    return panel;
}

int
call_raw_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    return 0;
}

int
call_raw_draw(PANEL *panel)
{
    struct sip_msg *msg = NULL;
    int pline = 0, raw_line;
    const char *all_lines[1024];
    int all_linescnt = 0;

    // Get panel information
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);
    // Get window of main panel
    WINDOW *win = panel_window(panel);
    wclear(win);

    /*
     FIXME This part could be coded decently.
     A better aproach is creating a pad window and copy the visible area
     into the panel window, taking into account the scroll position.
     This is dirty but easier to manage by now
     */
    memset(all_lines, 0, 1024);
    while ((msg = call_get_next_msg(info->call, msg))) {
        for (raw_line = 0; raw_line < msg->plines; raw_line++) {
            all_lines[all_linescnt++] = msg->payload[raw_line];
        }
        all_lines[all_linescnt++] = NULL;
        all_lines[all_linescnt++] = NULL;
    }
    info->all_lines = all_linescnt;

    for (raw_line = info->scrollpos; raw_line <= all_linescnt; raw_line++) {
        // Until we have reached the end of the screen
        if (pline >= info->linescnt) break;
        // If printable line, otherwise let this line empty
        if (all_lines[raw_line]) mvwprintw(win, pline, 0, "%s", all_lines[raw_line]);
        // but increase line counter
        pline++;
    }
    return 0;
}

int
call_raw_handle_key(PANEL *panel, int key)
{
    int i, rnpag_steps = 10;
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);

    // Sanity check, this should not happen
    if (!info) return -1;

    mvwprintw(panel_window(panel), 0, 10, "%d", key);

    switch (key) {
    case KEY_DOWN:
        if (info->scrollpos + info->linescnt < info->all_lines) info->scrollpos++;
        break;
    case KEY_UP:
        if (info->scrollpos > 0) info->scrollpos--;
        break;
    case KEY_NPAGE:
        // Next page => N key down strokes 
        for (i = 0; i < rnpag_steps; i++)
            call_raw_handle_key(panel, KEY_DOWN);
        break;
    case KEY_PPAGE:
        // Prev page => N key up strokes
        for (i = 0; i < rnpag_steps; i++)
            call_raw_handle_key(panel, KEY_UP);
        break;
    default:
        return -1;
    }
    return 0;
}

int
call_raw_set_call(sip_call_t *call)
{
    ui_t *raw_panel;
    PANEL *panel;
    call_raw_info_t *info;

    if (!call) return -1;

    if (!(raw_panel = ui_find_by_type(RAW_PANEL))) return -1;

    if (!(panel = raw_panel->panel)) return -1;

    if (!(info = (call_raw_info_t*) panel_userptr(panel))) return -1;

    info->call = call;
    info->scrollpos = 0;
    return 0;
}
