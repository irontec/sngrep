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

#include "ui_manager.h"
#include <menu.h>
#include "scrollbar.h"

/**
 * @brief Enum of available fields
 *
 */
enum call_list_field_list {
    FLD_LIST_FILTER = 0,
    //! Never remove this field id
    FLD_LIST_COUNT
};

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
struct call_list_column {
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
struct call_list_info {
    //! Displayed calls vector
    vector_t *dcalls;
    //! Selected call in the list
    int cur_call;
    //! Selected calls with space
    sip_call_group_t *group;
    //! Displayed column list, make it configurable in the future
    call_list_column_t columns[SIP_ATTR_COUNT];
    //! Displayed column count.
    int columncnt;
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
 * @brief Create Call List panel
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 *
 * @param ui UI structure pointer
 * @return the allocated ncurses panel
 */
void
call_list_create(ui_t *ui);

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param ui UI structure pointer
 */
void
call_list_destroy(ui_t *ui);

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param ui UI structure pointer
 * @return a pointer to info structure of given panel
 */
call_list_info_t *
call_list_info(ui_t *ui);

/**
 * @brief Determine if the screen requires redrawn
 *
 * This will query the interface if it requires to be redraw again.
 *
 * @param ui UI structure pointer
 * @return true if the panel requires redraw, false otherwise
 */
bool
call_list_redraw(ui_t *ui);

/**
 * @brief Resize the windows of Call List
 *
 * This function will be invoked when the ui size has changed
 *
 * @param ui UI structure pointer
 * @return 0 if the panel has been resized, -1 otherwise
 */
int
call_list_resize(ui_t *ui);

/**
 * @brief Draw panel header
 *
 * This funtion will draw Call list header
 *
 * @param ui UI structure pointer
 */
void
call_list_draw_header(ui_t *ui);

/**
 * @brief Draw panel footer
 *
 * This funtion will draw Call list footer that contains
 * keybinginds
 *
 * @param ui UI structure pointer
 */
void
call_list_draw_footer(ui_t *ui);

/**
 * @brief Draw panel list contents
 *
 * This funtion will draw Call list dialogs list
 *
 * @param ui UI structure pointer
 */
void
call_list_draw_list(ui_t *ui);

/**
 * @brief Draw the Call list panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param ui UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
int
call_list_draw(ui_t *ui);

/**
 * @brief Enable/Disable Panel form focus
 *
 * Enable or disable form fields focus so the next input will be
 * handled by call_list_handle_key or call_list_handle_form_key
 * This will also set properties in fields to show them as focused
 * and show/hide the cursor
 *
 * @param ui UI structure pointer
 * @param active Enable/Disable flag
 */
void
call_list_form_activate(ui_t *ui, int active);

/**
 * @brief Get List line from the given call
 *
 * Get the list line of the given call to display in the list
 * This line is built using the configured columns and sizes
 *
 * @param ui UI structure pointer
 * @param call Call to get data from
 * @param text Text pointer to store the generated line
 * @return A pointer to text
 */
const char*
call_list_line_text(ui_t *ui, sip_call_t *call, char *text);

/**
 * @brief Handle Call list key strokes
 *
 * This function will manage the custom keybindings of the panel.
 * This function return one of the values defined in @key_handler_ret
 *
 * @param ui UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
int
call_list_handle_key(ui_t *ui, int key);

/**
 * @brief Handle Forms entries key strokes
 *
 * This function will manage the custom keybindings of the panel form.
 *
 * @param ui UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
int
call_list_handle_form_key(ui_t *ui, int key);

/**
 * @brief Handle Sort menu key strokes
 *
 * This function will manage the custom keybidnings for sort menu
 *
 * @param ui UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
int
call_list_handle_menu_key(ui_t *ui, int key);

/**
 * @brief Request the panel to show its help
 *
 * This function will request to panel to show its help (if any) by
 * invoking its help function.
 *
 * @param ui UI structure pointer
 * @return 0 if the screen has help, -1 otherwise
 */
int
call_list_help(ui_t *ui);

/**
 * @brief Add a column the Call List
 *
 * This function will add a new column to the Call List panel
 * @todo Columns are not configurable yet.
 *
 * @param ui UI structure pointer
 * @param id SIP call attribute id
 * @param attr SIP call attribute name
 * @param title SIP call attribute description
 * @param width Column Width
 * @return 0 if column has been successufly added to the list, -1 otherwise
 */
int
call_list_add_column(ui_t *ui, enum sip_attr_id id, const char* attr, const char *title,
                     int width);

/**
 * @brief Remove all calls from the list and calls storage
 *
 * This funtion will clear all call lines in the list
 * @param ui UI structure pointer
 */
void
call_list_clear(ui_t *ui);

/**
 * @brief Move selected cursor to given line
 *
 * This function will move the cursor to given line, taking into account
 * selected line and scrolling position.
 *
 * @param ui UI structure pointer
 * @param line Position to move the cursor
 */
void
call_list_move(ui_t *ui, int line);

/**
 * @brief Select column to sort by
 *
 * This function will display a lateral menu to select the column to sort
 * the list for. The menu will be filled with current displayed columns.
 *
 * @param ui UI structure pointer
 */
void
call_list_select_sort_attribute(ui_t *ui);

#endif
