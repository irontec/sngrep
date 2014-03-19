/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
 **
 ** Copyright (C) 2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2014 Irontec SL. All rights reserved.
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
 * @file ui_save_raw.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ui window for saving captured packages
 *
 * This file contains the functions and structures to manage the save
 * dialog, that can be used to copy the temporal sngrep file to another location
 *
 */

#ifndef __UI_SAVE_RAW_H
#define __UI_SAVE_RAW_H
#include <form.h>
#include "group.h"
#include "ui_manager.h"

/**
 * @brief Enum of available dialog fields
 *
 * Dialog form has a field array. Following enum represents the
 * order this fields are stored in panel info structure.
 *
 */
enum save_raw_field_list {
    FLD_SAVE_RAW_FILE,
    FLD_SAVE_RAW_SAVE,
    FLD_SAVE_RAW_CANCEL,
    //! Never remove this field id @see save_info
    FLD_SAVE_RAW_COUNT,
};

//! Sorter declaration of struct save_info
typedef struct save_raw_info save_raw_info_t;

/**
 * @brief Save panel private information
 *
 * This structure contains the durable data of save panel.
 */
struct save_raw_info
{
    //! Form that contains the save fields
    FORM *form;
    //! An array of fields
    FIELD *fields[FLD_SAVE_RAW_COUNT];
    //! Group of calls information
    sip_call_group_t *group;
};

/**
 * @brief Creates a new save panel
 *
 * This function allocates all required memory for
 * displaying the save panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @return a panel pointer
 */
extern PANEL *
save_raw_create();

/**
 * @brief Destroy save panel
 *
 * This function do the final cleanups for this panel
 */
extern void
save_raw_destroy();

/**
 * @brief Manage pressed keys for save panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the save panel to manage
 * its own keys.
 * If this function return 0, the key will not be handled
 * by ui manager. Otherwise the return will be considered
 * a key code.
 *
 * @param panel Save panel pointer
 * @param key   key code
 * @return 0 if the key is handled, keycode otherwise
 */
extern int
save_raw_handle_key(PANEL *panel, int key);

/**
 * @brief Print an error message in Save panel
 *
 * General function to print any save error message
 * @param panel Save panel pointer
 * @param message Message to be printed in the panel
 */
extern void
save_raw_error_message(PANEL *panel, const char *message);

/**
 * @brief Set the group of calls to save raw data
 *
 * This functions sets the internal pointer to the
 * call group which information will be saved
 *
 * @param panel Save Raw panel pointer
 * @param group Group of calls displayed in the raw panel
 */
extern void
save_raw_set_group(PANEL *panel, sip_call_group_t *group);

/**
 * @brief Save form data to options
 *
 * This function will try to copy the temporal file to
 * another location user entered
 *
 * @param panel Save panel pointer
 */
extern int
save_raw_to_file(PANEL *panel);

#endif
