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

G_BEGIN_DECLS

#define NCURSES_TYPE_WINDOW window_get_type()
G_DECLARE_DERIVABLE_TYPE(Window, window, NCURSES, WINDOW, GObject)

//! Possible key handler results
typedef enum
{
    KEY_HANDLED = 0,        // Panel has handled the key, dont'use default key handler
    KEY_NOT_HANDLED = -1,   // Panel has not handled the key, try default key handler
    KEY_PROPAGATED = -2,    // Panel destroys and requests previous panel to handle key
    KEY_DESTROY = -3,       // Panel request destroy
} WindowKeyHandlerRet;

/**
 * @brief Enum for available panel types
 *
 * Mostly used for managing keybindings and offloop ui refresh
 */
typedef enum
{
    WINDOW_CALL_LIST = 0,    // Call List ui screen
    WINDOW_CALL_FLOW,        // Call-Flow ui screen
    WINDOW_CALL_RAW,         // Raw SIP messages ui screen
    WINDOW_FILTER,           // Filters panel
    WINDOW_SAVE,             // Save to pcap panel
    WINDOW_MSG_DIFF,         // Message compare
    WINDOW_COLUMN_SELECT,    // Column selector panel
    WINDOW_SETTINGS,         // Settings panel
    WINDOW_AUTH_VALIDATE,    // Authentication validator panel
    WINDOW_STATS,            // Stats panel
    WINDOW_RTP_PLAYER,       // RTP Player panel
    WINDOW_PROTOCOL_SELECT,  // RTP Player panel
} WindowType;

/**
 * @brief Panel information structure
 *
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct _WindowClass
{
    //! Parent class
    GObjectClass parent;
    //! Query the panel if redraw is required
    gboolean (*redraw)(Window *self);
    //! Request the panel to redraw its data
    gint (*draw)(Window *self);
    //! Notifies the panel the screen has changed
    gint (*resize)(Window *self);
    //! Handle a custom keybinding on this panel
    gint (*handle_key)(Window *self, gint key);
    //! Show help window for this panel (if any)
    gint (*help)(Window *self);
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
Window *
window_new(gint height, gint width);

/**
 * @brief Destroy a panel structure
 *
 * Removes the panel associated to the given ui and free
 * its memory. Most part of this task is done in the custom
 * destroy function of the panel.
 *
 * @param window UI structure
 */
void
window_free(Window *window);

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
window_resize(Window *window);

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
window_redraw(Window *window);

/**
 * @brief Notifies current ui the screen size has changed
 *
 * This function acts as wrapper to custom ui resize functions
 * with some checks
 *
 * @param window UI structure
 * @return 0 if ui has been resize, -1 otherwise
 */
int
window_draw(Window *window);

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
window_help(Window *window);

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
window_handle_key(Window *window, gint key);

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
window_set_title(Window *window, const gchar *title);

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
window_clear_line(Window *window, int line);

/**
 * @brief Draw keybinding info at the bottom of the panel
 *
 * This function will draw a line with the available keybindings
 * in the last line of the given panel
 *
 */
void
window_draw_bindings(Window *window, const gchar **keybindings, gint count);

PANEL *
window_get_ncurses_panel(Window *window);

WINDOW *
window_get_ncurses_window(Window *window);

void
window_set_window_type(Window *window, WindowType type);

guint
window_get_window_type(Window *window);

void
window_set_width(Window *window, gint width);

gint
window_get_width(Window *window);

void
window_set_height(Window *window, gint height);

gint
window_get_height(Window *window);


G_END_DECLS

#endif /* __SNGREP_WINDOW_H__ */
