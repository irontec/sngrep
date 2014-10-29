/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
#include "ui_manager.h"

//! Sorter declaration of struct msg_diff_info
typedef struct msg_diff_info msg_diff_info_t;

/**
 * @brief Call raw status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct msg_diff_info
{
    sip_msg_t *one;
    sip_msg_t *two;
    WINDOW *one_win;
    WINDOW *two_win;
};

PANEL *
msg_diff_create();

msg_diff_info_t *
msg_diff_info(PANEL *panel);

void
msg_diff_destroy(PANEL *panel);

int
msg_diff_draw(PANEL *panel);

void
msg_diff_draw_footer(PANEL *panel);

int
msg_diff_draw_messages(PANEL *panel);

int
msg_diff_handle_key(PANEL *panel, int key);

int
msg_diff_help(PANEL *panel);

int
msg_diff_set_msgs(PANEL *panel, sip_msg_t *one, sip_msg_t *two);

#endif
