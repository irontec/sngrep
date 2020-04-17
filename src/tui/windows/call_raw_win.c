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
 * @file call_raw_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in call_raw.h
 *
 * @todo Code help screen. Please.
 * @todo Replace the panel refresh. Wclear sucks on high latency connections.
 *
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "packet/packet_sip.h"
#include "tui/tui.h"
#include "tui/dialog.h"
#include "tui/windows/save_win.h"
#include "tui/windows/call_raw_win.h"


G_DEFINE_TYPE(CallRawWindow , call_raw_win, SNG_TYPE_WINDOW)

/**
 * @brief Determine if the screen requires redrawn
 *
 * This will query the interface if it requires to be redraw again.
 *
 * @param window UI structure pointer
 * @return true if the panel requires redraw, false otherwise
 */
static gboolean
call_raw_win_redraw(SngAppWindow *window)
{
    // Get panel information
    CallRawWindow *self = TUI_CALL_RAW(window);

    if (self->group) {
        // Check if any of the group has changed
        return call_group_changed(self->group);
    }

    return FALSE;
}

/**
 * @brief Draw a message in call Raw
 *
 * Draw a new message in the Raw pad.
 *
 * @param panel Ncurses panel pointer
 * @param msg New message to be printed
 */
static void
call_raw_win_print_msg(SngAppWindow *window, Message *msg)
{
    int payload_lines, column, height, width;
    // Message ngrep style Header
    char header[256];
    int color = 0;

    // Get panel information
    CallRawWindow *self = TUI_CALL_RAW(window);
    g_return_if_fail(self != NULL);

    // Get the pad window
    WINDOW *pad = self->pad;

    // Get current pad dimensions
    getmaxyx(pad, height, width);

    // Get message payload
    g_autofree gchar *payload = msg_get_payload(msg);

    // Check how many lines we well need to draw this message
    payload_lines = 0;
    column = 0;
    for (guint i = 0; i < strlen(payload); i++) {
        if (column == width || payload[i] == '\n') {
            payload_lines++;
            column = 0;
            continue;
        }
        column++;
    }

    // Check if we have enough space in our huge pad to store this message
    if (self->padline + payload_lines > (guint) height) {
        // Create a new pad with more lines!
        pad = newpad(height + 500, COLS);
        // And copy all previous information
        overwrite(self->pad, pad);
        // Delete previous pad
        delwin(self->pad);
        // And store the new pad
        self->pad = pad;
    }

    // Color the message {
    if (setting_get_enum(SETTING_TUI_COLORMODE) == SETTING_COLORMODE_REQUEST) {
        // Determine arrow color
        if (msg_is_request(msg)) {
            color = CP_RED_ON_DEF;
        } else {
            color = CP_GREEN_ON_DEF;
        }
    } else if (self->group && setting_get_enum(SETTING_TUI_COLORMODE) == SETTING_COLORMODE_CALLID) {
        // Color by call-id
        color = call_group_color(self->group, msg->call);
    } else if (setting_get_enum(SETTING_TUI_COLORMODE) == SETTING_COLORMODE_CSEQ) {
        // Color by CSeq within the same call
        color = msg_get_cseq(msg) % 7 + 1;
    }

    // Turn on the message color
    wattron(pad, COLOR_PAIR(color));

    // Print msg header
    wattron(pad, A_BOLD);
    mvwprintw(pad, self->padline++, 0, "%s", msg_get_header(msg, header));
    wattroff(pad, A_BOLD);

    // Print msg payload
    self->padline += draw_message_pos(pad, msg, self->padline);
    // Extra line between messages
    self->padline++;

    // Set this as the last printed message
    self->last = msg;
}

/**
 * @brief Draw the Call Raw panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
static gint
call_raw_win_draw(SngWidget *widget)
{
    Message *msg = NULL;

    // Get panel information
    SngAppWindow *window = SNG_APP_WINDOW(widget);
    CallRawWindow *self = TUI_CALL_RAW(window);
    g_return_val_if_fail(self != NULL, -1);
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    gint height = sng_widget_get_height(widget);
    gint width = sng_widget_get_width(widget);

    if (self->group) {
        // Print the call group messages into the pad
        while ((msg = call_group_get_next_msg(self->group, self->last))) {
            call_raw_win_print_msg(window, msg);
        }
    } else {
        call_raw_win_set_msg(window, self->msg);
    }

    // Copy the visible part of the pad into the panel window
    copywin(self->pad, win, self->scroll, 0, 0, 0, height - 1, width - 1, 0);
    touchwin(win);
    return 0;
}

/**
 * @brief Move selection cursor up N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move up
 */
static void
call_raw_win_move_up(SngAppWindow *window, guint times)
{
    // Get panel information
    CallRawWindow *self = TUI_CALL_RAW(window);
    g_return_if_fail(self != NULL);

    if (self->scroll < times) {
        self->scroll = 0;
    } else {
        self->scroll -= times;
    }
}


