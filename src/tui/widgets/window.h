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
#include "tui/widgets/button.h"
#include "tui/widgets/box.h"

G_BEGIN_DECLS

#define SNG_TYPE_WINDOW sng_window_get_type()
G_DECLARE_DERIVABLE_TYPE(SngWindow, sng_window, SNG, WINDOW, SngBox)

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

PANEL *
sng_window_get_ncurses_panel(SngWindow *window);

void
sng_window_set_title(SngWindow *window, const gchar *title);

void
sng_window_add_button(SngWindow *window, SngButton *button);

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
