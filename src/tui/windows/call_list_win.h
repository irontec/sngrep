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
 * @file ui_call_list.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage Call List screen
 *
 * This file contains the functions and structures to manage the call list
 * screen.
 *
 */
#ifndef __SNGREP_CALL_LIST_WIN_H__
#define __SNGREP_CALL_LIST_WIN_H__

#include <menu.h>
#include <form.h>
#include <glib.h>
#include "tui/tui.h"
#include "tui/window.h"
#include "tui/scrollbar.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_CALL_LIST call_list_get_type()
G_DECLARE_FINAL_TYPE(CallListWindow, call_list, TUI, CALL_LIST, Window)


/**
 * @brief Enum of available fields
 *
 */
typedef enum
{
    FLD_LIST_FILTER = 0,
    FLD_LIST_COUNT,
} CallListFieldList;

//! Sorter declaration of column struct
typedef struct _CallListColumn CallListColumn;

/**
 * @brief Call List column information
 *
 * It will be nice make which columns will appear in this list and
 * in which order a configurable option.
 * This structure is one step towards configurable stuff
 */
struct _CallListColumn
{
    //! Attribute id of the column
    enum AttributeId id;
    //! Attribute name
    const gchar *attr;
    //! Column title
    const gchar *title;
    //! Current column position
    gint position;
    //! Current width
    gint width;
};

/**
 * @brief Call List panel status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * panel pointer.
 */
struct _CallListWindow
{
    //! Parent object attributes
    Window parent;
    //! Displayed calls vector
    GPtrArray *dcalls;
    //! Selected call in the list
    gint cur_idx;
    //! First displayed call in the list
    gint first_idx;
    //! Selected calls with space
    CallGroup *group;
    //! Displayed column list, make it configurable in the future
    GPtrArray *columns;
    //! Columns subwindow
    WINDOW *cols_win;
    //! List subwindow
    WINDOW *list_win;
    //! Form that contains the display filter
    FORM *form;
    //! An array of window form fields
    FIELD *fields[FLD_LIST_COUNT + 1];
    //! We're entering keys on form
    gboolean form_active;
    // Columns sort menu
    MENU *menu;
    // Columns sort menu items
    ITEM *items[ATTR_COUNT + 1];
    //! We're selecting sorting field
    gboolean menu_active;
    //! Move to last list entry if autoscroll is enabled
    gboolean autoscroll;
    //! List vertical scrollbar
    Scrollbar vscroll;
    //! List horizontal scrollbar
    Scrollbar hscroll;
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
call_list_win_new();

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
const char *
call_list_win_line_text(Window *window, Call *call);

/**
 * @brief Remove all calls from the list and calls storage
 *
 * This function will clear all call lines in the list
 * @param window UI structure pointer
 */
void
call_list_win_clear(Window *window);

G_END_DECLS

#endif  /* __SNGREP_CALL_LIST_WIN_H__ */
