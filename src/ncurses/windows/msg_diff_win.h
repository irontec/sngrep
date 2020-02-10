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
 * @file msg_diff_win.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage diff display
 *
 */
#ifndef __SNGREP_MSG_DIFF_WIN_H__
#define __SNGREP_MSG_DIFF_WIN_H__

#include "config.h"
#include "ncurses/manager.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_MSG_DIFF msg_diff_win_get_type()
G_DECLARE_FINAL_TYPE(MsgDiffWindow, msg_diff_win, NCURSES, MSG_DIFF, Window)

/**
 * @brief Call raw status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct _MsgDiffWindow
{
    //! Parent object attributes
    Window parent;
    //! First message to compare
    Message *one;
    //! Second message to compare
    Message *two;
    //! Left displayed subwindow
    WINDOW *one_win;
    //! Right displayed subwindow
    WINDOW *two_win;
};

/**
 * @brief Create Message diff panel
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 *
 * @return the allocated ncurses panel
 */
Window *
msg_diff_win_new();

/**
 * @brief Deallocate panel memory
 *
 * This function will be called from ui manager logic to free
 * message diff panel memory
 *
 * @param window UI structure pointer
 */
void
msg_diff_win_free(Window *window);

/**
 * @brief Set the panel working messages
 *
 * This function will access the panel information and will set the
 * msg pointers to the processed messages.
 *
 * @param window UI structure pointer
 * @param one Message pointer to be set in the internal info struct
 * @param two Message pointer to be set in the internal info struct
 */
void
msg_diff_win_set_msgs(Window *window, Message *one, Message *two);

G_END_DECLS

#endif  /* __SNGREP_MSG_DIFF_WIN_H__ */
