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
#ifndef __UI_CALL_RAW_H
#define __UI_CALL_RAW_H
#include "sip.h"
#include "ui_manager.h"

//! Sorter declaration of struct call_raw_info
typedef struct call_raw_info call_raw_info_t;

/** 
 * @brief Call raw status information
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct call_raw_info {
    sip_call_t *call;
    int scrollpos;
};

extern PANEL *call_raw_create();
extern int call_raw_draw(PANEL *panel);
extern int call_raw_handle_key(PANEL *panel, int key);
extern int call_raw_help(PANEL *panel);
extern int call_raw_set_call(sip_call_t *call);

#endif
