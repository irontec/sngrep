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
#include "tui/widgets/window.h"
#include "tui/widgets/scrollbar.h"
#include "tui/widgets/menu_bar.h"

G_BEGIN_DECLS

#define TUI_TYPE_CALL_LIST_WIN call_list_win_get_type()
G_DECLARE_FINAL_TYPE(CallListWindow, call_list_win, TUI, CALL_LIST_WIN, Window)

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
    Attribute *attr;
    //! Attribute name
    const gchar *name;
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
    //! Window menu bar
    SngWidget *menu_bar;
    //! Display filter entry
    SngWidget *en_dfilter;
    //! Call List table
    SngWidget *tb_calls;
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

Table *
call_list_win_get_table(Window *window);

G_END_DECLS

#endif  /* __SNGREP_CALL_LIST_WIN_H__ */
