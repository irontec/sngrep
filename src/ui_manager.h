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
    PANEL *
    (*create)();
    //! Destroy current panel
    void
    (*destroy)(PANEL *);
    //! Request the panel to redraw its data
    int
    (*draw)(PANEL*);
    //! Check if the panel request redraw with given msg
    int
    (*redraw_required)(PANEL *, sip_msg_t *);
    //! Handle a custom keybind on this panel
    int
    (*handle_key)(PANEL*, int key);
    //! Show help window for this panel (if any)
    int
    (*help)(PANEL *);
    //! Replace current UI with the following in the next update
    ui_t *replace;
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
    DETAILS_EX_PANEL,
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
extern int
init_interface(const struct ui_config);

/**
 * @brief Create a panel structure
 */
extern ui_t *
ui_create(ui_t *ui);

/**
 * @brief Destroy a panel structure 
 */
extern void
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
extern PANEL *
ui_get_panel(ui_t *ui);

/**
 * @brief Check if the given msg makes the UI redraw
 *
 * This function is ivoked every time a new package is readed
 * in online mode. Check if the ui needs to be redrawn with the
 * message to avioid not needed work.
 *
 * @param ui UI structure
 * @param msg las readed message
 * @return 0 in case of redraw required, -1 otherwise
 */
extern int
ui_redraw_required(ui_t *ui, sip_msg_t *msg);

/**
 * @brief Redrawn current ui
 *
 * This function acts as wrapper to custom ui draw functions
 * with some checks
 *
 * @param ui UI structure
 * @return 0 if ui has been drawn, -1 otherwise
 */
extern int
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
extern void
ui_help(ui_t *ui);

/**
 * @brief Handle key inputs on given UI
 *
 * This function will pass the input key sequence
 * to the given UI. This will only happen if the key
 * sequence don't match any of the general keybindings
 *
 * @param ui UI structure
 * @param key keycode sequence of the pressed keys and mods
 */
extern int
ui_handle_key(ui_t *ui, int key);

/**
 * @brief Find a ui from its pannel pointer
 */
extern ui_t *
ui_find_by_panel(PANEL *panel);

/**
 * @brief Find a ui form its panel id
 */
extern ui_t *
ui_find_by_type(int type);
/**
 * @brief Toggle color mode on and off
 * @param on Pass 0 to turn all black&white
 */
extern void
toggle_color(int on);

/**
 * @brief Wait for user input.
 * This function manages all user input in all panel types and
 * redraws the panel using its own draw function
 * 
 * @param panel the topmost panel ui structure
 */
extern int
wait_for_input(ui_t *ui);

/**
 * Draw a box around passed windows with two bars (top and bottom)
 * of one line each.
 *
 * @param win Window to draw borders on
 */
extern void
title_foot_box(WINDOW *win);

/**
 * This function is invocked asynchronously from the
 * ngrep exec thread to notify a new message of the giving
 * callid. If the UI is displaying this call or it's 
 * extended one, the topmost panel will be redraw again 
 *
 * @param callid Call-ID from the last received message
 */
extern void
ui_new_msg_refresh(sip_msg_t *msg);

/**
 * @brief Replace one UI with another
 *
 * This function can be handy when we want to destroy one UI element
 * and replace it for another.
 */
extern int
ui_set_replace(ui_t *, ui_t*);

#endif    // __SNGREP_UI_MANAGER_H
