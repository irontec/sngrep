/**************************************************************************
 **
 **  sngrep - Ncurses ngrep interface for SIP
 **
 **   Copyright (C) 2013 Ivan Alonso (Kaian)
 **   Copyright (C) 2013 Irontec SL. All rights reserved.
 **
 **   This program is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   This program is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
#ifndef __UI_CALL_FLOW_H
#define __UI_CALL_FLOW_H
#include "sip.h"
#include "ui_manager.h"

//! Sorter declaration of struct call_flow_info
typedef struct call_flow_info call_flow_info_t;

/**
 * @brief Call flow status information
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct call_flow_info {
    sip_call_t *call;
    sip_msg_t *first_msg;
    sip_msg_t *cur_msg;
    int linescnt;
    int cur_line;
};

extern PANEL *call_flow_create();
extern void call_flow_destroy(PANEL *panel);
extern int call_flow_draw(PANEL *panel);
extern int call_flow_handle_key(PANEL *panel, int key);
extern int call_flow_help(PANEL *panel);
extern int call_flow_set_call(sip_call_t *call);

#endif
