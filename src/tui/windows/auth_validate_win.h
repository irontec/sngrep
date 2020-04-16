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
 * @file auth_validate_win.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ui window for saving captured packets
 *
 * This file contains the functions and structures to manage the auth validator
 * dialog
 *
 */

#ifndef __SNGREP_AUTH_VALIDATE_WIN_H__
#define __SNGREP_AUTH_VALIDATE_WIN_H__

#include "config.h"
#include <form.h>
#include "storage/group.h"
#include "tui/tui.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_AUTH_VALIDATE auth_validate_win_get_type()
G_DECLARE_FINAL_TYPE(AuthValidateWindow, auth_validate_win, TUI, AUTH_VALIDATE, SngWindow)

/**
 * @brief Enum of available dialog fields
 *
 * Dialog form has a field array. Following enum represents the
 * order this fields are stored in panel info structure.
 */
typedef enum
{
    FLD_AUTH_PASS,
    FLD_AUTH_CLOSE,
    FLD_AUTH_COUNT
} AuthValidateWindowField;

/**
 * @brief Save panel private information
 *
 * This structure contains the durable data of auth validator panel.
 */
struct _AuthValidateWindow
{
    //! Parent object attributes
    SngWindow parent;
    //! Form that contains the validator fields
    FORM *form;
    //! An array of fields
    FIELD *fields[FLD_AUTH_COUNT + 1];
    //! Message to be checked
    Message *msg;
    //! Authorization method
    const gchar *method;
    //! Authorization username
    const gchar *username;
    //! Authorization realm
    const gchar *realm;
    //! Authorization uri
    const gchar *uri;
    //! Authorization algorithm
    const gchar *algorithm;
    //! Authorization nonce
    const gchar *nonce;
    //! Authorization domain
    const gchar *response;
    //! Authorization calculated value
    gchar *calculated;
};

/**
 * @brief Creates a new authorization validator panel
 *
 * This function allocates all required memory for
 * displaying the validator panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @param window UI structure pointer
 */
SngWindow *
auth_validate_win_new();

/**
 * @brief Destroy authorization validator  panel
 *
 * This function do the final cleanups for this panel
 *
 * @param window UI structure pointer
 */
void
auth_validate_win_free(SngWindow *window);

/**
 * @brief Set the group call of the panel
 *
 * This function will access the panel information and will set the
 * group call pointer to the selected calls
 *
 * @param window UI structure pointer
 * @param group Call group pointer to be set in the internal info struct
 */
void
auth_validate_win_set_group(SngWindow *window, CallGroup *group);

/**
 * @brief Set the SIP message to be validated
 *
 * This function will access the panel information and will set the
 * pointer to the selected SIP message
 *
 * @param window UI structure pointer
 * @param msg SIP message pointer to be set in the internal info struct
 */
void
auth_validate_win_set_msg(SngWindow *window, Message *msg);

G_END_DECLS

#endif  /* __SNGREP_AUTH_VALIDATE_WIN_H__ */
