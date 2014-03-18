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
 ** == 1)
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
#include "option.h"

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
    info->totlines = 0;

    return panel;
}

int
call_raw_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    call_raw_info_t *info;
    if (!(info = (call_raw_info_t*) panel_userptr(panel))) return -1;
    return call_group_exists(info->group, msg->call);
}

int
call_raw_draw(PANEL *panel)
{
    struct sip_msg *msg = NULL;
    wclear(panel_window(panel));

    // Get panel information
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);
    info->totlines = info->scrline = 0;

    if (info->msg) {
        call_raw_print_msg(panel, info->msg);
    } else {
        while ((msg = call_group_get_next_msg(info->group, msg)))
            call_raw_print_msg(panel, msg);
    }

    return 0;
}

int
call_raw_print_msg(PANEL *panel, sip_msg_t *msg)
{
    // Get panel information
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);

    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get current line
    int raw_line;

    // Determine arrow color
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

    for (raw_line = 0; raw_line < msg->plines; raw_line++) {
        info->totlines++;
        // Check if we must start drawing
        if (info->totlines < info->scrollpos) continue;
        // Until we have reached the end of the screen
        if (info->scrline >= info->linescnt) break;

        // If printable line, otherwise let this line empty
        mvwprintw(win, info->scrline++, 0, "%s", msg->payload[raw_line]);
        if (raw_line == msg->plines - 1) {
            info->scrline++;
            info->totlines++;
        }
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
        if (info->scrollpos + info->linescnt - 10 < info->totlines) info->scrollpos++;
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
        return key;
    }
    return 0;
}

int
call_raw_set_group(sip_call_group_t *group)
{
    ui_t *raw_panel;
    PANEL *panel;
    call_raw_info_t *info;

    if (!group) return -1;

    if (!(raw_panel = ui_find_by_type(RAW_PANEL))) return -1;

    if (!(panel = raw_panel->panel)) return -1;

    if (!(info = (call_raw_info_t*) panel_userptr(panel))) return -1;

    info->group = group;
    info->scrollpos = 0;
    return 0;
}

int
call_raw_set_msg(sip_msg_t *msg)
{
    ui_t *raw_panel;
    PANEL *panel;
    call_raw_info_t *info;

    if (!msg) return -1;

    if (!(raw_panel = ui_find_by_type(RAW_PANEL))) return -1;

    if (!(panel = raw_panel->panel)) return -1;

    if (!(info = (call_raw_info_t*) panel_userptr(panel))) return -1;

    info->msg = msg;
    info->scrollpos = 0;
    return 0;

}
