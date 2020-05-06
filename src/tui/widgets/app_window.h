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
 * @file window.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Common process for all Interface panels
 *
 * This file contains common functions shared by all panels.
 */

#ifndef __SNGREP_APP_WINDOW_H__
#define __SNGREP_APP_WINDOW_H__

#include <glib.h>
#include <glib-object.h>
#include <ncurses.h>
#include <panel.h>
#include "tui/widgets/menu.h"
#include "tui/widgets/window.h"

G_BEGIN_DECLS

#define SNG_TYPE_APP_WINDOW sng_app_window_get_type()
G_DECLARE_DERIVABLE_TYPE(SngAppWindow, sng_app_window, SNG, APP_WINDOW, SngWindow)

/**
 * @brief Enum for available panel types
 *
 * Mostly used for managing keybindings and offloop ui refresh
 */
typedef enum
{
    SNG_WINDOW_TYPE_CALL_LIST,
    SNG_WINDOW_TYPE_CALL_FLOW,
    SNG_WINDOW_TYPE_CALL_RAW,
    SNG_WINDOW_TYPE_FILTER,
    SNG_WINDOW_TYPE_SAVE,
    SNG_WINDOW_TYPE_MSG_DIFF,
    SNG_WINDOW_TYPE_COLUMN_SELECT,
    SNG_WINDOW_TYPE_SETTINGS,
    SNG_WINDOW_TYPE_AUTH_VALIDATE,
    SNG_WINDOW_TYPE_STATS,
    SNG_WINDOW_TYPE_RTP_PLAYER,
    SNG_WINDOW_TYPE_PROTOCOL_SELECT,
} SngAppWindowType;

/**
 * @brief Panel information structure
 *
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct _SngAppWindowClass
{
    //! Parent class
    SngWindowClass parent;
    //! Query the panel if redraw is required
    gboolean (*redraw)(SngAppWindow *app_window);
    //! Notifies the panel the screen has changed
    gint (*resize)(SngAppWindow *app_window);
    //! Show help window for this panel (if any)
    gint (*help)(SngAppWindow *app_window);
};

/**
 * @brief Resize current ui
 *
 * This function acts as wrapper to custom ui draw functions
 * with some checks
 *
 * @param ui UI structure
 * @return 0 if ui has been drawn, -1 otherwise
 */
int
sng_app_window_resize(SngAppWindow *window);

/**
 * @brief Check if the panel requires redraw
 *
 * This function acts as wrapper to custom ui redraw function
 * with some checks
 *
 * @param app_window UI structure
 * @return true if the panel must be drawn, false otherwise
 */
gboolean
sng_app_window_redraw(SngAppWindow *app_window);

/**
 * @brief Show help screen from current UI (if any)
 *
 * This function will display the help screen for given
 * ui if exits.
 * All help screens exits after any character input
 *
 * @param window UI structure
 */
void
sng_app_window_help(SngAppWindow *window);

void
sng_app_window_add_menu(SngAppWindow *window, SngMenu *menu);

void
sng_app_window_add_button(SngAppWindow *window, const gchar *label, SngAction action);

SngWidget *
sng_app_window_get_content(SngAppWindow *app_window);

G_END_DECLS

#endif /* __SNGREP_APP_WINDOW_H__ */
