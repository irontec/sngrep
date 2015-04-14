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
#include <ctype.h>
#include <string.h>
#include "ui_manager.h"
#include "option.h"
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
    key_bind_action(ACTION_NPAGE, KEY_CTRL('F'));
    key_bind_action(ACTION_PPAGE, KEY_PPAGE);
    key_bind_action(ACTION_PPAGE, KEY_CTRL('B'));
    key_bind_action(ACTION_HNPAGE, KEY_CTRL('D'));
    key_bind_action(ACTION_HPPAGE, KEY_CTRL('U'));
    key_bind_action(ACTION_BEGIN, KEY_HOME);
    key_bind_action(ACTION_BEGIN, KEY_CTRL('A'));
    key_bind_action(ACTION_END, KEY_END);
    key_bind_action(ACTION_END, KEY_CTRL('E'));
    key_bind_action(ACTION_PREV_FIELD, KEY_TAB);
    key_bind_action(ACTION_NEXT_FIELD, KEY_TAB);
    key_bind_action(ACTION_NEXT_FIELD, KEY_DOWN);
    key_bind_action(ACTION_RESIZE_SCREEN, KEY_RESIZE);
    key_bind_action(ACTION_CLEAR, KEY_CTRL('U'));
    key_bind_action(ACTION_CLEAR_CALLS, KEY_F(5));
    key_bind_action(ACTION_TOGGLE_SYNTAX, KEY_F(8));
    key_bind_action(ACTION_TOGGLE_SYNTAX, 'C');
    key_bind_action(ACTION_CYCLE_COLOR, KEY_F(7));
    key_bind_action(ACTION_CYCLE_COLOR, 'c');
    key_bind_action(ACTION_SHOW_HOSTNAMES, KEY_F(9));
    key_bind_action(ACTION_TOGGLE_PAUSE, 'p');
    key_bind_action(ACTION_PREV_SCREEN, KEY_ESC);
    key_bind_action(ACTION_PREV_SCREEN, 'q');
    key_bind_action(ACTION_PREV_SCREEN, 'Q');
    key_bind_action(ACTION_SHOW_HELP, KEY_F(1));
    key_bind_action(ACTION_SHOW_HELP, 'h');
    key_bind_action(ACTION_SHOW_HELP, 'H');
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
    key_bind_action(ACTION_DISP_FILTER, '/');
    key_bind_action(ACTION_DISP_FILTER, KEY_TAB);
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
    key_bind_action(ACTION_TOGGLE_HINT, 'K');
}

void
key_bind_action(int action, int key)
{
    if (action < 0)
        return;

    if (bindings[action].bindcnt == MAX_BINDINGS)
        return;

    bindings[action].keys[bindings[action].bindcnt++] = key;
}

