/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
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
    .handle_key = NULL,
    .destroy = msg_diff_destroy,
    .draw = msg_diff_draw,
    .help = NULL
};

void
msg_diff_create(ui_t *ui)
{
    int hwidth;
    msg_diff_info_t *info;

    // Create a new panel to fill all the screen
    ui_panel_create(ui, LINES, COLS);

    // Initialize panel specific data
    info = sng_malloc(sizeof(msg_diff_info_t));

    // Store it into panel userptr
    set_panel_userptr(ui->panel, (void*) info);

    // Calculate subwindows width
    hwidth = ui->width / 2 - 1;

    // Create 2 subwindows, one for each message
    info->one_win = subwin(ui->win, ui->height - 2, hwidth, 1, 0);
    info->two_win = subwin(ui->win, ui->height - 2, hwidth, 1, hwidth + 1); // Header - Footer - Address

    // Draw a vertical line to separe both subwindows
    mvwvline(ui->win, 0, hwidth, ACS_VLINE, ui->height);

    // Draw title
    ui_set_title(ui, "sngrep - SIP messages flow viewer");

    // Draw keybindings
    msg_diff_draw_footer(ui);
}

void
msg_diff_destroy(ui_t *ui)
{
    sng_free(msg_diff_info(ui));
    ui_panel_destroy(ui);
}

msg_diff_info_t *
msg_diff_info(ui_t *ui)
{
    return (msg_diff_info_t*) panel_userptr(ui->panel);
}

int
msg_diff_line_highlight(const char* payload1, const char* payload2, char *highlight)
{
    char search[MAX_SIP_PAYLOAD];
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
msg_diff_draw_footer(ui_t *ui)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Calls Flow",
        key_action_key_str(ACTION_SHOW_HELP), "Help"
    };

    ui_draw_bindings(ui, keybindings, 4);
}

int
msg_diff_draw(ui_t *ui)
{
    // Get panel information
    msg_diff_info_t *info = msg_diff_info(ui);
    char highlight[MAX_SIP_PAYLOAD];

    // Draw first message
    memset(highlight, 0, sizeof(highlight));
    msg_diff_line_highlight(msg_get_payload(info->one), msg_get_payload(info->two), highlight);
    msg_diff_draw_message(info->one_win, info->one, highlight);
    // Draw second message
    memset(highlight, 0, sizeof(highlight));
    msg_diff_line_highlight(msg_get_payload(info->two), msg_get_payload(info->one), highlight);
    msg_diff_draw_message(info->two_win, info->two, highlight);

    // Redraw footer
    msg_diff_draw_footer(ui);

    return 0;
}

int
msg_diff_draw_message(WINDOW *win, sip_msg_t *msg, char *highlight)
{
    int height, width, line, column, i;
    char header[MAX_SIP_PAYLOAD];
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
msg_diff_set_msgs(ui_t *ui, sip_msg_t *one, sip_msg_t *two)
{
    msg_diff_info_t *info;

    // Get panel information
    info = msg_diff_info(ui);
    info->one = one;
    info->two = two;

    return 0;
}

