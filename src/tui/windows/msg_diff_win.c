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
#include <string.h>
#include "msg_diff_win.h"
#include "setting.h"

G_DEFINE_TYPE(MsgDiffWindow, msg_diff_win, SNG_TYPE_WINDOW)

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
 * | Useful hotkeys                                         |
 * +--------------------------------------------------------+
 *
 */

static int
msg_diff_win_line_highlight(const char *payload1, const char *payload2, char *highlight)
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
msg_diff_win_draw_footer(SngAppWindow *window)
{
    const char *keybindings[] = {
        key_action_key_str(ACTION_CLOSE), "Calls Flow",
        key_action_key_str(ACTION_SHOW_HELP), "Help"
    };

}

/**
 * @brief Draw a message into a raw subwindow
 *
 * This function will be called for each message that wants to be draw
 * in the panel.
 *
 */
static int
msg_diff_win_draw_message(WINDOW *win, Message *msg, char *highlight)
{
    int height, width, line, column;
    char header[MAX_SIP_PAYLOAD];
    g_autofree gchar *payload = msg_get_payload(msg);

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
static void
msg_diff_win_draw(SngWidget *widget)
{
    // Get panel information
    MsgDiffWindow *self = TUI_MSG_DIFF(widget);
    SngAppWindow *app_window = SNG_APP_WINDOW(widget);

    char highlight[MAX_SIP_PAYLOAD];

    // Draw first message
    memset(highlight, 0, sizeof(highlight));
    g_autofree gchar *payload_one = msg_get_payload(self->one);
    g_autofree gchar *payload_two = msg_get_payload(self->two);

    msg_diff_win_line_highlight(payload_one, payload_two, highlight);
    msg_diff_win_draw_message(self->one_win, self->one, highlight);

    // Draw second message
    memset(highlight, 0, sizeof(highlight));
    msg_diff_win_line_highlight(payload_two, payload_one, highlight);
    msg_diff_win_draw_message(self->two_win, self->two, highlight);

    // Redraw footer
    msg_diff_win_draw_footer(app_window);
}


void
msg_diff_win_set_msgs(SngAppWindow *window, Message *one, Message *two)
{
    MsgDiffWindow *self = TUI_MSG_DIFF(window);
    g_return_if_fail(self != NULL);

    g_return_if_fail(one != NULL);
    g_return_if_fail(two != NULL);

    self->one = one;
    self->two = two;
}

SngAppWindow *
msg_diff_win_new()
{
    return g_object_new(
        WINDOW_TYPE_MSG_DIFF,
        "window-type", SNG_WINDOW_TYPE_MSG_DIFF,
        NULL
    );
}

static void
msg_diff_win_constructed(GObject *object)
{
    // Chain-up parent constructed
    G_OBJECT_CLASS(msg_diff_win_parent_class)->constructed(object);

    MsgDiffWindow *self = TUI_MSG_DIFF(object);
    SngAppWindow *app_window = SNG_APP_WINDOW(self);
    SngWidget *widget = SNG_WIDGET(object);
    WINDOW *win = sng_widget_get_ncurses_window(widget);

    gint height = sng_widget_get_height(widget);
    gint width = sng_widget_get_width(widget);

    // Calculate sub-windows width
    gint hwidth = width / 2 - 1;

    // Create 2 sub-windows, one for each message
    self->one_win = subwin(win, height - 2, hwidth, 1, 0);
    self->two_win = subwin(win, height - 2, hwidth, 1, hwidth + 1); // Header - Footer - Address

    // Draw a vertical line to separate both sub-windows
    mvwvline(win, 0, hwidth, ACS_VLINE, height);

}

static void
msg_diff_win_class_init(MsgDiffWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = msg_diff_win_constructed;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = msg_diff_win_draw;
}

static void
msg_diff_win_init(G_GNUC_UNUSED MsgDiffWindow *self)
{
}
