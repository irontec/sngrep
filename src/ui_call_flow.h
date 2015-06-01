/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
 * @file ui_call_flow.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage Call Flow Extended screen
 *
 * This file contains the functions and structures to manage the call flow
 * extended screen.
 *
 */
#ifndef __UI_CALL_FLOW_EX_H
#define __UI_CALL_FLOW_EX_H

#include "config.h"
#include "ui_manager.h"
#include "group.h"

//! Sorter declaration of struct call_flow_info
typedef struct call_flow_info call_flow_info_t;
//! Sorter declaration of struct call_flow_column
typedef struct call_flow_column call_flow_column_t;

struct call_flow_column {
    const char *addr;
    const char *callid;
    const char *callid2;
    int colpos;
    call_flow_column_t *next;
};

/**
 * @brief Call flow Extended status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct call_flow_info {
    WINDOW *raw_win;
    WINDOW *flow_win;
    sip_call_group_t *group;
    sip_msg_t *first_msg;
    sip_msg_t *cur_msg;
    sip_msg_t *selected;
    int raw_width;
    int cur_line;
    int show_rtp;
    call_flow_column_t *columns;
};

/**
 * @brief Create Call Flow extended panel
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 *
 * @return the allocated ncurses panel
 */
PANEL *
call_flow_create();

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @return panel Ncurses panel pointer
 */
void
call_flow_destroy(PANEL *panel);

/**
 * @brief Draw the Call flow extended panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
int
call_flow_draw(PANEL *panel);

/**
 * @brief Draw the footer of the panel with keybindings info
 *
 * @param panel Ncurses panel pointer
 */
void
call_flow_draw_footer(PANEL *panel);

/**
 * @brief Draw the visible columns in panel window
 *
 * @param panel Ncurses panel pointer
 */
int
call_flow_draw_columns(PANEL *panel);

/**
 * @brief Draw the message arrow in the given line
 *
 * Draw the given message arrow in the given line.
 * This function will calculate origin and destriny coordinates
 * base on message information. Each message use two lines
 *
 * @param panel Ncurses panel pointer
 * @param msg Message data to draw
 * @param cline Window line to draw the message
 * @return 0 if arrow is drawn, 1 otherwise
 */
int
call_flow_draw_message(PANEL *panel, sip_msg_t *msg, int cline);

/**
 * @brief Draw raw panel with message payload
 *
 * Draw the given message payload into the raw window.
 *
 * @param panel Ncurses panel pointer
 * @param msg Message data to draw
 * @return 0 in all cases
 */
int
call_flow_draw_raw(PANEL *panel, sip_msg_t *msg);

/**
 * @brief Handle Call flow extended key strokes
 *
 * This function will manage the custom keybindings of the panel. If this
 * function returns -1, the ui manager will check if the pressed key
 * is one of the common ones (like toggle colors and so).
 *
 * @param panel Ncurses panel pointer
 * @param key Pressed keycode
 * @return 0 if the function can handle the key, key otherwise
 */
int
call_flow_handle_key(PANEL *panel, int key);

/**
 * @brief Request the panel to show its help
 *
 * This function will request to panel to show its help (if any) by
 * invoking its help function.
 *
 * @param panel Ncurses panel pointer
 * @return 0 if the screen has help, -1 otherwise
 */
int
call_flow_help(PANEL *panel);

/**
 * @brief Set the group call of the panel
 *
 * This function will access the panel information and will set the
 * group call pointer to the processed calls.
 *
 * @param group Call group pointer to be set in the internal info struct
 */
int
call_flow_set_group(sip_call_group_t *group);

/**
 * @brief Add a new column (if required)
 *
 * Check if the given callid and address has already a column.
 * If not, create a new call for that callid/address
 * Each column has one address and two callids (unless split mode
 * is disabled)
 *
 * @param panel Ncurses panel pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 */
void
call_flow_column_add(PANEL *panel, const char *callid, const char *addr);

/**
 * @brief Get a flow column data
 *
 * @param panel Ncurses panel pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 * @return column structure pointer or NULL if not found
 */
call_flow_column_t *
call_flow_column_get(PANEL *panel, const char *callid, const char *addr);

#endif
