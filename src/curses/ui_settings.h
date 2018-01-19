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
 * @file ui_settings.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to change sngrep configurable settings
 *
 * This file contains the functions to display the interface panel that handles
 * the changes of settings in realtime, also allowing to save them.
 *
 */

#ifndef __SNGREP_UI_SETTINGS_H
#define __SNGREP_UI_SETTINGS_H

//! Sorter declaration of struct option_info
typedef struct settings_info settings_info_t;
//! Sorter declaration of struct settings_category
typedef struct settings_category settings_category_t;
//! Sorter declaration of struct settings_entry
typedef struct settings_entry settings_entry_t;

enum settings_category_list {
    CAT_SETTINGS_INTERFACE = 1,
    CAT_SETTINGS_CAPTURE,
    CAT_SETTINGS_CALL_FLOW,
    CAT_SETTINGS_EEP_HOMER,
    CAT_SETTINGS_COUNT,
};

/**
 * @brief Enum of available dialog fields
 *
 * Dialog form has a field array. Following enum represents the
 * order this fields are stored in panel info structure.
 *
 */
enum settings_field_list {
    FLD_SETTINGS_BACKGROUND = 0,
    FLD_SETTINGS_BACKGROUND_LB,
    FLD_SETTINGS_SYNTAX,
    FLD_SETTINGS_SYNTAX_LB,
    FLD_SETTINGS_SYNTAX_TAG,
    FLD_SETTINGS_SYNTAX_TAG_LB,
    FLD_SETTINGS_SYNTAX_BRANCH,
    FLD_SETTINGS_SYNTAX_BRANCH_LB,
    FLD_SETTINGS_ALTKEY_HINT,
    FLD_SETTINGS_ALTKEY_HINT_LB,
    FLD_SETTINGS_COLORMODE,
    FLD_SETTINGS_COLORMODE_LB,
    FLD_SETTINGS_EXITPROMPT,
    FLD_SETTINGS_EXITPROMPT_LB,
    FLD_SETTINGS_DISPLAY_ALIAS,
    FLD_SETTINGS_DISPLAY_ALIAS_LB,
    FLD_SETTINGS_CAPTURE_LIMIT,
    FLD_SETTINGS_CAPTURE_LIMIT_LB,
    FLD_SETTINGS_CAPTURE_DEVICE,
    FLD_SETTINGS_CAPTURE_DEVICE_LB,
    FLD_SETTINGS_SIP_NOINCOMPLETE,
    FLD_SETTINGS_SIP_NOINCOMPLETE_LB,
    FLD_SETTINGS_SAVEPATH,
    FLD_SETTINGS_SAVEPATH_LB,
    FLD_SETTINGS_CF_FORCERAW,
    FLD_SETTINGS_CF_FORCERAW_LB,
    FLD_SETTINGS_CF_SPLITCACALLID,
    FLD_SETTINGS_CF_SPLITCACALLID_LB,
    FLD_SETTINGS_CF_SDPONLY,
    FLD_SETTINGS_CF_SDPONLY_LB,
    FLD_SETTINGS_CF_SCROLLSTEP,
    FLD_SETTINGS_CF_SCROLLSTEP_LB,
    FLD_SETTINGS_CF_HIGHTLIGHT,
    FLD_SETTINGS_CF_HIGHTLIGHT_LB,
    FLD_SETTINGS_CF_LOCALHIGHLIGHT,
    FLD_SETTINGS_CF_LOCALHIGHLIGHT_LB,
    FLD_SETTINGS_CF_DELTA,
    FLD_SETTINGS_CF_DELTA_LB,
    FLD_SETTINGS_CF_MEDIA,
    FLD_SETTINGS_CF_MEDIA_LB,
#ifdef USE_EEP
    FLD_SETTINGS_EEP_SEND,
    FLD_SETTINGS_EEP_SEND_LB,
    FLD_SETTINGS_EEP_SEND_VER,
    FLD_SETTINGS_EEP_SEND_VER_LB,
    FLD_SETTINGS_EEP_SEND_ADDR,
    FLD_SETTINGS_EEP_SEND_ADDR_LB,
    FLD_SETTINGS_EEP_SEND_PORT,
    FLD_SETTINGS_EEP_SEND_PORT_LB,
    FLD_SETTINGS_EEP_SEND_PASS,
    FLD_SETTINGS_EEP_SEND_PASS_LB,
    FLD_SETTINGS_EEP_SEND_ID,
    FLD_SETTINGS_EEP_SEND_ID_LB,
    FLD_SETTINGS_EEP_LISTEN,
    FLD_SETTINGS_EEP_LISTEN_LB,
    FLD_SETTINGS_EEP_LISTEN_VER,
    FLD_SETTINGS_EEP_LISTEN_VER_LB,
    FLD_SETTINGS_EEP_LISTEN_ADDR,
    FLD_SETTINGS_EEP_LISTEN_ADDR_LB,
    FLD_SETTINGS_EEP_LISTEN_PORT,
    FLD_SETTINGS_EEP_LISTEN_PORT_LB,
    FLD_SETTINGS_EEP_LISTEN_PASS,
    FLD_SETTINGS_EEP_LISTEN_PASS_LB,
	FLD_SETTINGS_EEP_LISTEN_UUID,
	FLD_SETTINGS_EEP_LISTEN_UUID_LB,
#endif
    FLD_SETTINGS_COUNT,
};

