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
#ifndef __SNGREP_CURSES_H
#define __SNGREP_CURSES_H

#include <ncurses.h>
#include <panel.h>
#include "sip.h"

/**
 * The actual UI is not very flexible. It requires a lot of space to 
 * be correctyle drawn.. It would be nice to be more adaptative and hide
 * some columns in main panel depending on the available columns 
 */
#define UI_MIN_COLS	175

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
 * Panel information structure
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct panel_info
{
    int type; /* Panel Type @see panel_types enum */
    int highlight; /* Selected index */
    int padpos; /* Scroll position */
    int entries; /* Max index of selectable items */
    int (*draw)(PANEL *); /* Pointer to panel draw function */
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
 * Wait for user input.
 * This function manages all user input in all panel types and
 * redraws the panel using its own draw function
 * 
 * @param panel the topmost panel
 */
void wait_for_input(PANEL *panel);

/**
 * Toggle color mode on and off
 * @param on Pass 0 to turn all black&white
 */
void toggle_color(int on);

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

/****************************************************************************
 * Draw functions for each type of panel 
 * A pointer to one of this function is stored into each panel  information 
 * structure @panel_info. Draw function is invoked during the user input loop 
 * or async when the exec thread request UI update.
 ****************************************************************************/
/**
 * Main Calls panel draw function
 *
 * This panel contains a list of the parsed calls, one line per found
 * Call-ID.
 *
 * At the top of the panel a brief bar will show the current running 
 * status: pcap file (Offline) or ngrep parsing (Online).
 *
 */
int draw_main_panel(PANEL *panel);

/**
 * Main Help panel draw function
 * 
 * This panel contains information about common keybindings, but it's a 
 * bit deprecated. XXX
 *
 */
int draw_main_help_panel(PANEL *panel);

/**
 * Call Details panel
 *
 * This panel show the call-flow of active_call. The panel will be divided
 * into two pads: one containing the call flow and one showing the payload
 * of the selected SIP message.
 *
 * XXX By now, scrolling is only available in Call flow pad, but it would be
 * great to scroll SIP message pad using and mod key (like SHIFT)
 *
 */
int draw_details_panel(PANEL *panel);

/**
 * Call Details Extended panel
 *
 * This panel show the call-flow of active_call and the second leg of the call
 * if any is found in calls list (@see get_ex_call function)
 *
 * The panel will be divided
 * into two pads: one containing the call flow and one showing the payload
 * of the selected SIP message.
 *
 * XXX By now, scrolling is only available in Call flow pad, but it would be
 * great to scroll SIP message pad using and mod key (like SHIFT)
 *
 */
int draw_details_ex_panel(PANEL *panel);

/**
 * Show the SIPs messages in a raw full screen panel
 *
 * This panel was designed with the intention of making the SIP Messages easier
 * to copy :)
 */
int draw_raw_panel(PANEL *panel);

#endif 

