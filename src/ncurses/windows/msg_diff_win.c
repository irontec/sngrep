/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
 * @file msg_diff_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_msg_diff.h
 *
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "msg_diff_win.h"
#include "setting.h"

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
 * @brief Get panel information structure
 *
 * All required information of the panel is stored in the info pointer
 * of the panel.
 * This function will return the pointer to the info structure of the
 * panel.
 *
 * @param ui UI structure pointer
 * @return a pointer to the info structure or NULL if no structure exists
 */
static MsgDiffWinInfo *
msg_diff_info(Window *ui)
{
    return (MsgDiffWinInfo *) panel_userptr(ui->panel);
}

static int
msg_diff_line_highlight(const char *payload1, const char *payload2, char *highlight)
{
    char search[MAX_SIP_PAYLOAD];
    int len;

    // Initialize search terms
    memset(search, 0, sizeof(search));
    len = 0;

    for (guint i = 0; i < strlen(payload1); i++) {
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

/**
 * @brief Draw panel footer
 *
 * Usually panel footer contains useful keybidings. This function
 * will draw that footer
 *
 * @param window UI structure pointer
 */
static void
msg_diff_draw_footer(Window *window)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_PREV_SCREEN), "Calls Flow",
        key_action_key_str(ACTION_SHOW_HELP), "Help"
    };

    window_draw_bindings(window, keybindings, 4);
}

/**
 * @brief Draw a message into a raw subwindow
 *
 * This function will be called for each message that wants to be draw
 * in the panel.
 *
 */
static int
msg_diff_draw_message(WINDOW *win, Message *msg, char *highlight)
{
    int height, width, line, column;
    char header[MAX_SIP_PAYLOAD];
    const char *payload = msg_get_payload(msg);

    // Clear the window
    werase(win);

    // Get window of main panel
    getmaxyx(win, height, width);

    wattron(win, A_BOLD);
    mvwprintw(win, 0, 0, msg_get_header(msg, header));
    wattroff(win, A_BOLD);

    // Print msg payload
    line = 2;
    column = 0;
    for (guint i = 0; i < strlen(payload); i++) {
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

/**
 * @brief Redraw panel data
 *
 * This function will be called from ui manager logic when the panels
 * needs to be redrawn.
 *
 * @param window UI structure pointer
 * @return 0 in all cases
 */
static int
msg_diff_draw(Window *window)
{
    // Get panel information
    MsgDiffWinInfo *info = msg_diff_info(window);
    g_return_val_if_fail(info != NULL, -1);

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
    msg_diff_draw_footer(window);

    return 0;
}


void
msg_diff_win_set_msgs(Window *window, Message *one, Message *two)
{
    MsgDiffWinInfo *info = msg_diff_info(window);
    g_return_if_fail(info != NULL);

    g_return_if_fail(one != NULL);
    g_return_if_fail(two != NULL);

    info->one = one;
    info->two = two;
}

void
msg_diff_win_free(Window *window)
{
    MsgDiffWinInfo *info = msg_diff_info(window);
    g_return_if_fail(info != NULL);

    g_free(info);
    window_deinit(window);
}

Window *
msg_diff_win_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_MSG_DIFF;
    window->destroy = msg_diff_win_free;
    window->draw = msg_diff_draw;

    // Create a new panel to fill all the screen
    window_init(window, getmaxy(stdscr), getmaxx(stdscr));

    // Initialize panel specific data
    MsgDiffWinInfo *info = g_malloc0(sizeof(MsgDiffWinInfo));

    // Store it into panel userptr
    set_panel_userptr(window->panel, (void *) info);

    // Calculate subwindows width
    gint hwidth = window->width / 2 - 1;

    // Create 2 subwindows, one for each message
    info->one_win = subwin(window->win, window->height - 2, hwidth, 1, 0);
    info->two_win = subwin(window->win, window->height - 2, hwidth, 1, hwidth + 1); // Header - Footer - Address

    // Draw a vertical line to separe both subwindows
    mvwvline(window->win, 0, hwidth, ACS_VLINE, window->height);

    // Draw title
    window_set_title(window, "sngrep - SIP messages flow viewer");

    return window;
}
