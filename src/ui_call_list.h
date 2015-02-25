/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
#include <form.h>
#include "ui_manager.h"

/**
 * @brief Enum of available fields
 *
 */
enum call_list_field_list
{
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
    //! Selected calls with space
    sip_call_group_t *group;
    //! Displayed column list, make it configurable in the future
    call_list_column_t columns[SIP_ATTR_SENTINEL];
    //! Displayed column count.
    int columncnt;
    //! Stores the current selected line
    int cur_line;
    //! List subwindow
    WINDOW *list_win;
    //! Form that contains the display filter
    FORM *form;
    //! An array of window form fields
    FIELD *fields[FLD_LIST_COUNT + 1];
    //! We're entering keys on form
    bool form_active;
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
PANEL *
call_list_create();

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param panel Ncurses panel pointer
 * @return panel Ncurses panel pointer
 */
void
call_list_destroy(PANEL *panel);

/**
 * @brief Draw panel footer
 * 
 * This funtion will draw Call list footer that contains
 * keybinginds
 * 
 * @param panel Ncurses panel pointer
 */
void
call_list_draw_footer(PANEL *panel);

/**
 * @brief Draw the Call list panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
int
call_list_draw(PANEL *panel);

/**
 * @brief Enable/Disable Panel form focus
 *
 * Enable or disable form fields focus so the next input will be
 * handled by call_list_handle_key or call_list_handle_form_key
 * This will also set properties in fields to show them as focused
 * and show/hide the cursor
 *
 * @param panel Ncurses panel pointer
 * @param active Enable/Disable flag
 */
void
call_list_form_activate(PANEL *panel, bool active);

/**
 * @brief Get List line from the given call
 *
 * Get the list line of the given call to display in the list
 * This line is built using the configured columns and sizes
 *
 * @param panel Ncurses panel pointer
 * @param call Call to get data from
 * @param text Text pointer to store the generated line
 * @return A pointer to text
 */
const char*
call_list_line_text(PANEL *panel, sip_call_t *call, char *text);

/**
 * @brief Handle Call list key strokes
 *
 * This function will manage the custom keybindings of the panel. If this
 * function returns -1, the ui manager will check if the pressed key
 * is one of the common ones (like toggle colors and so).
 *
 * @param panel Ncurses panel pointer
 * @param key Pressed keycode
 * @return 0 if the function can handle the key, key otherwise
 */
int
call_list_handle_key(PANEL *panel, int key);

/**
 * @brief Handle Forms entries key strokes
 *
 * This function will manage the custom keybindings of the panel form.
 *
 * @param panel Ncurses panel pointer
 * @param key Pressed keycode
 * @return 0 if the function can handle the key, key otherwise
 */
int
call_list_handle_form_key(PANEL *panel, int key);

/**
 * @brief Request the panel to show its help
 *
 * This function will request to panel to show its help (if any) by
 * invoking its help function.
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the screen has help, -1 otherwise
 */
int
call_list_help(PANEL *panel);

/**
 * @brief Request confirmation before exit
 *
 * This function will request the user to confirm exit from
 * the program. This message can be avoided using configuration
 * option cl.noexitprompt 0
 * The default button can be configured using option
 * cl.defexitbutton (default 1, that means yes)
 *
 * @param panel Call list panel pointer
 * @return 27 if user confirmed exit, 0 otherwise
 */
int
call_list_exit_confirm(PANEL *panel);

/**
 * @brief Add a column the Call List
 *
 * This function will add a new column to the Call List panel
 * @todo Columns are not configurable yet.
 *
 * @param panel Ncurses panel pointer
 * @param id SIP call attribute id
 * @param attr SIP call attribute name
 * @param title SIP call attribute description
 * @param width Column Width
 * @return 0 if column has been successufly added to the list, -1 otherwise
 */
int
call_list_add_column(PANEL *panel, enum sip_attr_id id, const char* attr, const char *title,
                     int width);

/**
 * @brief Update list information after a filter has been set
 *
 * This function is called after showing the filter dialog (@see ui_filter.h)
 * and resets the displayed information to force a new call list
 * load.
 *
 * @param panel Call list panel pointer
 */
void
call_list_filter_update(PANEL *panel);

/**
 * @brief Remove all calls from the list and calls storage
 *
 * This funtion will clear all call lines in the list
 * @param panel Call list panel pointer
 */
void
call_list_clear(PANEL *panel);

/**
 * @brief Get call count after applying display fitering
 *
 * @param panel Ncurses panel pointer
 * @return number of calls that match display filter
 */
int
call_list_count(PANEL *panel);

/**
 * @brief Get next call after applying display fitering
 *
 * @param panel Ncurses panel pointer
 * @param call Start searching from this call
 * @return next matching call or NULL
 */
sip_call_t *
call_list_get_next(PANEL *panel, sip_call_t *cur);

/**
 * @brief Get previous call after applying display fitering
 *
 * @param panel Ncurses panel pointer
 * @param call Start searching from this call
 * @return previous matching call or NULL
 */
sip_call_t *
call_list_get_prev(PANEL *panel, sip_call_t *cur);

/**
 * @brief Check if call match display filter
 *
 * @param panel Ncurses panel pointer
 * @param call Start searching from this call
 * @return 1 if the call match display filters, 0 otherwise
 */
int
call_list_match_dfilter(PANEL *panel, sip_call_t *call);

#endif
