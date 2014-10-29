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
 * @file ui_msg_diff.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_msg_diff.h
 *
 */
#include <stdlib.h>
#include <string.h>
#include "ui_msg_diff.h"
#include "option.h"

/***
 *
 * Some basic ascii art of this panel.
 *
 * +--------------------------------------------------------+
 * |                     Title                              |
 * |                          |                             |
 * |                          |                             |
 * |  First message payload   |                             |
 * |                          |                             |
 * |                          |   Second message payload    |
 * |                          |                             |
 * |                          |                             |
 * |                          |                             |
 * |                          |                             |
 * |                          |                             |
 * |                          |                             |
 * |                          |                             |
 * | Usefull hotkeys                                        |
 * +--------------------------------------------------------+
 *
 */

PANEL *
msg_diff_create()
{
    PANEL *panel;
    WINDOW *win;
    int height, width, hwidth;
    msg_diff_info_t *info;

    // Create a new panel to fill all the screen
    panel = new_panel(newwin(LINES, COLS, 0, 0));

    // Initialize panel specific data
    info = malloc(sizeof(msg_diff_info_t));
    memset(info, 0, sizeof(msg_diff_info_t));

    // Store it into panel userptr
    set_panel_userptr(panel, (void*) info);

    // Let's draw the fixed elements of the screen
    win = panel_window(panel);
    getmaxyx(win, height, width);

    // Calculate subwindows width
    hwidth = width / 2 - 1;

    // Create 2 subwindows, one for each message
    info->one_win = subwin(win, height - 2, hwidth, 1, 0);
    info->two_win = subwin(win, height - 2, hwidth, 1, hwidth + 1); // Header - Footer - Address

    // Draw a vertical line to separe both subwindows
    mvwvline(win, 0, hwidth, ACS_VLINE, height);

    // Draw title
    wattron(win, COLOR_PAIR(KEYBINDINGS_ACTION));
    mvwprintw(win, 0, 0, "%*s", width, "");
    mvwprintw(win, 0, (width - 45) / 2, "sngrep - SIP messages flow viewer");
    wattroff(win, COLOR_PAIR(KEYBINDINGS_ACTION));

    // Draw keybindings
    msg_diff_draw_footer(panel);

    return panel;
}

msg_diff_info_t *
msg_diff_info(PANEL *panel)
{
    return (msg_diff_info_t*) panel_userptr(panel);
}

void
msg_diff_destroy(PANEL *panel)
{
}

int
msg_diff_draw(PANEL *panel)
{
    msg_diff_info_t *info;
    WINDOW *win;
    int height, width, raw_line, line, column, raw_char;

    // Get panel information
    info = msg_diff_info(panel);

    werase(info->one_win);
    werase(info->two_win);

    // Get window of main panel
    win = info->one_win;
    getmaxyx(win, height, width);

    // Print msg payload
    for (line = 0, raw_line = 0; raw_line < info->one->plines; raw_line++) {
        if (strstr(info->one->payload[raw_line], ": "))
            wattron(win, A_BOLD);

        // Add character by character
        for (column = 0, raw_char = 0; raw_char < strlen(info->one->payload[raw_line]); raw_char++) {
            // Wrap at the end of the window
            if (column == width) {
                line++;
                column = 0;
            }
            // Don't write out of the window
            if (line == height)
                break;

            if (info->one->payload[raw_line][raw_char] == ':')
                wattroff(win, A_BOLD);

            // Put next character in position
            mvwaddch(win, line, column++, info->one->payload[raw_line][raw_char]);
        }
        // Done with this payload line, go to the next one
        line++;
    }

    // Get window of main panel
    win = info->two_win;
    getmaxyx(win, height, width);

    // Print msg payload
    for (line = 0, raw_line = 0; raw_line < info->two->plines; raw_line++) {
        if (strstr(info->two->payload[raw_line], ": "))
            wattron(win, A_BOLD);

        // Add character by character
        for (column = 0, raw_char = 0; raw_char < strlen(info->two->payload[raw_line]); raw_char++) {
            // Wrap at the end of the window
            if (column == width) {
                line++;
                column = 0;
            }
            // Don't write out of the window
            if (line == height)
                break;

            if (info->two->payload[raw_line][raw_char] == ':')
                wattroff(win, A_BOLD);

            // Put next character in position
            mvwaddch(win, line, column++, info->two->payload[raw_line][raw_char]);
        }
        // Done with this payload line, go to the next one
        line++;
    }

    // Redraw raw win
    wnoutrefresh(info->one_win);
    wnoutrefresh(info->two_win);

    return 0;
}

void
msg_diff_draw_footer(PANEL *panel)
{
    const char *keybindings[] =
        { "Esc", "Calls Flow", "F1", "Help" };

    draw_keybindings(panel, keybindings, 4);
}

int
msg_diff_draw_messages(PANEL *panel)
{
    return 0;
}

int
msg_diff_handle_key(PANEL *panel, int key)
{
    return key;
}

int
msg_diff_help(PANEL *panel)
{
    return 0;
}

int
msg_diff_set_msgs(PANEL *panel, sip_msg_t *one, sip_msg_t *two)
{
    msg_diff_info_t *info;

    // Get panel information
    info = msg_diff_info(panel);
    info->one = one;
    info->two = two;

    return 0;
}

