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

void
msg_diff_destroy(PANEL *panel)
{
    if (msg_diff_info(panel))
        free(msg_diff_info(panel));
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

    for (i=0; i < strlen(payload1); i++) {
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
msg_diff_highlight(sip_msg_t *one, sip_msg_t *two, char *highlight)
{
    if (get_option_value("diff.mode")) {
        // Select proper highlight logic
        if (!strcasecmp(get_option_value("diff.mode"), "lcs")) {
            // @todo msg_diff_lcs_highlight(one->payloadptr, two->payloadptr, highlight);
        } else if (!strcasecmp(get_option_value("diff.mode"), "line")) {
            msg_diff_line_highlight(one->payloadptr, two->payloadptr, highlight);
        } else {
            // Unknown hightlight enabled
        }
    }
}

int
msg_diff_draw(PANEL *panel)
{
    // Get panel information
    msg_diff_info_t *info = msg_diff_info(panel);
    char highlight[4086];

    // Draw first message
    memset(highlight, 0 , sizeof(highlight));
    msg_diff_highlight(info->one, info->two, highlight);
    msg_diff_draw_message(info->one_win, info->one, highlight);
    // Draw second message
    memset(highlight, 0 , sizeof(highlight));
    msg_diff_highlight(info->two, info->one, highlight);
    msg_diff_draw_message(info->two_win, info->two, highlight);

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
msg_diff_draw_message(WINDOW *win, sip_msg_t *msg, char *highlight)
{
    int height, width, line, column, i;
    char header[256];

    // Clear the window
    werase(win);

    // Get window of main panel
    getmaxyx(win, height, width);

    mvwprintw(win, 0, 0, msg_get_header(msg, header));

    // Print msg payload
    line = 2;
    column = 0;
    for (i = 0; i < strlen(msg->payloadptr); i++) {
        if (msg->payloadptr[i] == '\r')
            continue;

        if (column == width || msg->payloadptr[i] == '\n') {
            line++;
            column = 0;
            continue;
        }

        if (line == height)
            break;

        if (highlight[i] == '1') {
            wattron(win, COLOR_PAIR(DIFF_HIGHLIGHT));
        } else {
            wattroff(win, COLOR_PAIR(DIFF_HIGHLIGHT));
        }

        // Put next character in position
        mvwaddch(win, line, column++, msg->payloadptr[i]);
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

/*
int
msg_diff_lcs_sequence(const char* s1, const char* s2, int *highlight)
{
    size_t l1 = strlen(s1), l2 = strlen(s2);
    size_t sz = (l1 + 1) * (l2 + 1) * sizeof(size_t);
    size_t w = l2 + 1;
    size_t* dpt;
    size_t i1, i2;
    int distance = 0;

    if (sz / (l1 + 1) / (l2 + 1) != sizeof(size_t) || (dpt = malloc(sz)) == NULL)
        return 1;

    for (i1 = 0; i1 <= l1; i1++)
        dpt[w * i1 + 0] = 0;
    for (i2 = 0; i2 <= l2; i2++)
        dpt[w * 0 + i2] = 0;

    for (i1 = 1; i1 <= l1; i1++) {
        for (i2 = 1; i2 <= l2; i2++) {
            if (s1[l1 - i1] == s2[l2 - i2]) {
                dpt[w * i1 + i2] = dpt[w * (i1 - 1) + (i2 - 1)] + 1;
            } else if (dpt[w * (i1 - 1) + i2] > dpt[w * i1 + (i2 - 1)]) {
                dpt[w * i1 + i2] = dpt[w * (i1 - 1) + i2];
            } else {
                dpt[w * i1 + i2] = dpt[w * i1 + (i2 - 1)];
            }
        }
    }

    i1 = l1;
    i2 = l2;
    for (;;) {
        if ((i1 > 0) && (i2 > 0) && (s1[l1 - i1] == s2[l2 - i2])) {
            highlight[l1 - i1] = 0;
            i1--;
            i2--;
            continue;
        } else {
            if (i1 > 0 && (i2 == 0 || dpt[w * (i1 - 1) + i2] >= dpt[w * i1 + (i2 - 1)])) {
                highlight[l1 - i1] = 1;
                i1--;
                distance++;
                continue;
            } else if (i2 > 0 && (i1 == 0 || dpt[w * (i1 - 1) + i2] < dpt[w * i1 + (i2 - 1)])) {
                i2--;
                distance++;
                continue;
            }
        }
        break;
    }

    free(dpt);
    return distance;
}

int
msg_diff_lcs_highlight(const char* s1, const char* s2, int *highlight)
{
    // Split payload into lines
    char *p1[256];
    char *p2[256];
    int p1l = 0, p2l = 0, i, j, best, distance;
    int tmp_highlight[256], *out_highlight;
    char *pch;
    char *payload1 = strdup(s1);
    char *payload2 = strdup(s2);
    int outpos = 0;

    for (pch = strtok(payload1, "\n"); pch; pch = strtok(NULL, "\n")) {
        p1[p1l] = strdup(pch);
        p1l++;
    }

    for (pch = strtok(payload2, "\n"); pch; pch = strtok(NULL, "\n")) {
        p2[p2l] = strdup(pch);
        p2l++;
    }

    out_highlight = highlight;
    int r;

    for (i = 0; i < p1l; i++) {
        best = 256;
        for (j = 0; j < p2l; j++) {

            if (strncasecmp(p1[i], p2[j], 3))
                continue;

            distance = msg_diff_lcs_sequence(p1[i], p2[j], tmp_highlight);
            if (distance < best) {
                best = distance;
                for (r = 0; r < strlen(p1[i]); r++)
                    highlight[outpos + r] = tmp_highlight[r];
            }
        }

        if (best == 256) {
            for (r = 0; r < strlen(p1[i]); r++)
                highlight[outpos + r] = 1;
        }
        outpos += strlen(p1[i]) + 1;
    }

    free(payload1);
    free(payload2);
    return 0;
}
*/

