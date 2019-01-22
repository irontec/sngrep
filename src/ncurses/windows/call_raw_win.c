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
 * @file call_raw_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in call_raw.h
 *
 * @todo Code help screen. Please.
 * @todo Replace the panel refresh. Wclear sucks on high latency conections.
 *
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "capture/dissectors/packet_sip.h"
#include "ncurses/manager.h"
#include "ncurses/dialog.h"
#include "ncurses/windows/save_win.h"
#include "ncurses/windows/call_raw_win.h"

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param panel Ncurses panel pointer
 * @return a pointer to info structure of given panel
 */
static CallRawWinInfo *
call_raw_info(Window *window)
{
    return (CallRawWinInfo *) panel_userptr(window->panel);
}

/**
 * @brief Determine if the screen requires redrawn
 *
 * This will query the interface if it requires to be redraw again.
 *
 * @param window UI structure pointer
 * @return true if the panel requires redraw, false otherwise
 */
static gboolean
call_raw_redraw(Window *window)
{
    // Get panel information
    CallRawWinInfo *info = call_raw_info(window);
    g_return_val_if_fail(info != NULL, FALSE);

    if (info->group) {
        // Check if any of the group has changed
        return call_group_changed(info->group);
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
call_raw_print_msg(Window *window, Message *msg)
{
    int payload_lines, column, height, width;
    // Message ngrep style Header
    char header[256];
    char payload[MAX_SIP_PAYLOAD];
    int color = 0;

    // Get panel information
    CallRawWinInfo *info = call_raw_info(window);
    g_return_if_fail(info != NULL);

    // Get the pad window
    WINDOW *pad = info->pad;

    // Get current pad dimensions
    getmaxyx(pad, height, width);

    // Get message payload
    strcpy(payload, msg_get_payload(msg));

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
    if (info->padline + payload_lines > (guint) height) {
        // Create a new pad with more lines!
        pad = newpad(height + 500, COLS);
        // And copy all previous information
        overwrite(info->pad, pad);
        // Delete previous pad
        delwin(info->pad);
        // And store the new pad
        info->pad = pad;
    }

    // Color the message {
    if (setting_has_value(SETTING_COLORMODE, "request")) {
        // Determine arrow color
        if (msg_is_request(msg)) {
            color = CP_RED_ON_DEF;
        } else {
            color = CP_GREEN_ON_DEF;
        }
    } else if (info->group && setting_has_value(SETTING_COLORMODE, "callid")) {
        // Color by call-id
        color = call_group_color(info->group, msg->call);
    } else if (setting_has_value(SETTING_COLORMODE, "cseq")) {
        // Color by CSeq within the same call
        color = packet_sip_cseq(msg->packet) % 7 + 1;
    }

    // Turn on the message color
    wattron(pad, COLOR_PAIR(color));

    // Print msg header
    wattron(pad, A_BOLD);
    mvwprintw(pad, info->padline++, 0, "%s", msg_get_header(msg, header));
    wattroff(pad, A_BOLD);

    // Print msg payload
    info->padline += draw_message_pos(pad, msg, info->padline);
    // Extra line between messages
    info->padline++;

    // Set this as the last printed message
    info->last = msg;
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
call_raw_draw(Window *window)
{
    Message *msg = NULL;

    // Get panel information
    CallRawWinInfo *info = call_raw_info(window);
    g_return_val_if_fail(info != NULL, -1);

    if (info->group) {
        // Print the call group messages into the pad
        while ((msg = call_group_get_next_msg(info->group, info->last))) {
            call_raw_print_msg(window, msg);
        }
    } else {
        call_raw_win_set_msg(window, info->msg);
    }

    // Copy the visible part of the pad into the panel window
    copywin(info->pad, window->win, info->scroll, 0, 0, 0, window->height - 1, window->width - 1, 0);
    touchwin(window->win);
    return 0;
}

/**
 * @brief Move selection cursor up N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move up
 */
static void
call_raw_move_up(Window *window, guint times)
{
    // Get panel information
    CallRawWinInfo *info = call_raw_info(window);
    g_return_if_fail(info != NULL);

    if (info->scroll < times) {
        info->scroll = 0;
    } else {
        info->scroll -= times;
    }
}


/**
 * @brief Move selection cursor down N times
 *
 * @param window UI structure pointer
 * @param times number of lines to move up
 */
static void
call_raw_move_down(Window *window, guint times)
{
    // Get panel information
    CallRawWinInfo *info = call_raw_info(window);
    g_return_if_fail(info != NULL);

    info->scroll += times;

    if (info->scroll > info->padline)
        info->scroll = info->padline;
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
static int
call_raw_handle_key(Window *window, int key)
{
    guint rnpag_steps = (guint) setting_get_intvalue(SETTING_CR_SCROLLSTEP);

    CallRawWinInfo *info = call_raw_info(window);
    g_return_val_if_fail(info != NULL, KEY_NOT_HANDLED);

    // Check actions for this key
    enum KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ERR) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                call_raw_move_down(window, 1);
                break;
            case ACTION_UP:
                call_raw_move_up(window, 1);
                break;
            case ACTION_HNPAGE:
                call_raw_move_down(window, rnpag_steps / 2);
                break;
            case ACTION_NPAGE:
                call_raw_move_down(window, rnpag_steps);
                break;
            case ACTION_HPPAGE:
                call_raw_move_up(window, rnpag_steps / 2);
                break;
            case ACTION_PPAGE:
                call_raw_move_up(window, rnpag_steps);
                break;
            case ACTION_SAVE:
                if (capture_sources_count(capture_manager()) > 1) {
                    dialog_run("Saving is not possible when multiple input sources are specified.");
                    break;
                }
                if (info->group) {
                    // Display save panel
                    save_set_group(ncurses_create_window(WINDOW_SAVE), info->group);
                }
                break;
            case ACTION_TOGGLE_SYNTAX:
            case ACTION_CYCLE_COLOR:
                // Handle colors using default handler
                ncurses_default_keyhandler(window, key);
                // Create a new pad (forces messages draw)
                delwin(info->pad);
                info->pad = newpad(500, COLS);
                info->last = NULL;
                // Force refresh panel
                if (info->group) {
                    call_raw_win_set_group(window, info->group);
                } else {
                    call_raw_win_set_msg(window, info->msg);
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
call_raw_win_set_group(Window *window, CallGroup *group)
{
    CallRawWinInfo *info = call_raw_info(window);
    g_return_if_fail(info != NULL);
    g_return_if_fail(group != NULL);

    // Set call raw call group
    info->group = group;
    info->msg = NULL;

    // Initialize internal pad
    info->padline = 0;
    wclear(info->pad);
}

void
call_raw_win_set_msg(Window *window, Message *msg)
{
    CallRawWinInfo *info = call_raw_info(window);
    g_return_if_fail(info != NULL);
    g_return_if_fail(msg != NULL);

    // Set call raw message
    info->group = NULL;
    info->msg = msg;

    // Initialize internal pad
    info->padline = 0;
    wclear(info->pad);

    // Print the message in the pad
    call_raw_print_msg(window, msg);
}

void
call_raw_win_free(Window *window)
{
    CallRawWinInfo *info = call_raw_info(window);
    g_return_if_fail(info != NULL);

    // Delete panel windows
    delwin(info->pad);
    g_free(info);

    window_deinit(window);
}

Window *
call_raw_win_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_CALL_RAW;
    window->destroy = call_raw_win_free;
    window->redraw = call_raw_redraw;
    window->draw = call_raw_draw;
    window->handle_key = call_raw_handle_key;

    // Create a new panel to fill all the screen
    window_init(window, getmaxy(stdscr), getmaxx(stdscr));

    // Initialize Call Raw specific data
    CallRawWinInfo *info = g_malloc0(sizeof(CallRawWinInfo));
    set_panel_userptr(window->panel, (void *) info);

    // Create a initial pad of 1000 lines
    info->pad = newpad(500, COLS);
    info->padline = 0;
    info->scroll = 0;

    return window;
}
