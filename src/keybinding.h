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

/**
 * @brief Available Key actions
 */
enum key_actions
{
    ACTION_UP = 1,
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
    ACTION_NEXT_FIELD,
    ACTION_CLEAR,
    ACTION_CLEAR_CALLS,
    ACTION_TOGGLE_SYNTAX,
    ACTION_TOGGLE_COLOR,
    ACTION_SHOW_HOSTNAMES,
    ACTION_TOGGLE_PAUSE,
    ACTION_PREV_SCREEN,
    ACTION_SHOW_HELP,
    ACTION_SHOW_RAW,
    ACTION_SHOW_FLOW,
    ACTION_SHOW_FLOW_EX,
    ACTION_SHOW_FILTERS,
    ACTION_SHOW_COLUMNS,
    ACTION_DISP_FILTER,
    ACTION_DISP_INVITE,
    ACTION_SAVE,
    ACTION_SELECT,
    ACTION_TOGGLE_RTP,
    ACTION_TOGGLE_RAW,
    ACTION_INCREASE_RAW,
    ACTION_DECREASE_RAW,
    ACTION_RESET_RAW,
    ACTION_ONLY_SDP,
    ACTION_SDP_INFO,
    ACTION_COMPRESS,
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
    int keys[5];
    //! How many keys are binded to this action
    int bindcnt;
};

void
key_bind_action(int action, int key);

int
key_find_action(int key, int start);

#endif /* __SNGREP_KEYBINDING_H_ */
