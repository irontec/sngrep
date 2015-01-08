/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
#include "ui_manager.h"
#include "ui_call_raw.h"
#include "ui_save_raw.h"
#include "option.h"
#include "capture.h"

PANEL *
call_raw_create()
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

    // Create a initial pad of 1000 lines
    info->pad = newpad(500, COLS);
    info->padline = 0;
    info->scroll = 0;

    return panel;
}

int
call_raw_redraw_required(PANEL *panel, sip_msg_t *msg)
{
    call_raw_info_t *info;
    // Get panel info
    if (!(info = (call_raw_info_t*) panel_userptr(panel)))
        return -1;
    // Check if we're displaying a group
    if (!info->group)
        return -1;
    // If this message belongs to one of the printed calls
    if (call_group_exists(info->group, msg->call)) {
        call_raw_print_msg(panel, msg_parse(msg));
        return 0;
    }
    return -1;
}

int
call_raw_draw(PANEL *panel)
{
    // Get panel information
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);
    // Copy the visible part of the pad into the panel window
    copywin(info->pad, panel_window(panel), info->scroll, 0, 0, 0, LINES - 1, COLS - 1, 0);
    return 0;
}

int
call_raw_print_msg(PANEL *panel, sip_msg_t *msg)
{
    // Previous message pointer
    sip_msg_t *prev;
    // Variables for drawing each message character
    int raw_line, raw_char, column;
    // Message ngrep style Header
    char header[256];

    // Get panel information
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);

    // Get the pad window
    WINDOW *pad = info->pad;
    // Get the pad's current line
    int line = info->padline;

    // Check if we have enough space in our huge pad to store this message
    int height = getmaxy(pad);
    if (line + msg->plines + 10 > height) {
        // Create a new pad with more lines!
        pad = newpad(height + 500, COLS);
        // And copy all previous information
        overwrite(info->pad, pad);
        info->pad = pad;
    }

    // Color the message {
    if (is_option_enabled("color.request")) {
        // Determine arrow color
        if (msg_get_attribute(msg, SIP_ATTR_REQUEST)) {
            msg->color = OUTGOING_COLOR;
        } else {
            msg->color = INCOMING_COLOR;
        }
    } else if (info->group && is_option_enabled("color.callid")) {
        // Color by call-id
        msg->color = call_group_color(info->group, msg->call);
    } else if (is_option_enabled("color.cseq")) {
        // Color by CSeq within the same call
        msg->color = atoi(msg_get_attribute(msg, SIP_ATTR_CSEQ)) % 7 + 1;
    }

    // Turn on the message color
    wattron(pad, COLOR_PAIR(msg->color));

    // Print msg header
    wattron(pad, A_BOLD);
    mvwprintw(pad, line++, 0, "%s", msg_get_header(msg, header));
    wattroff(pad, A_BOLD);

    // Print msg payload
    for (raw_line = 0; raw_line < msg->plines; raw_line++) {
        // Add character by character
        for (column = 0, raw_char = 0; raw_char < strlen(msg->payload[raw_line]); raw_char++) {
            // Wrap at the end of the window
            if (column == COLS) {
                line++;
                column = 0;
            }
            mvwaddch(pad, line, column++, msg->payload[raw_line][raw_char]);
        }
        // Increase line after writing it
        line++;
    }
    // Extra line between messages
    line++;

    // Store current pad position
    info->padline = line;

    return 0;
}

int
call_raw_handle_key(PANEL *panel, int key)
{
    call_raw_info_t *info = (call_raw_info_t*) panel_userptr(panel);
    ui_t *next_panel;

    // Sanity check, this should not happen
    if (!info)
        return -1;

    // Move scroll depending on key pressed
    switch (key) {
    case KEY_DOWN:
        info->scroll++;
        break;
    case KEY_UP:
        info->scroll--;
        break;
    case KEY_NPAGE:
        // Next page => N key down strokes
        info->scroll += 10;
        break;
    case KEY_PPAGE:
        // Prev page => N key up strokes
        info->scroll -= 10;
        break;
    case 'l':
        // Tooggle Host/Address display
        toggle_option("sngrep.displayhost");
        // Force refresh panel
        if (info->group) {
            call_raw_set_group(info->group);
        } else {
            call_raw_set_msg(info->msg);
        }
        break;
    case 's':
    case 'S':
        if (info->group) {
            // KEY_S, Display save panel
            next_panel = ui_create(ui_find_by_type(SAVE_RAW_PANEL));
            save_raw_set_group(next_panel->panel, info->group);
            wait_for_input(next_panel);
        }
        break;
    default:
        return key;
    }

    if (info->scroll < 0 || info->padline < LINES) {
        info->scroll = 0;   // Disable scrolling if there's nothing to scroll
    } else {
        if (info->scroll + LINES / 2 > info->padline)
            info->scroll = info->padline - LINES / 2;
    }
    return 0;
}

int
call_raw_set_group(sip_call_group_t *group)
{
    ui_t *raw_panel;
    PANEL *panel;
    call_raw_info_t *info;
    sip_msg_t *msg = NULL;

    if (!group)
        return -1;

    if (!(raw_panel = ui_find_by_type(RAW_PANEL)))
        return -1;

    if (!(panel = raw_panel->panel))
        return -1;

    if (!(info = (call_raw_info_t*) panel_userptr(panel)))
        return -1;

    // Set call raw call group
    info->group = group;
    info->msg = NULL;

    // Initialize internal pad
    info->padline = 0;
    wclear(info->pad);

    // Print the call group messages into the pad
    while ((msg = call_group_get_next_msg(info->group, msg)))
        call_raw_print_msg(panel, msg);

    return 0;
}

int
call_raw_set_msg(sip_msg_t *msg)
{
    ui_t *raw_panel;
    PANEL *panel;
    call_raw_info_t *info;

    if (!msg)
        return -1;

    if (!(raw_panel = ui_find_by_type(RAW_PANEL)))
        return -1;

    if (!(panel = raw_panel->panel))
        return -1;

    if (!(info = (call_raw_info_t*) panel_userptr(panel)))
        return -1;

    // Set call raw message
    info->group = NULL;
    info->msg = msg;

    // Initialize internal pad
    info->padline = 0;
    wclear(info->pad);

    // Print the message in the pad
    call_raw_print_msg(panel, msg);

    return 0;

}