enum settings_button_list {
    BTN_SETTINGS_ACCEPT = 0,
    BTN_SETTINGS_SAVE,
    BTN_SETTINGS_CANCEL,
    BTN_SETTINGS_COUNT,
};

#define SETTINGS_ENTRY_COUNT (FLD_SETTINGS_COUNT - 3)

struct settings_category {
    // Category id
    enum settings_category_list cat_id;
    // Category label
    const char *title;
};

struct settings_entry {
    enum settings_category_list cat_id;
    //! Field id in settings_info array
    enum settings_field_list field_id;
    //! Setting id of current entry
    enum setting_id setting_id;
    //! Entry text
    const char *label;
};

/**
 * @brief settings panel private information
 *
 * This structure contains the durable data of settings panel.
 */
struct settings_info {
    // Window containing form data (and buttons)
    WINDOW *form_win;
    //! Form that contains the filter fields
    FORM *form;
    //! An array of fields
    FIELD *fields[FLD_SETTINGS_COUNT + 1];
    //! Form that contains the buttons
    FORM *buttons_form;
    //! Array of panel buttons
    FIELD *buttons[BTN_SETTINGS_COUNT + 1];
    //! Active form
    FORM *active_form;
    //! Active category
    enum settings_category_list active_category;
};

/**
 * @brief Creates a new settings panel
 *
 * This function allocates all required memory for
 * displaying the save panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @param ui UI structure pointer
 */
void
settings_create(ui_t *ui);

/**
 * @brief Destroy settings panel
 *
 * This function do the final cleanups for this panel
 *
 * @param ui UI structure pointer
 */
void
settings_destroy(ui_t *ui);

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param ui UI structure pointer
 * @return a pointer to info structure of given panel
 */
settings_info_t *
settings_info(ui_t *ui);

/**
 * @brief Draw the settings panel
 *
 * This function will drawn the panel into the screen with
 * current status settings
 *
 * @param ui UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
int
settings_draw(ui_t *ui);

/**
 * @brief Manage pressed keys for settings panel
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
settings_handle_key(ui_t *ui, int key);

/**
 * @brief Return entry information of the field
 *
 * If field is storing a setting value, return the entry
 * structure associated to the setting
 *
 * @param field Ncurses field pointer of screen
 * @return Setting information structure
 */
settings_entry_t *
ui_settings_is_entry(FIELD *field);

/**
 * @brief Update settings with panel values
 *
 * Update all settings with the selected on screen.
 * Note that some settings require application restart to
 * take effect.
 *
 * @param ui UI structure pointer
 * @return 0 in all cases
 */
int
ui_settings_update_settings(ui_t *ui);

/**
 * @brief Update user resource file with panel values
 *
 * Save all settings into user configuration file located
 * in it's home directory.
 *
 * @param ui UI structure pointer
 */
void
ui_settings_save(ui_t *ui);

#endif /* __SNGREP_UI_SETTINGS_H */
