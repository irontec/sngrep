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
 * @file {file}
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_DIALOG_H__
#define __SNGREP_DIALOG_H__

#include <glib.h>
#include <glib-object.h>
#include <form.h>
#include "tui/widgets/window.h"

G_BEGIN_DECLS

#define SNG_DIALOG_MIN_HEIGHT   5
#define SNG_DIALOG_MIN_WIDTH    40

// Class declaration
#define SNG_TYPE_DIALOG sng_dialog_get_type()
G_DECLARE_FINAL_TYPE(SngDialog, sng_dialog, SNG, DIALOG, SngWindow)

typedef enum
{
    SNG_BUTTONS_NONE,
    SNG_BUTTONS_ACCEPT,
    SNG_BUTTONS_OK,
    SNG_BUTTONS_CANCEL,
    SNG_BUTTONS_YES_NO,
    SNG_BUTTONS_OK_CANCEL
} SngDialogButtons;

typedef enum
{
    SNG_DIALOG_INFO,
    SNG_DIALOG_WARNING,
    SNG_DIALOG_QUESTION,
    SNG_DIALOG_ERROR,
    SNG_DIALOG_OTHER
} SngDialogType;

typedef enum
{
    SNG_RESPONSE_ACCEPT,
    SNG_RESPONSE_OK,
    SNG_RESPONSE_CANCEL,
    SNG_RESPONSE_YES,
    SNG_RESPONSE_NO,
} SngDialogResponse;

struct _SngDialog
{
    //! Parent object attributes
    SngWindow parent;
    //! Dialog loop
    GMainLoop *loop;
    //! Dialog type
    SngDialogType type;
    //! Dialog buttons
    SngDialogButtons buttons;
    //! Dialog text message
    gchar *message;
    //! Dialog response
    SngDialogResponse response;
};

SngWidget *
sng_dialog_new(SngDialogType type, SngDialogButtons buttons, const gchar *title, const gchar *message);

void
sng_dialog_show_message(const gchar *title, const gchar *format, ...);

gboolean
sng_dialog_confirm(const gchar *title, const gchar *format, ...);

SngDialogResponse
sng_dialog_run(SngDialog *dialog);


#endif    /* __SNGREP_DIALOG_H__ */
