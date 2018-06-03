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
 * @file ui_column_select.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage columns select panel
 */

#ifndef __UI_COLUMN_SELECT_H
#define __UI_COLUMN_SELECT_H

#include "config.h"
#include <menu.h>
#include <form.h>
#include "ui_manager.h"
#include "scrollbar.h"
#include "sip_attr.h"

/**
 * @brief Enum of available fields
 */
enum column_select_field_list {
    FLD_COLUMNS_ACCEPT = 0,
    FLD_COLUMNS_SAVE,
    FLD_COLUMNS_CANCEL,
    //! Never remove this field id
    FLD_COLUMNS_COUNT
};


//! Sorter declaration of struct columns_select_info
typedef struct column_select_info column_select_info_t;

/**
 * @brief Column selector panel private information
 *
 * This structure contains the durable data of column selection panel.
 */
struct column_select_info {
    // Section of panel where menu is being displayed
    WINDOW *menu_win;
    // Columns menu
    MENU *menu;
    // Columns Items
    ITEM *items[SIP_ATTR_COUNT + 1];
    //! Form that contains the save fields
    FORM *form;
    //! An array of window form fields
    FIELD *fields[FLD_COLUMNS_COUNT + 1];
    //! Flag to handle key inputs
    int form_active;
    //! Scrollbar for the menu window
    scrollbar_t scroll;
};

/**
 * @brief Creates a new column selection panel
 *
 * This function allocates all required memory for
 * displaying the column selection panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @param ui UI structure pointer
 */
void
column_select_create(ui_t *ui);

/**
 * @brief Destroy column selection panel
 *
 * This function do the final cleanups for this panel
 *
 * @param ui UI structure pointer
 */
void
column_select_destroy(ui_t *ui);

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param ui UI structure pointer
 * @return a pointer to info structure of given panel
 */
column_select_info_t *
column_select_info(ui_t *ui);

/**
 * @brief Manage pressed keys for column selection panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the filter panel to manage
 * its own keys.
 *
 * @param ui UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
int
column_select_handle_key(ui_t *ui, int key);

/**
 * @brief Manage pressed keys for column selection panel
 *
 * This function will handle keys when menu is active.
 * You can switch between menu and rest of the components
 * using TAB
 *
 * @param ui UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
int
column_select_handle_key_menu(ui_t *ui, int key);

/**
 * @brief Manage pressed keys for column selection panel
 *
 * This function will handle keys when form is active.
 * You can switch between menu and rest of the components
 * using TAB
 *
 * @param ui UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
int
column_select_handle_key_form(ui_t *ui, int key);

/**
 * @brief Update Call List columns
 *
 * This function will update the columns of Call List
 *
 * @param ui UI structure pointer
 */
void
column_select_update_columns(ui_t *ui);

/**
 * @brief Save selected columns to user config file
 *
 * Remove previously configurated columns from user's
 * $SNGREPRC or $HOME/.sngreprc and add new ones
 *
 * @param ui UI structure pointer
 */
void
column_select_save_columns(ui_t *ui);

/**
 * @brief Move a item to a new position
 *
 * This function can be used to reorder the column list
 *
 * @param ui UI structure pointer
 * @param item Menu item to be moved
 * @param post New position in the menu
 */
void
column_select_move_item(ui_t *ui, ITEM *item, int pos);

/**
 * @brief Select/Deselect a menu item
 *
 * This function can be used to toggle selection status of
 * the menu item
 *
 * @param ui UI structure pointer
 * @param item Menu item to be (de)selected
 */
void
column_select_toggle_item(ui_t *ui, ITEM *item);

/**
 * @brief Update menu after a change
 *
 * After moving an item or updating its selectioactivn status
 * menu must be redrawn.
 *
 * @param ui UI structure pointer
 */
void
column_select_update_menu(ui_t *ui);

#endif /* __UI_COLUMN_SELECT_H */