void
key_unbind_action(int action, int key)
{
    key_binding_t bind;
    int i;

    // Action is not valid
    if (action < 0)
        return;

    // Copy binding to temporal struct
    memcpy(&bind, &bindings[action], sizeof(key_binding_t));
    // Reset bindings for this action
    memset(&bindings[action], 0, sizeof(key_binding_t));

    // Add all bindings but the unbinded
    for (i=0; i < bind.bindcnt; i++) {
        if (bind.keys[i] != key) {
            key_bind_action(action, bind.keys[i]);
        }
    }
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
key_action_id(const char *action)
{

    if (!strcmp(action, "up")) return ACTION_UP;
    if (!strcmp(action, "down")) return ACTION_DOWN;
    if (!strcmp(action, "left")) return ACTION_LEFT;
    if (!strcmp(action, "right")) return ACTION_RIGHT;
    if (!strcmp(action, "delete")) return ACTION_DELETE;
    if (!strcmp(action, "backspace")) return ACTION_BACKSPACE;
    if (!strcmp(action, "npage")) return ACTION_NPAGE;
    if (!strcmp(action, "ppage")) return ACTION_PPAGE;
    if (!strcmp(action, "hnpage")) return ACTION_HNPAGE;
    if (!strcmp(action, "hppage")) return ACTION_HPPAGE;
    if (!strcmp(action, "begin")) return ACTION_BEGIN;
    if (!strcmp(action, "end")) return ACTION_END;
    if (!strcmp(action, "pfield")) return ACTION_PREV_FIELD;
    if (!strcmp(action, "nfield")) return ACTION_NEXT_FIELD;
    if (!strcmp(action, "clear")) return ACTION_CLEAR;
    if (!strcmp(action, "clearcalls")) return ACTION_CLEAR_CALLS;
    if (!strcmp(action, "togglesyntax")) return ACTION_TOGGLE_SYNTAX;
    if (!strcmp(action, "colormode")) return ACTION_CYCLE_COLOR;
    if (!strcmp(action, "togglehostname")) return ACTION_SHOW_HOSTNAMES;
    if (!strcmp(action, "pause")) return ACTION_TOGGLE_PAUSE;
    if (!strcmp(action, "prevscreen")) return ACTION_PREV_SCREEN;
    if (!strcmp(action, "help")) return ACTION_SHOW_HELP;
    if (!strcmp(action, "raw")) return ACTION_SHOW_RAW;
    if (!strcmp(action, "flow")) return ACTION_SHOW_FLOW;
    if (!strcmp(action, "flowex")) return ACTION_SHOW_FLOW_EX;
    if (!strcmp(action, "filters")) return ACTION_SHOW_FILTERS;
    if (!strcmp(action, "columns")) return ACTION_SHOW_COLUMNS;
    if (!strcmp(action, "columnup")) return ACTION_COLUMN_MOVE_UP;
    if (!strcmp(action, "columndown")) return ACTION_COLUMN_MOVE_DOWN;
    if (!strcmp(action, "search")) return ACTION_DISP_FILTER;
    if (!strcmp(action, "save")) return ACTION_SAVE;
    if (!strcmp(action, "select")) return ACTION_SELECT;
    if (!strcmp(action, "confirm")) return ACTION_CONFIRM;
    if (!strcmp(action, "rtp")) return ACTION_TOGGLE_RTP;
    if (!strcmp(action, "rawpreview")) return ACTION_TOGGLE_RAW;
    if (!strcmp(action, "morerawpreview")) return ACTION_INCREASE_RAW;
    if (!strcmp(action, "lessrawpreview")) return ACTION_DECREASE_RAW;
    if (!strcmp(action, "resetrawpreview")) return ACTION_RESET_RAW;
    if (!strcmp(action, "onlysdp")) return ACTION_ONLY_SDP;
    if (!strcmp(action, "sdpinfo")) return ACTION_SDP_INFO;
    if (!strcmp(action, "compress")) return ACTION_COMPRESS;
    if (!strcmp(action, "hintalt")) return ACTION_TOGGLE_HINT;
    return -1;
}

int
key_is_printable(int key)
{
    return (key > 33 && key < 126) || (key > 160 && key < 255);
}

const char *
key_to_str(int key)
{
    //! Check if we already have a human readable key
    if (key_is_printable(key))
        return keyname(key);

    //! Check function keys and Special keys
    switch(key) {
        case KEY_F(1): return "F1";
        case KEY_F(2): return "F2";
        case KEY_F(3): return "F3";
        case KEY_F(4): return "F4";
        case KEY_F(5): return "F5";
        case KEY_F(6): return "F6";
        case KEY_F(7): return "F7";
        case KEY_F(8): return "F8";
        case KEY_F(9): return "F9";
        case KEY_F(10): return "F10";
        case KEY_ESC: return "Esc";
        case KEY_INTRO: return "Enter";
        case ' ': return "Space";
    }

    return "";
}

int
key_from_str(const char *key)
{
    if (!key)
        return 0;

    // Single character string
    if (strlen(key) == 1)
        return *key;

    // Function keys
    if (*key == 'F')
        return KEY_F(atoi(key+1));

    // Control Secuences
    if (*key == '^')
        return KEY_CTRL(toupper(*(key+1)));
    if (!strncasecmp(key, "Ctrl-", 5))
        return KEY_CTRL(toupper(*(key+5)));

    // Special Name characters
    if (!strcasecmp(key, "Esc"))
        return KEY_ESC;
    if (!strcasecmp(key, "Space"))
        return ' ';
    if (!strcasecmp(key, "Enter"))
        return KEY_INTRO;

    return 0;
}

const char *
key_action_key_str(int action)
{
    if (is_option_enabled("hintkeyalt") && bindings[action].bindcnt > 1) {
        // First alt keybinding
        return key_to_str(bindings[action].keys[1]);
    } else {
        // Default keybinding
        return key_to_str(bindings[action].keys[0]);
    }
}
