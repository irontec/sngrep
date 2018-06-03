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

#ifdef WITH_UNICODE
#define _X_OPEN_SOURCE_EXTENDED
#include <wctype.h>
#endif
#include <ncurses.h>
#include <panel.h>
#include <form.h>
#include <stdbool.h>

//! Possible key handler results
enum key_handler_ret {
    //! Panel has handled the key, dont'use default key handler
    KEY_HANDLED         = 0,
    //! Panel has not handled the key, try defualt key handler
    KEY_NOT_HANDLED     = -1,
    //! Panel destroys and requests previous panel to handle key
    KEY_PROPAGATED      = -2
};

/**
 * @brief Enum for available panel types
 *
 * Mostly used for managing keybindings and offloop ui refresh
 */
enum panel_types {
    //! Call List ui screen
    PANEL_CALL_LIST = 0,
    //! Call-Flow ui screen
    PANEL_CALL_FLOW,
    //! Raw SIP messages ui screen
    PANEL_CALL_RAW,
    //! Filters panel
    PANEL_FILTER,
    //! Save to pcap panel
    PANEL_SAVE,
    //! Message comprare
    PANEL_MSG_DIFF,
    //! Column selector panel
    PANEL_COLUMN_SELECT,
    //! Settings panel
    PANEL_SETTINGS,
    //! Stats panel
    PANEL_STATS,
    //! Panel Counter
    PANEL_COUNT,
};

//! Shorter declaration of ui structure
typedef struct ui ui_t;

/**
 * @brief Panel information structure
 *
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct ui {
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
    //! Panel Type @see panel_types enum
    enum panel_types type;
    //! Flag this panel as redraw required
    bool changed;

    //! Constructor for this panel
    void (*create)(ui_t *);
    //! Destroy current panel
    void (*destroy)(ui_t *);
    //! Query the panel if redraw is required
    bool (*redraw)(ui_t *);
    //! Request the panel to redraw its data
    int (*draw)(ui_t *);
    //! Notifies the panel the screen has changed
    int (*resize)(ui_t *);
    //! Handle a custom keybind on this panel
    int (*handle_key)(ui_t *, int key);
    //! Show help window for this panel (if any)
    int (*help)(ui_t *);
};

/**
 * @brief Create a panel structure
 *
 * Create a ncurses panel associated to the given ui
 * This function is a small wrapper for panel create function
 *
 * @param ui UI structure
 * @return the ui structure with the panel pointer created
 */
ui_t *
ui_create(ui_t *ui);

/**
 * @brief Destroy a panel structure
 *
 * Removes the panel associatet to the given ui and free
 * its memory. Most part of this task is done in the custom
 * destroy function of the panel.
 *
 * @param ui UI structure
 */
void
ui_destroy(ui_t *ui);

/**
 * @brief Get panel pointer from an ui element
 *
 * Basic getter to get the Ncurses PANEL pointer
 * from ui structure. Use this instead of accessing
 * directly to the pointer.
 *
 * @param ui UI structure
 * @return ncurses panel pointer of given UI
 */
PANEL *
ui_get_panel(ui_t *ui);

/**
 * @brief Redrawn current ui
 *
 * This function acts as wrapper to custom ui draw functions
 * with some checks
 *
 * @param ui UI structure
 * @return 0 if ui has been drawn, -1 otherwise
 */
int
ui_resize_panel(ui_t *ui);

/**
 * @brief Check if the panel requires redraw
 *
 * This function acts as wrapper to custom ui redraw function
 * with some checks
 *
 * @param ui UI structure
 * @return true if the panel must be drawn, false otherwise
 */
bool
ui_draw_redraw(ui_t *ui);

/**
 * @brief Notifies current ui the screen size has changed
 *
 * This function acts as wrapper to custom ui resize functions
 * with some checks
 *
 * @param ui UI structure
 * @return 0 if ui has been resize, -1 otherwise
 */
int
ui_draw_panel(ui_t *ui);

/**
 * @brief Show help screen from current UI (if any)
 *
 * This function will display the help screen for given
 * ui if exits.
 * All help screens exits after any character input
 *
 * @param ui UI structure
 */
void
ui_help(ui_t *ui);

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
ui_handle_key(ui_t *ui, int key);

/**
 * @brief Create a ncurses panel for the given ui
 *
 * Create a panel and associated window and store their
 * porinters in ui structure.
 * If height and widht doesn't match the screen dimensions
 * the panel will be centered on the screen.
 *
 * @param ui UI structure
 * @param height panel window height
 * @param width panel windo width
 */
void
ui_panel_create(ui_t *ui, int height, int width);

/**
 * @brief Deallocate ncurses panel and window
 *
 * @param ui UI structure
 */
void
ui_panel_destroy(ui_t *ui);

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
ui_set_title(ui_t *ui, const char *title);

/**
 * @brief Clear a given window line
 *
 * This function can be used to clear a given line on the
 * screen.
 *
 * @param ui UI structure
 * @param line Number of line to be cleared
 */
void
ui_clear_line(ui_t *ui, int line);

/**
 * @brief Draw keybinding info at the bottom of the panel
 *
 * This function will draw a line with the available keybindings
 * in the last line of the given panel
 *
 */
void
ui_draw_bindings(ui_t *ui, const char *keybindings[], int count);

#endif /* __SNGREP_UI_PANEL_H */
