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

#include <menu.h>
#include <glib.h>
#include "ncurses/ui_manager.h"
#include "ncurses/scrollbar.h"

/**
 * @brief Enum of available fields
 *
 */
enum CallListFieldList {
    FLD_LIST_FILTER = 0,
    //! Never remove this field id
    FLD_LIST_COUNT
};

//! Sorter declaration of column struct
typedef struct _CallListColumn CallListColumn;
//! Sorter declaration of info struct
typedef struct _CallListInfo CallListInfo;

/**
 * @brief Call List column information
 *
 * It will be nice make which columns will appear in this list and
 * in which order a configurable option.
 * This structure is one step towards configurable stuff
 */
struct _CallListColumn {
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
struct _CallListInfo {
    //! Displayed calls vector
    GPtrArray *dcalls;
    //! Selected call in the list
    guint cur_call;
    //! Selected calls with space
    SipCallGroup *group;
    //! Displayed column list, make it configurable in the future
    CallListColumn columns[SIP_ATTR_COUNT];
    //! Displayed column count.
    guint columncnt;
    //! List subwindow
    WINDOW *list_win;
    //! Form that contains the display filter
    FORM *form;
    //! An array of window form fields
    FIELD *fields[FLD_LIST_COUNT + 1];
    //! We're entering keys on form
    int form_active;
    // Columns sort menu
    MENU *menu;
    // Columns sort menu items
    ITEM *items[SIP_ATTR_COUNT + 1];
    //! We're selecting sorting field
    int menu_active;
    //! Move to last list entry if autoscroll is enabled
    int autoscroll;
    //! List scrollbar
    scrollbar_t scroll;
};

/**
 * @brief Create Call List window
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 *
 * @param window UI structure pointer
 * @return the allocated window structure
 */
Window *
call_list_new();

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param window UI structure pointer
 * @return a pointer to info structure of given panel
 */
CallListInfo *
call_list_info(Window *window);

/**
 * @brief Get List line from the given call
 *
 * Get the list line of the given call to display in the list
 * This line is built using the configured columns and sizes
 *
 * @param window UI structure pointer
 * @param call Call to get data from
 * @param text Text pointer to store the generated line
 * @return A pointer to text
 */
const char*
call_list_line_text(Window *window, SipCall *call, char *text);

/**
 * @brief Add a column the Call List
 *
 * This function will add a new column to the Call List panel
 * @todo Columns are not configurable yet.
 *
 * @param window UI structure pointer
 * @param id SIP call attribute id
 * @param attr SIP call attribute name
 * @param title SIP call attribute description
 * @param width Column Width
 */
void
call_list_add_column(Window *window, enum sip_attr_id id, const char* attr,
                     const char *title, int width);

/**
 * @brief Remove all calls from the list and calls storage
 *
 * This funtion will clear all call lines in the list
 * @param window UI structure pointer
 */
void
call_list_clear(Window *window);

#endif
