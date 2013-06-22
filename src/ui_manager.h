/**************************************************************************
 **
 **  sngrep - Ncurses ngrep interface for SIP
 **
 **   Copyright (C) 2013 Ivan Alonso (Kaian)
 **   Copyright (C) 2013 Irontec SL. All rights reserved.
 **
 **   This program is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   This program is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
#ifndef __SNGREP_UI_MANAGER_H
#define __SNGREP_UI_MANAGER_H
#include <ncurses.h>
#include <panel.h>
#include "sip.h"

/**
 * The actual UI is not very flexible. It requires a lot of space to 
 * be correctyle drawn.. It would be nice to be more adaptative and hide
 * some columns in main panel depending on the available columns 
 */
#define UI_MIN_COLS     175

//! Shorter declaration of ui structure
typedef struct ui ui_t;

/**
 * @brief Panel information structure
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct ui
{
    //! Panel Type @see panel_types enum
    int type;         
    //! The actual ncurses panel pointer
    PANEL *panel;
    //! Constructor for this panel
    PANEL *(*create)();
    //! Reques the panel to redraw its data
    int (*draw)(PANEL*);
    //! Handle a custom keybind on this panel
    int (*handle_key)(PANEL*, int key);
    //! Show help window for this panel (if any)
    int (*help)(PANEL *);
    //! Destroy current panel
    void (*destroy)(PANEL *);
};

/**
 * Enum for available color pairs
 * Colors for each pair are chosen in toggle_colors function
 */
enum sngrep_colors
{
    HIGHLIGHT_COLOR = 1,
    HELP_COLOR,
    OUTGOING_COLOR,
    INCOMING_COLOR,
    DETAIL_BORDER_COLOR,
    DETAIL_WIN_COLOR,
};

/**
 * Enum for available panel types
 * Mostly used for managing keybindings and offloop ui refresh
 */
enum panel_types
{
    MAIN_PANEL = 0,
    MHELP_PANEL,
    DETAILS_PANEL,
    DETAILS_PANEL_EX,
    RAW_PANEL,
};

/**
 * Interface configuration.
 * If some day a rc file is created, its data will be loaded
 * into this structure. 
 * By now, we'll store some ui information.
 */
struct ui_config
{
    int color;
    int online; /* 0 - Offline, 1 - Online */
    const char *fname; /* Filename in offline mode */
};

/**
 * Initialize ncurses mode and create a main window
 * 
 * @param ui_config UI configuration structure
 * @returns 0 on ncurses initialization success, 1 otherwise 
 */
int init_interface(const struct ui_config);

/**
 * @brief Toggle color mode on and off
 * @param on Pass 0 to turn all black&white
 */
void toggle_color(int on);

/**
 * @brief Wait for user input.
 * This function manages all user input in all panel types and
 * redraws the panel using its own draw function
 * 
 * @param panel the topmost panel ui structure
 */
void wait_for_input(ui_t *ui);


void ui_draw_panel(ui_t *ui);

/**
 * Draw a box around passed windows with two bars (top and bottom)
 * of one line each.
 *
 * @param win Window to draw borders on
 */
void title_foot_box(WINDOW *win);

/**
 * This function is invocked asynchronously from the
 * ngrep exec thread to notify a new message of the giving
 * callid. If the UI is displaying this call or it's 
 * extended one, the topmost panel will be redraw again 
 *
 * @param callid Call-ID from the last received message
 */
void refresh_call_ui(const char *callid);

/**
 * @brief Create a panel structure
 */
ui_t *ui_create(int type);

/**
 * @berif Destroy a panel structure 
 */
void ui_destroy(ui_t *ui);

PANEL *ui_get_panel(ui_t *ui);
void ui_help(ui_t *ui);
void ui_handle_key(ui_t *ui, int key);

/**
 * @brief Find a ui from its pannel pointer
 */
ui_t *ui_find_by_panel(PANEL *panel);

/**
 * @brief Find a ui form its panel id
 */
ui_t *ui_find_by_type(int type);

#endif    // __SNGREP_UI_MANAGER_H
