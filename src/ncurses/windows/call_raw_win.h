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
 * @file call_raw_win.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage Raw output screen of Sip messages
 *
 * This file contains the functions and structures to manage the raw message
 * output screen.
 *
 */
#ifndef __SNGREP_CALL_RAW_WIN_H
#define __SNGREP_CALL_RAW_WIN_H

#include "config.h"
#include "storage/group.h"
#include "storage/message.h"
#include "ncurses/manager.h"

//! Sorter declaration of struct call_raw_info
typedef struct _CallRawWinInfo CallRawWinInfo;

/**
 * @brief Call raw status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct _CallRawWinInfo
{
    //! Group of calls displayed on the panel (Call raw display)
    CallGroup *group;
    //! Message to display on the panel (Single message raw display)
    Message *msg;
    //! Last printed message on panel (Call raw display)
    Message *last;
    //! Window pad to copy on displayed screen
    WINDOW *pad;
    //! Already used lines of the window pad
    guint padline;
    //! Scroll position of the window pad
    guint scroll;
};

/**
 * @brief Create Call Raw panel
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 */
Window *
call_raw_win_new();

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param panel Ncurses panel pointer
 */
void
call_raw_win_free(Window *window);

/**
 * @brief Set the active call group of the panel
 *
 * This function will access the panel information and will set the
 * call group pointer to the processed calls.
*
 * @param window Call raw window pointer
 * @param group Call Group pointer to be set in the internal info struct
 */
void
call_raw_win_set_group(Window *window, CallGroup *group);

/**
 * @brief Set the active msg of the panel
 *
 * This function will access the panel information and will set the
 * msg pointer to the processed message.
 *
 * @param msg Message pointer to be set in the internal info struct
 */
void
call_raw_win_set_msg(Window *window, Message *msg);

#endif /* __SNGREP_CALL_RAW_WIN_H */
