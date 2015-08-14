/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2015 Irontec SL. All rights reserved.
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
 * @brief Functions to manage Call Flow screen
 *
 * This file contains the functions and structures to manage the call flow
 * screen. Call flow screen display a ladder of arrows corresponding to the
 * SIP messages and RTP streams of the selected dialogs.
 *
 * Some basic ascii art of this panel.
 *
 * +--------------------------------------------------------+
 * |                     Title                              |
 * |   addr1  addr2  addr3  addr4 | Selected Raw Message    |
 * |   -----  -----  -----  ----- | preview                 |
 * | Tmst|      |      |      |   |                         |
 * | Tmst|----->|      |      |   |                         |
 * | Tmst|      |----->|      |   |                         |
 * | Tmst|      |<-----|      |   |                         |
 * | Tmst|      |      |----->|   |                         |
 * | Tmst|<-----|      |      |   |                         |
 * | Tmst|      |----->|      |   |                         |
 * | Tmst|      |<-----|      |   |                         |
 * | Tmst|      |------------>|   |                         |
 * | Tmst|      |<------------|   |                         |
 * |     |      |      |      |   |                         |
 * |     |      |      |      |   |                         |
 * |     |      |      |      |   |                         |
 * | Useful hotkeys                                         |
 * +--------------------------------------------------------+
 *
 */
#ifndef __SNGREP_UI_CALL_FLOW_H
#define __SNGREP_UI_CALL_FLOW_H

#include "ui_manager.h"
#include "group.h"

//! Sorter declaration of struct call_flow_info
typedef struct call_flow_info call_flow_info_t;
//! Sorter declaration of struct call_flow_column
typedef struct call_flow_column call_flow_column_t;
//! Sorter declaration of struct call_flow_arrow
typedef struct call_flow_arrow call_flow_arrow_t;

/**
 * @brief Structure to hold one column information
 */
struct call_flow_column {
    const char *addr;
    const char *callid;
    const char *callid2;
    int colpos;
    call_flow_column_t *next;
};

/**
 * @brief Call flow arrow types
 */
enum call_flow_arrow_type {
    CF_ARROW_SIP,
    CF_ARROW_RTP,
};

/**
 * @brief Call Flow arrow information
 */
struct call_flow_arrow {
    //! Type of arrow @see call_flow_arrow_type
    int type;
    //! Msg pointer for SIP type arrows
    sip_msg_t *msg;
    //! Stream information for RTP type arrows
    rtp_stream_t *stream;
    //! Stream packet count for this arrow
    int rtp_count;
    //! Stream arrow position
    int rtp_ind_pos;
    //! Number of screen lines this arrow uses
    int height;
    //! Line of flow window this line starts
    int line;
    //! Pointer to the next arrow
    call_flow_arrow_t *next;
};

/**
 * @brief Call flow Extended status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct call_flow_info {
    //! Window to display SIP payload
    WINDOW *raw_win;
    //! Window to diplay arrows
    WINDOW *flow_win;
    //! Group of calls displayed on the panel
    sip_call_group_t *group;
    //! Last processed message
    sip_msg_t *last_msg;
    //! List of arrows
    call_flow_arrow_t *arrows;
    //! First printed arrow of the panel
    call_flow_arrow_t *first_arrow;
    //! Current arrow where the cursor is
    call_flow_arrow_t *cur_arrow;
    //! Selected arrow to compare
    call_flow_arrow_t *selected;
    //! Width of raw_win
    int raw_width;
    //! Current line for scrolling
    int cur_line;
    //! List of columns in the panel
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
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param panel Ncurses panel pointer
 * @return a pointer to info structure of given panel
 */
call_flow_info_t *
call_flow_info(PANEL *panel);

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
 * This function will calculate origin and destination coordinates
 * base on message information. Each message use multiple lines
 * depending on the display mode of call flow
 *
 * @param panel Ncurses panel pointer
 * @param arrow Call flow arrow to be drawn
 * @param cline Window line to draw the message
 * @return the arrow passed as parameter
 */
call_flow_arrow_t *
call_flow_draw_message(PANEL *panel, call_flow_arrow_t *arrow, int cline);

/**
 * @brief Draw the stream data in the given line
 *
 * Draw the given arrow of type stream in the given line.
 *
 * @param panel Ncurses panel pointer
 * @param arrow Call flow arrow to be drawn
 * @param cline Window line to draw the message
 * @return the arrow passed as parameter
 */
call_flow_arrow_t *
call_flow_draw_stream(PANEL *panel, call_flow_arrow_t *arrow, int cline);

/**
 * @brief Get the next chronological arrow
 *
 * Give the next arrow taking into account Call list display mode.
 *
 * @param panel Ncurses panel pointer
 * @param cur Current arrow to use as reference
 * @return next chronological arrow
 */
call_flow_arrow_t *
call_flow_next_arrow(PANEL *panel, const call_flow_arrow_t *cur);

/**
 * @brief Get the previous chronological arrow
 *
 * Give the previous arrow taking into account Call list display mode.
 *
 * @param panel Ncurses panel pointer
 * @param cur Current arrow to use as reference
 * @return previous chronological arrow
 */
call_flow_arrow_t *
call_flow_prev_arrow(PANEL *panel, const call_flow_arrow_t *cur);

/**
 * @brief Get how many lines of screen an arrow will use
 *
 * Depending on the arrow tipe and panel display mode lines can
 * take more than two lines. This function will calculate how many
 * lines the arrow will use.
 *
 * @param panel Ncurses panel pointer
 * @param arrow Arrow structure to calculate height
 * @return height the arrow will have
 */
int
call_flow_arrow_height(PANEL *panel, const call_flow_arrow_t *arrow);

/**
 * @brief Return the arrow of a SIP msg or RTP stream
 *
 * This function will try to find an existing arrow with a
 * message or stream equals to the giving pointer.
 *
 * @param panel Ncurses panel pointer
 * @param data Data to search in the arrow structure
 * @return a pointer to the found arrow or NULL
 */
call_flow_arrow_t *
call_flow_arrow_find(PANEL *panel, const void *data);

/**
 * @brief Return the SIP message associated with the arrow
 *
 * Return the SIP message. If the arrow is of type SIP msg, it will
 * return the message itself. If the arrow is of type RTP stream,
 * it will return the SIP msg that setups the stream.
 *
 * @param arrow Call Flow Arrow pointer
 * @return associated SIP message with the arrow
 */
sip_msg_t *
call_flow_arrow_message(const  call_flow_arrow_t *arrow);

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

#endif /* __SNGREP_UI_CALL_FLOW_H */
