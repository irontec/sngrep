/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
 **
 ** Copyright (C) 2013 Ivan Alonso (Kaian)
 ** Copyright (C) 2013 Irontec SL. All rights reserved.
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
 * @file ui_call_raw.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage Raw output screen of Sip messages
 *
 * This file contains the functions and structures to manage the raw message
 * output screen.
 *
 */
#ifndef __UI_CALL_RAW_H
#define __UI_CALL_RAW_H
#include "ui_manager.h"

//! Sorter declaration of struct call_raw_info
typedef struct call_raw_info call_raw_info_t;

/**
 * @brief Call raw status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct call_raw_info
{
    sip_call_group_t *group;
    sip_msg_t *msg;
    int totlines;
    int scrline;
    int scrollpos;
    int linescnt;
};

/**
 * @brief Create Call Raw panel
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 *
 * @return the allocated ncurses panel
 */
extern PANEL *
call_raw_create();

/**
 * @brief Check if the panel requires to be redrawn
 *
 * During online mode, this function will be invoked if this is the topmost
 * panel every time a new message has been readed.
 *
 * @param panel Ncurses panel pointer
 * @param msg New readed message
 * @return 0 if the panel needs to be redrawn, -1 otherwise
 */
extern int
call_raw_redraw_required(PANEL *panel, sip_msg_t *msg);

/**
 * @brief Draw the Call Raw panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
extern int
call_raw_draw(PANEL *panel);

extern int
call_raw_print_msg(PANEL *panel, sip_msg_t *msg);

/**
 * @brief Handle Call Raw key strokes
 *
 * This function will manage the custom keybindings of the panel. If this
 * function returns -1, the ui manager will check if the pressed key
 * is one of the common ones (like toggle colors and so).
 *
 * @param panel Ncurses panel pointer
 * @param key Pressed keycode
 * @return 0 if the function can handle the key, key otherwise
 */
extern int
call_raw_handle_key(PANEL *panel, int key);

/**
 * @brief Set the active call group of the panel
 *
 * This function will access the panel information and will set the
 * call group pointer to the processed calls.
 *
 * @param group Call Group pointer to be set in the internal info struct
 */
extern int
call_raw_set_group(sip_call_group_t *group);

/**
 * @brief Set the active msg of the panel
 *
 * This function will access the panel information and will set the
 * msg pointer to the processed message.
 *
 * @param msg Message pointer to be set in the internal info struct
 */
extern int
call_raw_set_msg(sip_msg_t *msg);

#endif