/**
 * @brief Move selection cursor down N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move up
 */
static void
call_raw_win_move_down(SngAppWindow *window, guint times)
{
    // Get panel information
    CallRawWindow *self = TUI_CALL_RAW(window);
    g_return_if_fail(self != NULL);

    self->scroll += times;

    if (self->scroll > self->padline)
        self->scroll = self->padline;
}

/**
 * @brief Handle Call Raw key strokes
 *
 * This function will manage the custom keybindings of the panel.
 * This function return one of the values defined in @key_handler_ret
 *
 * @param panel Ncurses panel pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
static gint
call_raw_win_handle_key(SngWidget *widget, int key)
{
    guint rnpag_steps = (guint) setting_get_intvalue(SETTING_TUI_CR_SCROLLSTEP);

    SngAppWindow *app_window = SNG_APP_WINDOW(widget);
    CallRawWindow *self = TUI_CALL_RAW(app_window);
    g_return_val_if_fail(self != NULL, KEY_NOT_HANDLED);

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                call_raw_win_move_down(app_window, 1);
                break;
            case ACTION_UP:
                call_raw_win_move_up(app_window, 1);
                break;
            case ACTION_HNPAGE:
                call_raw_win_move_down(app_window, rnpag_steps / 2);
                break;
            case ACTION_NPAGE:
                call_raw_win_move_down(app_window, rnpag_steps);
                break;
            case ACTION_HPPAGE:
                call_raw_win_move_up(app_window, rnpag_steps / 2);
                break;
            case ACTION_PPAGE:
                call_raw_win_move_up(app_window, rnpag_steps);
                break;
            case ACTION_SAVE:
                if (self->group) {
                    // Display save panel
                    save_set_group(tui_create_app_window(SNG_WINDOW_TYPE_SAVE), self->group);
                }
                break;
            case ACTION_TOGGLE_SYNTAX:
            case ACTION_CYCLE_COLOR:
                // Handle colors using default handler
                tui_default_keyhandler(SNG_WINDOW(app_window), key);
                // Create a new pad (forces messages draw)
                delwin(self->pad);
                self->pad = newpad(500, COLS);
                self->last = NULL;
                // Force refresh panel
                if (self->group) {
                    call_raw_win_set_group(app_window, self->group);
                } else {
                    call_raw_win_set_msg(app_window, self->msg);
                }
                break;
            case ACTION_CLEAR_CALLS:
            case ACTION_CLEAR_CALLS_SOFT:
                // Propagate the key to the previous panel
                return KEY_PROPAGATED;
            default:
                // Parse next action
                continue;
        }

        // This panel has handled the key successfully
        break;
    }


    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

void
call_raw_win_set_group(SngAppWindow *window, CallGroup *group)
{
    CallRawWindow *self = TUI_CALL_RAW(window);
    g_return_if_fail(self != NULL);
    g_return_if_fail(group != NULL);

    // Set call raw call group
    self->group = group;
    self->msg = NULL;

    // Initialize internal pad
    self->padline = 0;
    wclear(self->pad);
}

void
call_raw_win_set_msg(SngAppWindow *window, Message *msg)
{
    CallRawWindow *self = TUI_CALL_RAW(window);
    g_return_if_fail(self != NULL);
    g_return_if_fail(msg != NULL);

    // Set call raw message
    self->group = NULL;
    self->msg = msg;

    // Initialize internal pad
    self->padline = 0;
    wclear(self->pad);

    // Print the message in the pad
    call_raw_win_print_msg(window, msg);
}

void
call_raw_win_free(SngAppWindow *window)
{
    g_object_unref(window);
}

SngAppWindow *
call_raw_win_new()
{
    return g_object_new(
        WINDOW_TYPE_CALL_RAW,
        "window-type", SNG_WINDOW_TYPE_CALL_RAW,
        "height", getmaxy(stdscr),
        "width", getmaxx(stdscr),
        NULL
    );
}

static void
call_raw_win_finalize(GObject *object)
{
    CallRawWindow *self = TUI_CALL_RAW(object);

    // Delete panel windows
    delwin(self->pad);

    // Chain-up parent finalize
    G_OBJECT_CLASS(call_raw_win_parent_class)->finalize(object);
}

static void
call_raw_win_class_init(CallRawWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = call_raw_win_finalize;

    SngAppWindowClass *window_class = SNG_APP_WINDOW_CLASS(klass);
    window_class->redraw = call_raw_win_redraw;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = call_raw_win_draw;
    widget_class->key_pressed = call_raw_win_handle_key;

}

static void
call_raw_win_init(CallRawWindow *self)
{
    // Create a initial pad of 1000 lines
    self->pad = newpad(500, COLS);
    self->padline = 0;
    self->scroll = 0;
}
