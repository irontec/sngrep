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

#ifndef __SNGREP_WINDOW_H__
#define __SNGREP_WINDOW_H__

#include <glib.h>
#include <glib-object.h>
#include <ncurses.h>
#include <panel.h>
#include "tui/widgets/box.h"

G_BEGIN_DECLS

#define SNG_TYPE_WINDOW sng_window_get_type()
G_DECLARE_DERIVABLE_TYPE(SngWindow, sng_window, SNG, WINDOW, SngBox)

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
} SngWindowType;

/**
 * @brief Panel information structure
 *
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct _SngWindowClass
{
    //! Parent class
    SngBoxClass parent;
    //! Query the panel if redraw is required
    gboolean (*redraw)(SngWindow *self);
    //! Notifies the panel the screen has changed
    gint (*resize)(SngWindow *self);
    //! Show help window for this panel (if any)
    gint (*help)(SngWindow *self);
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
SngWindow *
sng_window_new(gint height, gint width);

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
sng_window_resize(SngWindow *window);

/**
 * @brief Check if the panel requires redraw
 *
 * This function acts as wrapper to custom ui redraw function
 * with some checks
 *
 * @param window UI structure
 * @return true if the panel must be drawn, false otherwise
 */
gboolean
sng_window_redraw(SngWindow *window);

/**
 * @brief Notifies current ui the screen size has changed
 *
 * This function acts as wrapper to custom ui resize functions
 * with some checks
 *
 * @param window UI structure
 * @return 0 if ui has been resize, -1 otherwise
 */
gint
sng_window_draw(SngWindow *window);

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
sng_window_help(SngWindow *window);

/**
 * @brief Handle moves events on given window
 *
 * This function will pass the mouse event
 * to the given window function.
 *
 * @return enum @key_handler_ret*
 */
gint
sng_window_handle_mouse(SngWindow *window, MEVENT mevent);

/**
 * @brief Handle key inputs on given UI
 *
 * This function will pass the input key sequence
 * to the given UI.
 *
 * @param ui UI structure
 * @param key keycode sequence of the pressed keys and mods
 * @return enum @key_handler_ret*
 */
gint
sng_window_handle_key(SngWindow *window, gint key);

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
sng_window_set_title(SngWindow *window, const gchar *title);

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
sng_window_clear_line(SngWindow *window, int line);

/**
 * @brief Draw keybinding info at the bottom of the panel
 *
 * This function will draw a line with the available keybindings
 * in the last line of the given panel
 *
 */
void
sng_window_draw_bindings(SngWindow *window, const gchar **keybindings, gint count);

PANEL *
sng_window_get_ncurses_panel(SngWindow *window);

WINDOW *
sng_window_get_ncurses_window(SngWindow *window);

void
sng_window_set_window_type(SngWindow *window, SngWindowType type);

guint
sng_window_get_window_type(SngWindow *window);

void
sng_window_set_width(SngWindow *window, gint width);

gint
sng_window_get_width(SngWindow *window);

void
sng_window_set_height(SngWindow *window, gint height);

gint
sng_window_get_height(SngWindow *window);

void
sng_window_set_default_focus(SngWindow *window, SngWidget *widget);

SngWidget *
sng_window_focused_widget(SngWindow *window);

void
sng_window_focus_next(SngWindow *window);

void
sng_window_focus_prev(SngWindow *window);

void
sng_window_set_focused_widget(SngWindow *window, SngWidget *widget);

G_END_DECLS

#endif /* __SNGREP_WINDOW_H__ */
