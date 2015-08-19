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
 * |  First message header    |   Second message header     |
 * |                          |                             |
 * |  First message payload   |                             |
 * |                          |                             |
 * |                          |   Second message payload    |
 * |                          |                             |
 * |                          |                             |
 * |                          |                             |
 * |                          |                             |
 * |                          |                             |
 * | Usefull hotkeys                                        |
 * +--------------------------------------------------------+
 *
 */

/**
 * Ui Structure definition for Message Diff panel
 */
ui_t ui_msg_diff = {
    .type = PANEL_MSG_DIFF,
    .panel = NULL,
    .create = msg_diff_create,
    .handle_key = msg_diff_handle_key,
    .destroy = msg_diff_destroy,
    .draw = msg_diff_draw,
    .help = msg_diff_help
};

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
    info = sng_malloc(sizeof(msg_diff_info_t));

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
    draw_title(panel, "sngrep - SIP messages flow viewer");

    // Draw keybindings
    msg_diff_draw_footer(panel);

    return panel;
}

void
msg_diff_destroy(PANEL *panel)
{
    if (msg_diff_info(panel))
        sng_free(msg_diff_info(panel));
}

msg_diff_info_t *
msg_diff_info(PANEL *panel)
{
    return (msg_diff_info_t*) panel_userptr(panel);
}

int
msg_diff_line_highlight(const char* payload1, const char* payload2, char *highlight)
{
    char search[512];
    int len, i;

    // Initialize search terms
    memset(search, 0, sizeof(search));
    len = 0;

    for (i = 0; i < strlen(payload1); i++) {
        // Store this char in the search term
        search[len++] = payload1[i];
        // If we have a full line in search array
        if (payload1[i] == '\n') {
            // Check if this line is in the other payload
            if (strstr(payload2, search) == NULL) {
                // Highlight this line as different from the other payload
                memset(highlight + i - len + 1, '1', len);
            }

            // Reset search terms
            memset(search, 0, sizeof(search));
            len = 0;
        }
    }

    return 0;
}

void
msg_diff_draw_footer(PANEL *panel)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Calls Flow",
        key_action_key_str(ACTION_SHOW_HELP), "Help"
    };

    draw_keybindings(panel, keybindings, 4);
}

int
msg_diff_draw(PANEL *panel)
{
    // Get panel information
    msg_diff_info_t *info = msg_diff_info(panel);
    char highlight[4086];

    // Draw first message
    memset(highlight, 0, sizeof(highlight));
    msg_diff_line_highlight(msg_get_payload(info->one), msg_get_payload(info->two), highlight);
    msg_diff_draw_message(info->one_win, info->one, highlight);
    // Draw second message
    memset(highlight, 0, sizeof(highlight));
    msg_diff_line_highlight(msg_get_payload(info->two), msg_get_payload(info->one), highlight);
    msg_diff_draw_message(info->two_win, info->two, highlight);

    // Redraw footer
    msg_diff_draw_footer(panel);

    return 0;
}

int
msg_diff_draw_message(WINDOW *win, sip_msg_t *msg, char *highlight)
{
    int height, width, line, column, i;
    char header[256];
    const char * payload = msg_get_payload(msg);

    // Clear the window
    werase(win);

    // Get window of main panel
    getmaxyx(win, height, width);

    wattron(win, A_BOLD);
    mvwprintw(win, 0, 0, sip_get_msg_header(msg, header));
    wattroff(win, A_BOLD);

    // Print msg payload
    line = 2;
    column = 0;
    for (i = 0; i < strlen(payload); i++) {
        if (payload[i] == '\r')
            continue;

        if (column == width || payload[i] == '\n') {
            line++;
            column = 0;
            continue;
        }

        if (line == height)
            break;

        if (highlight[i] == '1') {
            wattron(win, COLOR_PAIR(CP_YELLOW_ON_DEF));
        } else {
            wattroff(win, COLOR_PAIR(CP_YELLOW_ON_DEF));
        }

        // Put next character in position
        mvwaddch(win, line, column++, payload[i]);
    }

    // Redraw raw win
    wnoutrefresh(win);

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

