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
 * @file column_select_win.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage columns select panel
 */

#ifndef __COLUMN_SELECT_WIN_H
#define __COLUMN_SELECT_WIN_H

#include "config.h"
#include <menu.h>
#include <form.h>
#include "ncurses/manager.h"
#include "ncurses/scrollbar.h"
#include "attribute.h"

/**
 * @brief Enum of available fields
 */
enum ColumnSelectFields
{
    FLD_COLUMNS_ACCEPT = 0,
    FLD_COLUMNS_SAVE,
    FLD_COLUMNS_CANCEL,
    FLD_COLUMNS_COUNT
};


//! Sorter declaration of struct columns_select_info
typedef struct _ColumnSelectWinInfo ColumnSelectWinInfo;

/**
 * @brief Column selector panel private information
 *
 * This structure contains the durable data of column selection panel.
 */
struct _ColumnSelectWinInfo
{
    //! Section of panel where menu is being displayed
    WINDOW *menu_win;
    //! Columns menu
    MENU *menu;
    // Columns Items
    ITEM *items[ATTR_COUNT + 1];
    //! Current selected columns
    GPtrArray *selected;
    //! Form that contains the save fields
    FORM *form;
    //! An array of window form fields
    FIELD *fields[FLD_COLUMNS_COUNT + 1];
    //! Flag to handle key inputs
    gboolean form_active;
    //! Scrollbar for the menu window
    Scrollbar scroll;
};

/**
 * @brief Creates a new column selection window
 *
 * This function allocates all required memory for
 * displaying the column selection window. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @return Window UI structure pointer
 */
Window *
column_select_win_new();

/**
 * @brief Set Column array to be updated
 * @param columns Array of currect active columns
 */
void
column_select_win_set_columns(Window *window, GPtrArray *columns);

#endif /* __COLUMN_SELECT_WIN_H */
