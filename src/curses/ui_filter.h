/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2016 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2016 Irontec SL. All rights reserved.
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
 * @file ui_filter.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ui window for filtering options
 *
 * This file contains the functions and structures to manage the filter
 * dialog, that can be used to filter the lines in call list window.
 */

#ifndef __UI_FILTER_H
#define __UI_FILTER_H

#include "config.h"
#include <form.h>
#include "ui_manager.h"

/**
 * @brief Enum of available dialog fields
 *
 * Dialog form has a field array. Following enum represents the
 * order this fields are stored in panel info structure.
 *
 */
enum filter_field_list {
    FLD_FILTER_SIPFROM = 0,
    FLD_FILTER_SIPTO,
    FLD_FILTER_SRC,
    FLD_FILTER_DST,
    FLD_FILTER_PAYLOAD,
    FLD_FILTER_REGISTER,
    FLD_FILTER_INVITE,
    FLD_FILTER_SUBSCRIBE,
    FLD_FILTER_NOTIFY,
    FLD_FILTER_OPTIONS,
    FLD_FILTER_PUBLISH,
    FLD_FILTER_MESSAGE,
    FLD_FILTER_FILTER,
    FLD_FILTER_CANCEL,
    //! Never remove this field id @see filter_info
    FLD_FILTER_COUNT
};

//! Sorter declaration of struct filter_info
typedef struct filter_info filter_info_t;

/**
 * @brief Filter panel private information
 *
 * This structure contains the durable data of filter panel.
 */
struct filter_info {
    //! Form that contains the filter fields
    FORM *form;
    //! An array of fields
    FIELD *fields[FLD_FILTER_COUNT + 1];
};

/**
 * @brief Creates a new filter panel
 *
 * This function allocates all required memory for
 * displaying the filter panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @return a panel pointer
 */
void
filter_create(ui_t *ui);

/**
 * @brief Destroy filter panel
 *
 * This function do the final cleanups for this panel
 */
void
filter_destroy(ui_t *ui);

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param panel Ncurses panel pointer
 * @return a pointer to info structure of given panel
 */
filter_info_t *
filter_info(ui_t *ui);

/**
 * @brief Manage pressed keys for filter panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the filter panel to manage
 * its own keys.
 * If this function return 0, the key will not be handled
 * by ui manager. Otherwise the return will be considered
 * a key code.
 *
 * @param panel Filter panel pointer
 * @param key   key code
 * @return 0 if the key is handled, keycode otherwise
 */
int
filter_handle_key(ui_t *ui, int key);

/**
 * @brief Save form data to options
 *
 * This function will update the options values
 * of filter fields with its new value.
 *
 * @param panel Filter panel pointer
 */
void
filter_save_options(ui_t *ui);

/**
 * @brief Return String value for a filter field
 *
 * @return method name
 */
const char*
filter_field_method(int field_id);

/**
 * @brief Set Method filtering from filter.methods setting format
 */
void
filter_method_from_setting(const char *value);

#endif
