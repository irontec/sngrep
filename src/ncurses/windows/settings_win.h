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
 * @file settings_win.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to change sngrep configurable settings
 *
 * This file contains the functions to display the interface panel that handles
 * the changes of settings in realtime, also allowing to save them.
 *
 */

#ifndef __SNGREP_SETTINGS_WIN_H__
#define __SNGREP_SETTINGS_WIN_H__

#include <glib.h>
#include "ncurses/manager.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_SETTINGS settings_win_get_type()
G_DECLARE_FINAL_TYPE(SettingsWindow, settings_win, NCURSES, SETTINGS, Window)

typedef enum
{
    CAT_SETTINGS_INTERFACE = 1,
    CAT_SETTINGS_CAPTURE,
    CAT_SETTINGS_CALL_FLOW,
    CAT_SETTINGS_HEP_HOMER,
    CAT_SETTINGS_COUNT,
} SettingsWindowCategoryId;

/**
 * @brief Enum of available dialog fields
 *
 * Dialog form has a field array. Following enum represents the
 * order this fields are stored in panel info structure.
 *
 */
typedef enum
{
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
#ifdef USE_HEP
    FLD_SETTINGS_HEP_SEND,
    FLD_SETTINGS_HEP_SEND_LB,
    FLD_SETTINGS_HEP_SEND_VER,
    FLD_SETTINGS_HEP_SEND_VER_LB,
    FLD_SETTINGS_HEP_SEND_ADDR,
    FLD_SETTINGS_HEP_SEND_ADDR_LB,
    FLD_SETTINGS_HEP_SEND_PORT,
    FLD_SETTINGS_HEP_SEND_PORT_LB,
    FLD_SETTINGS_HEP_SEND_PASS,
    FLD_SETTINGS_HEP_SEND_PASS_LB,
    FLD_SETTINGS_HEP_SEND_ID,
    FLD_SETTINGS_HEP_SEND_ID_LB,
    FLD_SETTINGS_HEP_LISTEN,
    FLD_SETTINGS_HEP_LISTEN_LB,
    FLD_SETTINGS_HEP_LISTEN_VER,
    FLD_SETTINGS_HEP_LISTEN_VER_LB,
    FLD_SETTINGS_HEP_LISTEN_ADDR,
    FLD_SETTINGS_HEP_LISTEN_ADDR_LB,
    FLD_SETTINGS_HEP_LISTEN_PORT,
    FLD_SETTINGS_HEP_LISTEN_PORT_LB,
    FLD_SETTINGS_HEP_LISTEN_PASS,
    FLD_SETTINGS_HEP_LISTEN_PASS_LB,
    FLD_SETTINGS_HEP_LISTEN_UUID,
    FLD_SETTINGS_HEP_LISTEN_UUID_LB,
#endif
    FLD_SETTINGS_COUNT,
} SettingsWindowField;

typedef enum
{
    BTN_SETTINGS_ACCEPT = 0,
    BTN_SETTINGS_SAVE,
    BTN_SETTINGS_CANCEL,
    BTN_SETTINGS_COUNT,
} SettingsWindowButton;

#define SETTINGS_ENTRY_COUNT (FLD_SETTINGS_COUNT - 3)

typedef struct
{
    // Category id
    SettingsWindowCategoryId cat_id;
    // Category label
    const char *title;
} SettingsWindowCategory;

typedef struct
{
    SettingsWindowCategoryId cat_id;
    //! Field id in settings_info array
    SettingsWindowField field_id;
    //! Setting id of current entry
    enum SettingId setting_id;
    //! Entry text
    const gchar *label;
} SettingsWindowEntry;

/**
 * @brief settings panel private information
 *
 * This structure contains the durable data of settings panel.
 */
struct _SettingsWindow
{
    //! Parent object attributes
    Window parent;
    //! Window containing form data (and buttons)
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
    SettingsWindowCategoryId active_category;
};

/**
 * @brief Creates a new settings panel
 *
 * This function allocates all required memory for
 * displaying the save panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 */
Window *
settings_win_new();

/**
 * @brief Destroy settings panel
 *
 * This function do the final cleanups for this panel
 *
 * @param window UI structure pointer
 */
void
settings_win_free(Window *window);

G_END_DECLS

#endif /* __SNGREP_SETTINGS_WIN_H__ */
