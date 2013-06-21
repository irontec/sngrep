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
#ifndef __UI_CALL_LIST_H
#define __UI_CALL_LIST_H
#include "ui_manager.h"

//! Sorter declaration of call_list_column struct
typedef struct call_list_column call_list_column_t;
//! Sorter declaration of call_list_info struct
typedef struct call_list_info call_list_info_t;

/**
 * @brief Call List column information
 * It will be nice make which columns will appear in this list and 
 * in which order a configurable option. 
 * This structure is one step towards configurable stuff
 */
struct call_list_column {
     int id;
     char *title;
     int width;
};

/**
 * @brief Call List panel status information
 * This data stores the actual status of the panel. It's stored in the
 * panel pointer.
 */
struct call_list_info {
    //! First displayed call, for drawing faster
    sip_call_t *first_call;    
    //! First displayed call counter, for drawing scroll arrow faster
    int first_line;
    //! Selected call in the list
    sip_call_t *cur_call;
    //! Displayed column list, make it configurable in the future
    call_list_column_t columns[10];
    //! Displayed column count. 
    int columncnt;
    //! Displayed lines in the list.
    int linescnt;
    //! Stores the current selected line
    int cur_line;
}; 

extern PANEL *call_list_create();
extern int call_list_draw(PANEL *panel);
extern int call_list_handle_key(PANEL *panel, int key);
extern int call_list_help(PANEL *panel);
extern int call_list_add_column (PANEL *panel, int id, const char *title, int width);
#endif
