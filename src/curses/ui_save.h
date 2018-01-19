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
 * @file ui_save_pcap.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ui window for saving captured packages
 *
 * This file contains the functions and structures to manage the save
 * dialog, that can be used to copy the temporal sngrep file to another location
 *
 */

#ifndef __UI_SAVE_PCAP_H
#define __UI_SAVE_PCAP_H
#include "config.h"
#include <form.h>
#include "group.h"
#include "ui_manager.h"

/**
 * @brief Enum of available dialog fields
 *
 * Dialog form has a field array. Following enum represents the
 * order this fields are stored in panel info structure.
 */
enum save_field_list {
    FLD_SAVE_PATH = 0,
    FLD_SAVE_FILE,
    FLD_SAVE_ALL,
    FLD_SAVE_SELECTED,
    FLD_SAVE_DISPLAYED,
    FLD_SAVE_MESSAGE,
    FLD_SAVE_PCAP,
    FLD_SAVE_PCAP_RTP,
    FLD_SAVE_TXT,
    FLD_SAVE_SAVE,
    FLD_SAVE_CANCEL,
    FLD_SAVE_COUNT
};

/**
 * @brief Dialogs to be saved
 */
enum save_mode {
    SAVE_ALL = 0,
    SAVE_SELECTED,
    SAVE_DISPLAYED,
    SAVE_MESSAGE
};

/**
 * @brief Save file formats
 */
enum save_format {
    SAVE_PCAP = 0,
    SAVE_PCAP_RTP,
    SAVE_TXT
};

//! Sorter declaration of struct save_info
typedef struct save_info save_info_t;

/**
 * @brief Save panel private information
 *
 * This structure contains the durable data of save panel.
 */
struct save_info {
    //! Form that contains the save fields
    FORM *form;
    //! An array of fields
    FIELD *fields[FLD_SAVE_COUNT + 1];
    //! Save mode @see save_modes
    enum save_mode savemode;
    //! Save format @see save_formats
    enum save_format saveformat;
    //! Call group to be saved
    sip_call_group_t *group;
    //! Message to be saved
    sip_msg_t *msg;
};

/**
 * @brief Creates a new save panel
 *
 * This function allocates all required memory for
 * displaying the save panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @param ui UI structure pointer
 */
void
save_create(ui_t *ui);

/**
 * @brief Destroy save panel
 *
 * This function do the final cleanups for this panel
 *
 * @param ui UI structure pointer
 */
void
save_destroy(ui_t *ui);

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param ui UI structure pointer
 * @return a pointer to info structure of given panel
 */
save_info_t *
save_info(ui_t *ui);

/**
 * @brief Draw the Save panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param ui UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
int
save_draw(ui_t *ui);

/**
 * @brief Manage pressed keys for save panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the save panel to manage
 * its own keys.
 *
 * @param ui UI structure pointer
 * @param key   key code
 * @return enum @key_handler_ret
 */
int
save_handle_key(ui_t *ui, int key);

/**
 * @brief Set the group call of the panel
 *
 * This function will access the panel information and will set the
 * group call pointer to the selected calls
 *
 * @param ui UI structure pointer
 * @param group Call group pointer to be set in the internal info struct
 */
void
save_set_group(ui_t *ui, sip_call_group_t *group);

/**
 * @brief Set the SIP message to be saved
 *
 * This function will access the panel information and will set the
 * pointer to the selected SIP message
 *
 * @param ui UI structure pointer
 * @param msg SIP message pointer to be set in the internal info struct
 */
void
save_set_msg(ui_t *ui, sip_msg_t *msg);

/**
 * @brief Print an error message in Save panel
 *
 * General function to print any save error message
 *
 * @param ui UI structure pointer
 * @param message Message to be printed in the panel
 */
void
save_error_message(ui_t *ui, const char *message);

/**
 * @brief Save form data to options
 *
 * Save capture packets to a file based on selected modes on screen
 * It will display an error or success dialog before exit
 *
 * @param ui UI structure pointer
 * @returns 1 in case of error, 0 otherwise.
 */
int
save_to_file(ui_t *ui);

/**
 * @brief Save one SIP message into open file
 *
 * @param f File opened with fopen
 * @param msg a SIP Message
 */
void
save_msg_txt(FILE *f, sip_msg_t *msg);

#endif
