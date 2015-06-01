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
 * @file option.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage keybindings
 *
 * sngrep keybindings are associated with actions. Each action can store multiple
 * keybindings.
 * Keybindings configured by user using *key* directive of sngreprc file,
 * in the format:
 *
 *   key ui_action keycode
 *
 * keycode must be a letter (lowercase or uppercase) or a ^ sign with an uppercase
 * letter when Ctrl modifier is used.
 *
 */

#ifndef __SNGREP_KEYBINDING_H_
#define __SNGREP_KEYBINDING_H_

//! Number of keybindings per action
#define MAX_BINDINGS    5

//! Some undefined key codes
#define KEY_CTRL(n)     ((n)-64)
#define KEY_ESC         27
#define KEY_INTRO       10
#define KEY_TAB         9
#define KEY_BACKSPACE2  8
#define KEY_BACKSPACE3  127

/**
 * @brief Available Key actions
 */
enum key_actions
{
    ACTION_PRINTABLE = 0,
    ACTION_UP,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_DELETE,
    ACTION_BACKSPACE,
    ACTION_NPAGE,
    ACTION_PPAGE,
    ACTION_HNPAGE,
    ACTION_HPPAGE,
    ACTION_BEGIN,
    ACTION_END,
    ACTION_PREV_FIELD,
    ACTION_NEXT_FIELD,
    ACTION_RESIZE_SCREEN,
    ACTION_CLEAR,
    ACTION_CLEAR_CALLS,
    ACTION_TOGGLE_SYNTAX,
    ACTION_CYCLE_COLOR,
    ACTION_SHOW_HOSTNAMES,
    ACTION_SHOW_ALIAS,
    ACTION_TOGGLE_PAUSE,
    ACTION_PREV_SCREEN,
    ACTION_SHOW_HELP,
    ACTION_SHOW_RAW,
    ACTION_SHOW_FLOW,
    ACTION_SHOW_FLOW_EX,
    ACTION_SHOW_FILTERS,
    ACTION_SHOW_COLUMNS,
    ACTION_COLUMN_MOVE_UP,
    ACTION_COLUMN_MOVE_DOWN,
    ACTION_DISP_FILTER,
    ACTION_DISP_INVITE,
    ACTION_SAVE,
    ACTION_SELECT,
    ACTION_CONFIRM,
    ACTION_TOGGLE_RTP,
    ACTION_TOGGLE_RAW,
    ACTION_INCREASE_RAW,
    ACTION_DECREASE_RAW,
    ACTION_RESET_RAW,
    ACTION_ONLY_SDP,
    ACTION_SDP_INFO,
    ACTION_COMPRESS,
    ACTION_TOGGLE_HINT,
    ACTION_SENTINEL
};

//! Shorter declaration of key_binding structure
typedef struct key_binding key_binding_t;

/**
 * @brief Struct to hold a keybinding data
 */
struct key_binding
{
    //! keybindings for this action
    int keys[MAX_BINDINGS];
    //! How many keys are binded to this action
    int bindcnt;
};

/**
 * @brief Initialize default keybindings
 */
void
key_bindings_init();

/**
 * @brief Bind a key to an action
 */
void
key_bind_action(int action, int key);

/**
 * @brief Unbind a key to an action
 */
void
key_unbind_action(int action, int key);

/**
 * @brief Find the next action for a given key
 *
 * Set start parameter to -1 for start searching the
 * first action.
 */
int
key_find_action(int key, int start);

/**
 * @brief Return the action id associate to an action str
 */
int
key_action_id(const char *action);

/**
 * @brief Check if key is a printable ascii character
 */
int
key_is_printable(int key);

/**
 * @brief Return a Human readable representation of a key
 */
const char *
key_to_str(int key);

/**
 * @brief Parse Human key declaration to curses key
 */
int
key_from_str(const char *key);

/**
 * @brief Return Human readable key for an action
 */
const char *
key_action_key_str(int action);

#endif /* __SNGREP_KEYBINDING_H_ */
