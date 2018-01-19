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

#include "config.h"
#include "ui_manager.h"

//! Sorter declaration of struct call_raw_info
typedef struct call_raw_info call_raw_info_t;

/**
 * @brief Call raw status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct call_raw_info {
    //! Group of calls displayed on the panel (Call raw display)
    sip_call_group_t *group;
    //! Message to display on the panel (Single message raw display)
    sip_msg_t *msg;
    //! Last printed message on panel (Call raw display)
    sip_msg_t *last;
    //! Window pad to copy on displayed screen
    WINDOW *pad;
    //! Already used lines of the window pad
    int padline;
    //! Scroll position of the window pad
    int scroll;
};

/**
 * @brief Create Call Raw panel
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 */
void
call_raw_create(ui_t *ui);

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param panel Ncurses panel pointer
 */
void
call_raw_destroy(ui_t *ui);

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param panel Ncurses panel pointer
 * @return a pointer to info structure of given panel
 */
call_raw_info_t *
call_raw_info(ui_t *ui);

/**
 * @brief Determine if the screen requires redrawn
 *
 * This will query the interface if it requires to be redraw again.
 *
 * @param ui UI structure pointer
 * @return true if the panel requires redraw, false otherwise
 */
bool
call_raw_redraw(ui_t *ui);

/**
 * @brief Draw the Call Raw panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
int
call_raw_draw(ui_t *ui);

/**
 * @brief Draw a message in call Raw
 *
 * Draw a new message in the Raw pad.
 *
 * @param panel Ncurses panel pointer
 * @param msg New message to be printed
 * @return 0 in call cases
 */
int
call_raw_print_msg(ui_t *ui, sip_msg_t *msg);

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
int
call_raw_handle_key(ui_t *ui, int key);

/**
 * @brief Set the active call group of the panel
 *
 * This function will access the panel information and will set the
 * call group pointer to the processed calls.
 *
 * @param group Call Group pointer to be set in the internal info struct
 */
int
call_raw_set_group(sip_call_group_t *group);

/**
 * @brief Set the active msg of the panel
 *
 * This function will access the panel information and will set the
 * msg pointer to the processed message.
 *
 * @param msg Message pointer to be set in the internal info struct
 */
int
call_raw_set_msg(sip_msg_t *msg);

#endif
