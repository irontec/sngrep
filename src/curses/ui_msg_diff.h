/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2016 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2016 Irontec SL. All rights reserved.
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
 * @file ui_msg_diff.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage diff display
 *
 */
#ifndef __UI_MSG_DIFF_H
#define __UI_MSG_DIFF_H
#include "config.h"
#include "ui_manager.h"

//! Sorter declaration of struct msg_diff_info
typedef struct msg_diff_info msg_diff_info_t;

/**
 * @brief Call raw status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct msg_diff_info {
    sip_msg_t *one;
    sip_msg_t *two;
    WINDOW *one_win;
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
PANEL *
msg_diff_create();

/**
 * @brief Deallocate panel memory
 *
 * This function will be called from ui manager logic to free
 * message diff panel memory
 *
 * @param panel Ncurses panel pointer
 */
void
msg_diff_destroy(PANEL *panel);

/**
 * @brief Get panel information structure
 *
 * All required information of the panel is stored in the info pointer
 * of the panel.
 * This function will return the pointer to the info structure of the
 * panel.
 *
 * @return a pointer to the info structure or NULL if no structure exists
 */
msg_diff_info_t *
msg_diff_info(PANEL *panel);

/**
 * @brief Redraw panel data
 *
 * This function will be called from ui manager logic when the panels
 * needs to be redrawn.
 *
 * @param panel Ncurses panel pointer
 * @return 0 in all cases
 */
int
msg_diff_draw(PANEL *panel);

/**
 * @brief Draw panel footer
 *
 * Usually panel footer contains useful keybidings. This function
 * will draw that footer
 *
 * @param panel Ncurses panel pointer
 */
void
msg_diff_draw_footer(PANEL *panel);

/**
 * @brief Draw a message into a raw subwindow
 *
 * This function will be called for each message that wants to be draw
 * in the panel.
 *
 */
int
msg_diff_draw_message(WINDOW *win, sip_msg_t *msg, char *highlight);

/**
 * @brief Handle key strokes
 *
 * This function will manage the custom keybindings of the panel. If this
 * function returns a key value, the ui manager will check if the pressed key
 * is one of the common ones (like toggle colors and so).
 *
 * @param panel Ncurses panel pointer
 * @param key Pressed keycode
 * @return 0 if the function can handle the key, key otherwise
 */
int
msg_diff_handle_key(PANEL *panel, int key);

/**
 * @brief Request the panel to show its help
 *
 * This function will request to panel to show its help by
 * invoking its help function.
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the screen has help
 */
int
msg_diff_help(PANEL *panel);

/**
 * @brief Set the panel working messages
 *
 * This function will access the panel information and will set the
 * msg pointers to the processed messages.
 *
 * @param panel Ncurses panel pointer
 * @param one Message pointer to be set in the internal info struct
 * @param two Message pointer to be set in the internal info struct
 * @return 0 in all cases
 */
int
msg_diff_set_msgs(PANEL *panel, sip_msg_t *one, sip_msg_t *two);

#endif
