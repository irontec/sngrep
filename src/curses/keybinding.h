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
#define KEY_SPACE       ' '

/**
 * @brief Available Key actions
 */
enum key_actions {
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
    ACTION_CLEAR_CALLS_SOFT,
    ACTION_TOGGLE_SYNTAX,
    ACTION_CYCLE_COLOR,
    ACTION_COMPRESS,
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
    ACTION_SHOW_SETTINGS,
    ACTION_SHOW_STATS,
    ACTION_COLUMN_MOVE_UP,
    ACTION_COLUMN_MOVE_DOWN,
    ACTION_SDP_INFO,
    ACTION_DISP_FILTER,
    ACTION_SAVE,
    ACTION_SELECT,
    ACTION_CONFIRM,
    ACTION_TOGGLE_MEDIA,
    ACTION_ONLY_MEDIA,
    ACTION_TOGGLE_RAW,
    ACTION_INCREASE_RAW,
    ACTION_DECREASE_RAW,
    ACTION_RESET_RAW,
    ACTION_ONLY_SDP,
    ACTION_TOGGLE_HINT,
    ACTION_AUTOSCROLL,
    ACTION_SORT_PREV,
    ACTION_SORT_NEXT,
    ACTION_SORT_SWAP,
    ACTION_TOGGLE_TIME,
    ACTION_SENTINEL
};

//! Shorter declaration of key_binding structure
typedef struct key_binding key_binding_t;

/**
 * @brief Struct to hold a keybinding data
 */
struct key_binding {
    //! Keybinding action id
    int id;
    //! Keybinding action name
    const char *name;
    //! keybindings for this action
    int keys[MAX_BINDINGS];
    //! How many keys are binded to this action
    int bindcnt;
};

/**
 * @brief Print configured keybindigs
 */
void
key_bindings_dump();

/**
 * @brief Return Keybinding data for a given action
 * @return key_binding_t structure pointer or NULL if not found
 */
key_binding_t *
key_binding_data(int action);

/**
 * @brief Bind a key to an action
 *
 * @param action One action defined in @key_actions
 * @param key Keycode returned by getch
 */
void
key_bind_action(int action, int key);

/**
 * @brief Unbind a key to an action
 *
 * @param action One action defined in @key_actions
 * @param key Keycode returned by getch
 */
void
key_unbind_action(int action, int key);

/**
 * @brief Find the next action for a given key
 *
 * Set start parameter to -1 for start searching the
 * first action.
 *
 * @param action One action defined in @key_actions
 * @param key Keycode returned by getch
 */
int
key_find_action(int key, int start);

/**
 * @brief Return the action id associate to an action str
 *
 * This function is used to translate keybindings configuration
 * found in sngreprc file to internal Action IDs
 *
 * @param action Configuration string for an action
 * @return action id from @key_actions or -1 if none found
 */
int
key_action_id(const char *action);

/**
 * @brief Check if key is a printable ascii character
 *
 * @return 1 if key is alphanumeric or space
 */
int
key_is_printable(int key);

/**
 * @brief Return a Human readable representation of a key
 *
 * @return Character string representing the key
 */
const char *
key_to_str(int key);

/**
 * @brief Parse Human key declaration to curses key
 *
 * This function is used to translate keybindings configuration
 * keys found in sngreprc file into internal ncurses keycodes
 *
 * @return ncurses keycode for the given key string
 */
int
key_from_str(const char *key);

/**
 * @brief Return Human readable key for an action
 *
 * This function is used to display keybindings in the bottom bar
 * of panels. Depending on sngrep configuration it will display the
 * first associated keybding with the action or the second one
 * (aka alternative).
 *
 * @param action One action defined in @key_actions
 * @return Main/Alt keybinding for the given action
 */
const char *
key_action_key_str(int action);

/**
 * @brief Return key value for a given action
 *
 * @param action One action defined in @key_actions
 * @return Main/Alt keybinding for the given action
 */
int
key_action_key(int action);

#endif /* __SNGREP_KEYBINDING_H_ */
