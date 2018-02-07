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
 * @file group.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in keybinding.h
 *
 */

#include "config.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "setting.h"
#include "keybinding.h"
#include "curses/ui_manager.h"

//! sngrep keybindings
key_binding_t bindings[ACTION_SENTINEL] = {
   { ACTION_PRINTABLE,      "",             { }, 0 },
   { ACTION_UP,             "up",           { KEY_UP, 'k' }, 2 },
   { ACTION_DOWN,           "down",         { KEY_DOWN, 'j' }, 2 },
   { ACTION_LEFT,           "left",         { KEY_LEFT, 'h' }, 2 },
   { ACTION_RIGHT,          "right",        { KEY_RIGHT, 'l'}, 2 },
   { ACTION_DELETE,         "delete",       { KEY_DC }, 1 },
   { ACTION_BACKSPACE,      "backspace",    { KEY_BACKSPACE, KEY_BACKSPACE2, KEY_BACKSPACE3 }, 3 },
   { ACTION_NPAGE,          "npage",        { KEY_NPAGE, KEY_CTRL('F') }, 2 },
   { ACTION_PPAGE,          "ppage",        { KEY_PPAGE, KEY_CTRL('B') }, 2 },
   { ACTION_HNPAGE,         "hnpage",       { KEY_CTRL('D') }, 1 },
   { ACTION_HPPAGE,         "hppage",       { KEY_CTRL('U') }, 2 },
   { ACTION_BEGIN,          "begin",        { KEY_HOME, KEY_CTRL('A') }, 2 },
   { ACTION_END,            "end",          { KEY_END, KEY_CTRL('E') }, 2 },
   { ACTION_PREV_FIELD,     "pfield",       { KEY_UP }, 1 },
   { ACTION_NEXT_FIELD,     "nfield",       { KEY_DOWN, KEY_TAB }, 2 },
   { ACTION_RESIZE_SCREEN,  "",             { KEY_RESIZE }, 1 },
   { ACTION_CLEAR,          "clear",        { KEY_CTRL('U'), KEY_CTRL('W')}, 2 },
   { ACTION_CLEAR_CALLS,    "clearcalls",   { KEY_F(5), KEY_CTRL('L')}, 2 },
   { ACTION_CLEAR_CALLS_SOFT, "clearcallssoft", {KEY_F(9)}, 2 },
   { ACTION_TOGGLE_SYNTAX,  "togglesyntax", { KEY_F(8), 'C' }, 2 },
   { ACTION_CYCLE_COLOR,    "colormode",    { 'c' }, 1 },
   { ACTION_COMPRESS,       "compress",     { 's' }, 1 },
   { ACTION_SHOW_ALIAS,     "togglealias",  { 'a' }, 1 },
   { ACTION_TOGGLE_PAUSE,   "pause",        { 'p' }, 1 },
   { ACTION_PREV_SCREEN,    "prevscreen",   { KEY_ESC, 'q', 'Q' }, 3 },
   { ACTION_SHOW_HELP,      "help",         { KEY_F(1), 'h', 'H', '?' }, 4 },
   { ACTION_SHOW_RAW,       "raw",          { KEY_F(6), 'R', 'r' }, 3 },
   { ACTION_SHOW_FLOW,      "flow",         { KEY_INTRO }, 1 },
   { ACTION_SHOW_FLOW_EX,   "flowex",       { KEY_F(4), 'x' }, 2 },
   { ACTION_SHOW_FILTERS,   "filters",      { KEY_F(7), 'f', 'F' }, 3 },
   { ACTION_SHOW_COLUMNS,   "columns",      { KEY_F(10), 't', 'T' }, 3 },
   { ACTION_SHOW_SETTINGS,  "settings",     { KEY_F(8), 'o', 'O' }, 3 },
   { ACTION_SHOW_STATS,     "stats",        { 'i' }, 1 },
   { ACTION_COLUMN_MOVE_UP, "columnup",     { '-' }, 1 },
   { ACTION_COLUMN_MOVE_DOWN, "columndown", { '+' }, 1 },
   { ACTION_SDP_INFO,       "sdpinfo",      { KEY_F(2), 'd' }, 2 },
   { ACTION_DISP_FILTER,    "search",       { KEY_F(3), '/', KEY_TAB }, 3 },
   { ACTION_SAVE,           "save",         { KEY_F(2), 's', 'S'}, 3 },
   { ACTION_SELECT,         "select",       { KEY_SPACE }, 1 },
   { ACTION_CONFIRM,        "confirm",      { KEY_INTRO }, 1 },
   { ACTION_TOGGLE_MEDIA,   "togglemedia",  { KEY_F(3), 'm' }, 2 },
   { ACTION_ONLY_MEDIA,     "onlymedia",    { 'M' }, 1 },
   { ACTION_TOGGLE_RAW,     "rawpreview",   { 't' }, 1 },
   { ACTION_INCREASE_RAW,   "morerawpreview", { '9' }, 1 },
   { ACTION_DECREASE_RAW,   "lessrawpreview", { '0' }, 1 },
   { ACTION_RESET_RAW,      "resetrawpreview", { 'T' }, 1 },
   { ACTION_ONLY_SDP,       "onlysdp",      { 'D' }, 1 },
   { ACTION_AUTOSCROLL,     "autoscroll",   { 'A' }, 1 },
   { ACTION_TOGGLE_HINT,    "hintalt",      { 'K' }, 1 },
   { ACTION_SORT_PREV,      "sortprev",     { '<' }, 1 },
   { ACTION_SORT_NEXT,      "sortnext",     { '>' }, 1 },
   { ACTION_SORT_SWAP,      "sortswap",     { 'z' }, 1 },
   { ACTION_TOGGLE_TIME,    "toggletime",   { 'w' }, 1 },
};

