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
    gboolean (*redraw)(SngAppWindow *self);
    //! Notifies the panel the screen has changed
    gint (*resize)(SngAppWindow *self);
    //! Show help window for this panel (if any)
    gint (*help)(SngAppWindow *self);
};

/**
 * @brief Create a ncurses panel for the given ui
 *
 * Create a panel and associated window and store their
 * pointers in ui structure.
 * If height and width doesn't match the screen dimensions
 * the panel will be centered on the screen.
 *
 * @param height panel window height
 * @param width panel windo width
 * @return window instance pointer
 */
SngAppWindow *
sng_app_window_new(gint height, gint width);

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

/**
 * @brief Draw title at the top of the panel UI
 *
 * This function will draw a line with the title on the first
 * row of the UI panel's window
 *
 * @param ui UI structure
 * @param title String containing the title
 */
void
sng_app_window_set_title(SngAppWindow *window, const gchar *title);

/**
 * @brief Clear a given window line
 *
 * This function can be used to clear a given line on the
 * screen.
 *
 * @param window UI structure
 * @param line Number of line to be cleared
 */
void
sng_app_window_clear_line(SngAppWindow *window, int line);

/**
 * @brief Draw keybinding info at the bottom of the panel
 *
 * This function will draw a line with the available keybindings
 * in the last line of the given panel
 *
 */
void
sng_app_window_draw_bindings(SngAppWindow *window, const gchar **keybindings, gint count);

void
sng_app_window_set_window_type(SngAppWindow *window, SngAppWindowType type);

guint
sng_app_window_get_window_type(SngAppWindow *window);

G_END_DECLS

#endif /* __SNGREP_APP_WINDOW_H__ */
