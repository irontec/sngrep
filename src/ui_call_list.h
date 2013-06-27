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
 * @file ui_call_list.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage Call List screen
 *
 * This file contains the functions and structures to manage the call list
 * screen.
 *
 */
#ifndef __UI_CALL_LIST_H
#define __UI_CALL_LIST_H
#include "ui_manager.h"

//! Sorter declaration of call_list_column struct
typedef struct call_list_column call_list_column_t;
//! Sorter declaration of call_list_info struct
typedef struct call_list_info call_list_info_t;

/**
 * @brief Call List column information
 *
 * It will be nice make which columns will appear in this list and 
 * in which order a configurable option. 
 * This structure is one step towards configurable stuff
 */
struct call_list_column
{
    enum sip_attr_id id;
    const char *attr;
    const char *title;
    int width;
};

/**
 * @brief Call List panel status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * panel pointer.
 */
struct call_list_info
{
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

/**
 * @brief Create Call List panel
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 *
 * @return the allocated ncurses panel
 */
extern PANEL *
call_list_create();

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @return panel Ncurses panel pointer
 */
extern void
call_list_destroy(PANEL *panel);

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
call_list_redraw_required(PANEL *panel, sip_msg_t *msg);

/**
 * @brief Draw the Call list panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
extern int
call_list_draw(PANEL *panel);

/**
 * @brief Handle Call list key strokes
 *
 * This function will manage the custom keybindings of the panel. If this
 * function returns -1, the ui manager will check if the pressed key
 * is one of the common ones (like toggle colors and so).
 *
 * @param panel Ncurses panel pointer
 * @param key Pressed keycode
 * @return 0 if the function can handle the key, -1 otherwise
 */
extern int
call_list_handle_key(PANEL *panel, int key);

/**
 * @brief Request the panel to show its help
 *
 * This function will request to panel to show its help (if any) by
 * invoking its help function.
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the screen has help, -1 otherwise
 */
extern int
call_list_help(PANEL *panel);

/**
 * @brief Add a column the Call List
 *
 * This function will add a new column to the Call List panel
 * @todo Columns are not configurable yet.
 *
 * @param panel Ncurses panel pointer
 * @param attr SIP call attribute name
 * @param title Column Title
 * @param width Column Width
 * @return 0 if column has been successufly added to the list, -1 otherwise
 */
extern int
call_list_add_column(PANEL *panel, enum sip_attr_id id, const char* attr, const char *title, int width);

#endif