void
key_bindings_dump()
{
    int i, j;
    for (i = 1; i < ACTION_SENTINEL; i++) {
        for (j = 0; j < bindings[i].bindcnt; j++) {
            printf("ActionID: %d\t ActionName: %-21s Key: %d (%s)\n",
                   bindings[i].id,
                   bindings[i].name,
                   bindings[i].keys[j],
                   key_to_str(bindings[i].keys[j]));
        }
    }
}

key_binding_t *
key_binding_data(int action)
{
    int i;
    for (i = 1; i < ACTION_SENTINEL; i++) {
        if (bindings[i].id == action)
            return &bindings[i];
    }

    return NULL;
}

void
key_bind_action(int action, int key)
{
    key_binding_t *bind;

    if (!(bind = key_binding_data(action)))
        return;

    if (bind->bindcnt == MAX_BINDINGS)
        return;

    bind->keys[bind->bindcnt++] = key;
}

void
key_unbind_action(int action, int key)
{
    key_binding_t tmp, *bind;
    int i;

    // Action is not valid
    if (!(bind = key_binding_data(action)))
        return;

    // Copy binding to temporal struct
    memcpy(&tmp, bind, sizeof(key_binding_t));

    // Reset bindings for this action
    memset(&bind->keys, 0, sizeof(int) * MAX_BINDINGS);

    // Add all bindings but the unbinded
    for (i=0; i < tmp.bindcnt; i++) {
        if (tmp.keys[i] != key) {
            key_bind_action(action, tmp.keys[i]);
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
                return bindings[i].id;
    }
    return -1;
}

int
key_action_id(const char *action)
{
    int i;
    for (i = 1; i < ACTION_SENTINEL; i++) {
        if (!strcasecmp(action, bindings[i].name))
            return bindings[i].id;

    }
    return -1;
}

int
key_is_printable(int key)
{
    return key == ' ' || (key > 33 && key < 126) || (key > 160 && key < 255);
}

const char *
key_to_str(int key)
{
    //! Check function keys and Special keys
    switch(key) {
        case KEY_F(1):
            return "F1";
        case KEY_F(2):
            return "F2";
        case KEY_F(3):
            return "F3";
        case KEY_F(4):
            return "F4";
        case KEY_F(5):
            return "F5";
        case KEY_F(6):
            return "F6";
        case KEY_F(7):
            return "F7";
        case KEY_F(8):
            return "F8";
        case KEY_F(9):
            return "F9";
        case KEY_F(10):
            return "F10";
        case KEY_ESC:
            return "Esc";
        case KEY_INTRO:
            return "Enter";
        case ' ':
            return "Space";
        default:
            if (key_is_printable(key))
                return keyname(key);
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
    key_binding_t *bind;

    if (!(bind = key_binding_data(action)))
        return NULL;

    if (setting_enabled(SETTING_ALTKEY_HINT) && bind->bindcnt > 1) {
        // First alt keybinding
        return key_to_str(bind->keys[1]);
    } else {
        // Default keybinding
        return key_to_str(bind->keys[0]);
    }
}

int
key_action_key(int action)
{
    key_binding_t *bind;

    if (!(bind = key_binding_data(action)))
        return -1;

    if (setting_enabled(SETTING_ALTKEY_HINT) && bind->bindcnt > 1) {
        // First alt keybinding
        return bind->keys[1];
    } else {
        // Default keybinding
        return bind->keys[0];
    }

    return -1;
}
