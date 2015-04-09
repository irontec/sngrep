/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2015 Irontec SL. All rights reserved.
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
 * @file group.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in keybinding.h
 *
 */

#include "config.h"
#include <string.h>
#include "ui_manager.h"
#include "keybinding.h"

//! sngrep keybindings
key_binding_t bindings[ACTION_SENTINEL];

void
key_bindings_init()
{
    // Initialize bindings structure
    memset(bindings, 0, sizeof(key_binding_t) * ACTION_SENTINEL);

    // Set default keybindings
    key_bind_action(ACTION_UP, KEY_UP);
    key_bind_action(ACTION_UP, 'j');
    key_bind_action(ACTION_DOWN, KEY_DOWN);
    key_bind_action(ACTION_DOWN, 'k');
    key_bind_action(ACTION_LEFT, KEY_LEFT);
    key_bind_action(ACTION_RIGHT, KEY_RIGHT);
    key_bind_action(ACTION_DELETE, KEY_DC);
    key_bind_action(ACTION_BACKSPACE, KEY_BACKSPACE);
    key_bind_action(ACTION_BACKSPACE, KEY_BACKSPACE2);
    key_bind_action(ACTION_BACKSPACE, KEY_BACKSPACE3);
    key_bind_action(ACTION_NPAGE, KEY_NPAGE);
    key_bind_action(ACTION_NPAGE, KEY_CTRL_F);
    key_bind_action(ACTION_PPAGE, KEY_PPAGE);
    key_bind_action(ACTION_PPAGE, KEY_CTRL_B);
    key_bind_action(ACTION_HNPAGE, KEY_CTRL_D);
    key_bind_action(ACTION_HPPAGE, KEY_CTRL_U);
    key_bind_action(ACTION_BEGIN, KEY_HOME);
    key_bind_action(ACTION_BEGIN, KEY_CTRL_A);
    key_bind_action(ACTION_END, KEY_END);
    key_bind_action(ACTION_END, KEY_CTRL_E);
    key_bind_action(ACTION_PREV_FIELD, KEY_TAB);
    key_bind_action(ACTION_NEXT_FIELD, KEY_TAB);
    key_bind_action(ACTION_NEXT_FIELD, KEY_DOWN);
    key_bind_action(ACTION_RESIZE_SCREEN, KEY_RESIZE);
    key_bind_action(ACTION_CLEAR, KEY_CTRL_U);
    key_bind_action(ACTION_CLEAR_CALLS, KEY_F(5));
    key_bind_action(ACTION_TOGGLE_SYNTAX, KEY_F(8));
    key_bind_action(ACTION_TOGGLE_SYNTAX, 'C');
    key_bind_action(ACTION_CYCLE_COLOR, KEY_F(7));
    key_bind_action(ACTION_CYCLE_COLOR, 'c');
    key_bind_action(ACTION_SHOW_HOSTNAMES, KEY_F(9));
    key_bind_action(ACTION_TOGGLE_PAUSE, 'p');
    key_bind_action(ACTION_PREV_SCREEN, KEY_ESC);
    key_bind_action(ACTION_PREV_SCREEN, 'Q');
    key_bind_action(ACTION_PREV_SCREEN, 'q');
    key_bind_action(ACTION_SHOW_HELP, KEY_F(1));
    key_bind_action(ACTION_SHOW_HELP, 'H');
    key_bind_action(ACTION_SHOW_HELP, 'h');
    key_bind_action(ACTION_SHOW_HELP, '?');
    key_bind_action(ACTION_SHOW_RAW, KEY_F(6));
    key_bind_action(ACTION_SHOW_RAW, 'r');
    key_bind_action(ACTION_SHOW_RAW, 'R');
    key_bind_action(ACTION_SHOW_FLOW, KEY_INTRO);
    key_bind_action(ACTION_SHOW_FLOW_EX, KEY_F(4));
    key_bind_action(ACTION_SHOW_FLOW_EX, 'x');
    key_bind_action(ACTION_SHOW_FLOW_EX, 'X');
    key_bind_action(ACTION_SHOW_FILTERS, KEY_F(7));
    key_bind_action(ACTION_SHOW_FILTERS, 'f');
    key_bind_action(ACTION_SHOW_FILTERS, 'F');
    key_bind_action(ACTION_SHOW_COLUMNS, KEY_F(10));
    key_bind_action(ACTION_SHOW_COLUMNS, 't');
    key_bind_action(ACTION_SHOW_COLUMNS, 'T');
    key_bind_action(ACTION_COLUMN_MOVE_UP, '-');
    key_bind_action(ACTION_COLUMN_MOVE_DOWN, '+');
    key_bind_action(ACTION_DISP_FILTER, KEY_F(3));
    key_bind_action(ACTION_DISP_FILTER, KEY_TAB);
    key_bind_action(ACTION_DISP_FILTER, '/');
    key_bind_action(ACTION_DISP_INVITE, 'i');
    key_bind_action(ACTION_DISP_INVITE, 'I');
    key_bind_action(ACTION_SAVE, KEY_F(2));
    key_bind_action(ACTION_SAVE, 's');
    key_bind_action(ACTION_SAVE, 'S');
    key_bind_action(ACTION_SELECT, ' ');
    key_bind_action(ACTION_CONFIRM, KEY_INTRO);
    key_bind_action(ACTION_TOGGLE_RTP, 'f');
    key_bind_action(ACTION_TOGGLE_RAW, KEY_F(3));
    key_bind_action(ACTION_TOGGLE_RAW, 't');
    key_bind_action(ACTION_INCREASE_RAW, '9');
    key_bind_action(ACTION_DECREASE_RAW, '0');
    key_bind_action(ACTION_RESET_RAW, 'T');
    key_bind_action(ACTION_ONLY_SDP, 'D');
    key_bind_action(ACTION_SDP_INFO, KEY_F(2));
    key_bind_action(ACTION_SDP_INFO, 'd');
    key_bind_action(ACTION_COMPRESS, KEY_F(5));
    key_bind_action(ACTION_COMPRESS, 's');

}

void
key_bind_action(int action, int key)
{
    if (bindings[action].bindcnt == MAX_BINDINGS)
        return;

    bindings[action].keys[bindings[action].bindcnt++] = key;
}

int
key_find_action(int key, int start)
{
    int i, j;
    for (i = start + 1; i < ACTION_SENTINEL; i++) {

        if (i == ACTION_PRINTABLE && key_is_printable(key))
            return ACTION_PRINTABLE;

        for (j = 0; j < bindings[i].bindcnt; j++)
            if (bindings[i].keys[j] == key)
                return i;
    }
    return -1;
}

int
key_is_printable(int key)
{
    return (key > 33 && key < 126) || (key > 160 && key < 255);
}
