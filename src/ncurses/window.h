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
 * @file u_panel.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Common process for all Interface panels
 *
 * This file contains common functions shared by all panels.
 */

#ifndef __SNGREP_UI_PANEL_H
#define __SNGREP_UI_PANEL_H

#include <glib.h>
#include <ncursesw/ncurses.h>
#include <panel.h>
#include <form.h>

//! Possible key handler results
enum WindowKeyHandlerRet {
    //! Panel has handled the key, dont'use default key handler
    KEY_HANDLED         = 0,
    //! Panel has not handled the key, try defualt key handler
    KEY_NOT_HANDLED     = -1,
    //! Panel destroys and requests previous panel to handle key
    KEY_PROPAGATED      = -2,
    //! Panel request destroy
    KEY_DESTROY         = -3
};

/**
 * @brief Enum for available panel types
 *
 * Mostly used for managing keybindings and offloop ui refresh
 */
enum WindowTypes {
    //! Call List ui screen
    WINDOW_CALL_LIST = 0,
    //! Call-Flow ui screen
    WINDOW_CALL_FLOW,
    //! Raw SIP messages ui screen
    WINDOW_CALL_RAW,
    //! Filters panel
    WINDOW_FILTER,
    //! Save to pcap panel
    WINDOW_SAVE,
    //! Message comprare
    WINDOW_MSG_DIFF,
    //! Column selector panel
    WINDOW_COLUMN_SELECT,
    //! Settings panel
    WINDOW_SETTINGS,
    //! Stats panel
    WINDOW_STATS,
    //! Panel Counter
    PANEL_COUNT,
};

//! Shorter declaration of ui structure
typedef struct _Window Window;

/**
 * @brief Panel information structure
 *
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct _Window {
    //! Curses panel pointer
    PANEL *panel;
    //! Window for the curses panel
    WINDOW *win;
    //! Height of the curses window
    int height;
    //! Width of the curses window
    int width;
    //! Vertical starting position of the window
    int x;
    //! Horizontal starting position of the window
    int y;
    //! Panel Type @see PanelTypes enum
    enum WindowTypes type;
    //! Flag this panel as redraw required
    gboolean changed;

    //! Constructor for this panel
    void (*create)(Window *);
    //! Destroy current panel
    void (*destroy)(Window *);
    //! Query the panel if redraw is required
    gboolean (*redraw)(Window *);
    //! Request the panel to redraw its data
    int (*draw)(Window *);
    //! Notifies the panel the screen has changed
    int (*resize)(Window *);
    //! Handle a custom keybind on this panel
    int (*handle_key)(Window *, int key);
    //! Show help window for this panel (if any)
    int (*help)(Window *);
};

/**
 * @brief Create a panel structure
 *
 * Create a ncurses panel associated to the given ui
 * This function is a small wrapper for panel create function
 *
 * @param window UI structure
 * @return the ui structure with the panel pointer created
 */
Window *
window_create(Window *window);

/**
 * @brief Destroy a panel structure
 *
 * Removes the panel associatet to the given ui and free
 * its memory. Most part of this task is done in the custom
 * destroy function of the panel.
 *
 * @param window UI structure
 */
void
window_destroy(Window *window);

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
int
window_handle_key(Window *window, int key);

/**
 * @brief Create a ncurses panel for the given ui
 *
 * Create a panel and associated window and store their
 * porinters in ui structure.
 * If height and widht doesn't match the screen dimensions
 * the panel will be centered on the screen.
 *
 * @param window UI structure
 * @param height panel window height
 * @param width panel windo width
 */
void
window_init(Window *window, int height, int width);

/**
 * @brief Deallocate ncurses panel and window
 *
 * @param ui UI structure
 */
void
window_deinit(Window *window);

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
window_draw_bindings(Window *window, const char **keybindings, int count);

#endif /* __SNGREP_UI_PANEL_H */
