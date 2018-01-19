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
    //! First message to compare
    sip_msg_t *one;
    //! Second message to compare
    sip_msg_t *two;
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
 * @param ui UI structure pointer
 * @return the allocated ncurses panel
 */
void
msg_diff_create(ui_t *ui);

/**
 * @brief Deallocate panel memory
 *
 * This function will be called from ui manager logic to free
 * message diff panel memory
 *
 * @param ui UI structure pointer
 */
void
msg_diff_destroy(ui_t *ui);

/**
 * @brief Get panel information structure
 *
 * All required information of the panel is stored in the info pointer
 * of the panel.
 * This function will return the pointer to the info structure of the
 * panel.
 *
 * @param ui UI structure pointer
 * @return a pointer to the info structure or NULL if no structure exists
 */
msg_diff_info_t *
msg_diff_info(ui_t *ui);

/**
 * @brief Redraw panel data
 *
 * This function will be called from ui manager logic when the panels
 * needs to be redrawn.
 *
 * @param ui UI structure pointer
 * @return 0 in all cases
 */
int
msg_diff_draw(ui_t *ui);

/**
 * @brief Draw panel footer
 *
 * Usually panel footer contains useful keybidings. This function
 * will draw that footer
 *
 * @param ui UI structure pointer
 */
void
msg_diff_draw_footer(ui_t *ui);

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
 * @brief Set the panel working messages
 *
 * This function will access the panel information and will set the
 * msg pointers to the processed messages.
 *
 * @param ui UI structure pointer
 * @param one Message pointer to be set in the internal info struct
 * @param two Message pointer to be set in the internal info struct
 * @return 0 in all cases
 */
int
msg_diff_set_msgs(ui_t *ui, sip_msg_t *one, sip_msg_t *two);

#endif
