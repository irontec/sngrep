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
 * @file manager.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage interface panels
 *
 * All sngrep panel pointers are encapsulated into a ui structure that is
 * used to invoke custom functions for creating, destroying, drawing, etc
 * the screens.
 *
 * This sctructure also manages concurrents updates and access to ncurses
 * panel pointers.
 *
 */
#ifndef __SNGREP_UI_MANAGER_H
#define __SNGREP_UI_MANAGER_H

#include "config.h"
#include "ncurses/window.h"
#include "ncurses/theme.h"
#include "ncurses/keybinding.h"
#include "storage.h"
#include "group.h"
#include "setting.h"

//! Refresh UI every 200 ms
#define REFRESHTHSECS   2

//! Error reporting
#define NCURSES_ERROR (capture_pcap_error_quark())

//! Error codes
enum ncurses_errors
{
    NCURSES_ERROR_INIT = 0,
};

/**
 * @brief Initialize ncurses mode
 *
 * @param error GError with failure description (optional)
 * @return TRUE on initialization success, FALSE otherwise
 */
gboolean
ncurses_init(GMainLoop *loop, GError **error);

/**
 * @brief Stops ncurses mode
 *
 * This functions will deinitialize ncurse mode
 *
 * @returns 0 on ncurses initialization success, 1 otherwise
 */
void
ncurses_deinit();

/**
 * @brief Create a panel of a given type
 *
 * Create a ncurses panel of the given type.
 * This function is a small wrapper for panel create function
 *
 * @param type Panel Type
 * @return the ui structure with the panel pointer created*
 */
Window *
ncurses_create_window(enum WindowTypes type);

/**
 * @brief Find a ui from its pannel pointer
 */
Window *
ncurses_find_by_panel(PANEL *panel);

/**
 * @brief Find a ui form its panel id
 */
Window *
ncurses_find_by_type(enum WindowTypes type);

/**
 * @brief Default handler for keys
 *
 * If ui doesn't handle the given key (ui_handle_key returns the key value)
 * then the default handler will be invoked
 *
 * @param window Current displayed UI structure
 * @param key key pressed by user
 */
int
ncurses_default_keyhandler(Window *window, int key);

/**
 * @brief Call Resize function in all panels in the stack
 *
 * This function acts as handler of screen resize function invoking all
 * resize functions of panels that implement it.
 */
void
ncurses_resize_panels();

/**
 * @brief Draw a box around passed windows
 *
 * Draw a box around passed windows  with two bars
 * (top and bottom) of one line each.
 *
 * @param win Window to draw borders on
 */
void
title_foot_box(PANEL *panel);

/**
 * @brief Draw a message payload in a window
 *
 * Generic drawing function for payload. This function will start
 * writting at 0,0 and return the number of lines written.
 *
 * @param win Ncurses window to draw payload
 * @param msg Msg to be drawn
 */
int
draw_message(WINDOW *win, Message *msg);

/**
 * @brief Draw a message payload in a window starting at a given line
 *
 * Generic drawing function for payload. This function will start
 * writting at line starting and first column and return the number
 * of lines written.
 *
 * @param win Ncurses window to draw payload
 * @param msg Msg to be drawn
 * @param starting Number of win line to start writting payload
 */
int
draw_message_pos(WINDOW *win, Message *msg, int starting);

#endif    // __SNGREP_UI_MANAGER_H
