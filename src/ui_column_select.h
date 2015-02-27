/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2014,2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2014,2015 Irontec SL. All rights reserved.
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

#include <menu.h>
#include "ui_manager.h"
#include "sip_attr.h"

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
    ITEM *items[SIP_ATTR_SENTINEL + 1];
};

/**
 * @brief Creates a new column selection panel
 *
 * This function allocates all required memory for
 * displaying the column selection panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @return a panel pointer
 */
PANEL *
column_select_create();

/**
 * @brief Destroy column selection panel
 *
 * This function do the final cleanups for this panel
 */
void
column_select_destroy();

/**
 * @brief Manage pressed keys for column selection panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the filter panel to manage
 * its own keys.
 * If this function return 0, the key will not be handled
 * by ui manager. Otherwise the return will be considered
 * a key code.
 *
 * @param panel Column selection panel pointer
 * @param key   key code
 * @return 0 if the key is handled, keycode otherwise
 */
int
column_select_handle_key(PANEL *panel, int key);

/**
 * @brief Update Call List columns
 *
 * This function will update the columns of Call List
 *
 * @param panel Column selection panel pointer
 */
void
column_select_update_columns(PANEL *panel);

/**
 * @brief Move a item to a new position
 *
 * This function can be used to reorder the column list
 *
 * @param panel Column selection panel pointer
 * @param item Menu item to be moved
 * @param post New position in the menu
 */
void
column_select_move_item(PANEL *panel, ITEM *item, int pos);

/**
 * @brief Select/Deselect a menu item
 *
 * This function can be used to toggle selection status of
 * the menu item
 *
 * @param panel Column selection panel pointer
 * @param item Menu item to be (de)selected
 */
void
column_select_toggle_item(PANEL *panel, ITEM *item);

/**
 * @brief Update menu after a change
 *
 * After moving an item or updating its selection status
 * menu must be redrawn.
 *
 * @param panel Column selection panel pointer
 */
void
column_select_update_menu(PANEL *panel);

#endif /* __UI_COLUMN_SELECT_H */
