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

#include <stdbool.h>
#include "ui_manager.h"
#include "group.h"
#include "scrollbar.h"

//! Sorter declaration of struct call_flow_info
typedef struct call_flow_info call_flow_info_t;
//! Sorter declaration of struct call_flow_column
typedef struct call_flow_column call_flow_column_t;
//! Sorter declaration of struct call_flow_arrow
typedef struct call_flow_arrow call_flow_arrow_t;

/**
 * @brief Call flow arrow types
 */
enum call_flow_arrow_type {
    CF_ARROW_SIP,
    CF_ARROW_RTP,
    CF_ARROW_RTCP,
};

/**
 * @brief Call flow arrow directions
 */
enum call_flow_arrow_dir {
    CF_ARROW_RIGHT = 0,
    CF_ARROW_LEFT,
    CF_ARROW_SPIRAL
};

/**
 * @brief Call Flow arrow information
 */
struct call_flow_arrow {
    //! Type of arrow @see call_flow_arrow_type
    int type;
    //! Item owner of this arrow
    void *item;
    //! Stream packet count for this arrow
    int rtp_count;
    //! Stream arrow position
    int rtp_ind_pos;
    //! Number of screen lines this arrow uses
    int height;
    //! Line of flow window this line starts
    int line;
    //! Arrow direction
    enum call_flow_arrow_dir dir;
    //! Source column for this arrow
    call_flow_column_t *scolumn;
    //! Destination column for this arrow
    call_flow_column_t *dcolumn;
};

/**
 * @brief Structure to hold one column information
 *
 * One column has one address:port for packets source or destination matching
 * and a couple of Call-Ids. Limiting the number of Call-Ids each column have
 * we avoid having one column with lots of begining or ending arrows of
 * different dialogs.
 *
 */
struct call_flow_column {
    //! Address header for this column
    address_t addr;
    //! Alias for the given address
    char alias[MAX_SETTING_LEN];
    //! Call Ids
    vector_t *callids;
    //! Column position (starting with zero) // FIXME array position?
    int colpos;
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
    //! List of arrows (call_flow_arrow_t *)
    vector_t *arrows;
    //! List of displayed arrows
    vector_t *darrows;
    //! First displayed arrow in the list
    int first_arrow;
    //! Current arrow index where the cursor is
    int cur_arrow;
    //! Selected arrow to compare
    int selected;
    //! Current line for scrolling
    scrollbar_t scroll;
    //! List of columns in the panel
    vector_t *columns;
    //! Max callids per column
    int maxcallids;
    //! Print timestamp next to the arrow
    bool arrowtime;
};

/**
 * @brief Create Call Flow extended panel
 *
 * This function will allocate the ncurses pointer and draw the static
 * stuff of the screen (which usually won't be redrawn)
 * It will also create an information structure of the panel status and
 * store it in the panel's userpointer
 *
 * @param ui UI structure pointer
 */
void
call_flow_create(ui_t *ui);

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param ui UI structure pointer
 */
void
call_flow_destroy(ui_t *ui);

/**
 * @brief Get custom information of given panel
 *
 * Return ncurses users pointer of the given panel into panel's
 * information structure pointer.
 *
 * @param ui UI structure pointer
 * @return a pointer to info structure of given panel
 */
call_flow_info_t *
call_flow_info(ui_t *ui);

/**
 * @brief Determine if the screen requires redrawn
 *
 * This will query the interface if it requires to be redraw again.
 *
 * @param ui UI structure pointer
 * @return true if the panel requires redraw, false otherwise
 */
bool
call_flow_redraw(ui_t *ui);

/**
 * @brief Draw the Call flow extended panel
 *
 * This function will drawn the panel into the screen based on its stored
 * status
 *
 * @param ui UI structure pointer
 * @return 0 if the panel has been drawn, -1 otherwise
 */
int
call_flow_draw(ui_t *ui);

/**
 * @brief Draw the footer of the panel with keybindings info
 *
 * @param ui UI structure pointer
 */
void
call_flow_draw_footer(ui_t *ui);

/**
 * @brief Draw the visible columns in panel window
 *
 * @param ui UI structure pointer
 */
int
call_flow_draw_columns(ui_t *ui);

/**
 * @brief Draw arrows in the visible part of the panel
 *
 * @param ui UI structure pointer
 */
void
call_flow_draw_arrows(ui_t *ui);

/**
 * @brief Draw panel preview of current arrow
 *
 * If user request to not draw preview panel, this function does nothing.
 *
 * @param ui UI structure pointer
 */
void
call_flow_draw_preview(ui_t *ui);

/**
 * @brief Draw a single arrow in arrow flow
 *
 * This function draws an arrow of any type in the given line of the flow.
 *
 * @param ui UI Structure pointer
 * @param arrow Arrow structure pointer of any type
 * @param line Line of the flow window to draw this arrow
 * @return the number of screen lines this arrow uses on screen
 */
int
call_flow_draw_arrow(ui_t *ui, call_flow_arrow_t *arrow, int line);

/**
 * @brief Draw the message arrow in the given line
 *
 * Draw the given message arrow in the given line.
 * This function will calculate origin and destination coordinates
 * base on message information. Each message use multiple lines
 * depending on the display mode of call flow
 *
 * @param ui UI structure pointer
 * @param arrow Call flow arrow with message to be drawn
 * @param cline Window line to draw the message
 * @return the number of screen lines this arrow uses on screen
 */
int
call_flow_draw_message(ui_t *ui, call_flow_arrow_t *arrow, int cline);

/**
 * @brief Draw the stream data in the given line
 *
 * Draw the given arrow of type stream in the given line.
 *
 * @param ui UI structure pointer
 * @param arrow Call flow arrow of stream to be drawn
 * @param cline Window line to draw the message
 * @return the number of screen lines this arrow uses on screen
 */
int
call_flow_draw_rtp_stream(ui_t *ui, call_flow_arrow_t *arrow, int cline);

/**
 * @brief Create a new arrow of given type
 *
 * Allocate memory for a new arrow of the given type and associate the
 * item pointer. If the arrow already exists in the ui arrows vector
 * this function will return that arrow instead of creating a new one.
 *
 * This function WON'T add the arrow to any ui vector.
 *
 * @param ui UI structure pointer
 * @param item Item pointer to associate to the arrow
 * @param type Type of arrow as defined in enum @call_flow_arrow_type
 * @return an arrow pointer or NULL in case of error
 */
call_flow_arrow_t *
call_flow_arrow_create(ui_t *ui, void *item, int type);

/**
 * @brief Get how many lines of screen an arrow will use
 *
 * Depending on the arrow tipe and panel display mode lines can
 * take more than two lines. This function will calculate how many
 * lines the arrow will use.
 *
 * @param ui UI structure pointer
 * @param arrow Arrow structure to calculate height
 * @return height the arrow will have
 */
int
call_flow_arrow_height(ui_t *ui, const call_flow_arrow_t *arrow);

/**
 * @brief Return the arrow of a SIP msg or RTP stream
 *
 * This function will try to find an existing arrow with a
 * message or stream equals to the giving pointer.
 *
 * @param ui UI structure pointer
 * @param data Data to search in the arrow structure
 * @return a pointer to the found arrow or NULL
 */
call_flow_arrow_t *
call_flow_arrow_find(ui_t *ui, const void *data);

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
call_flow_arrow_message(const call_flow_arrow_t *arrow);

/**
 * @brief Draw raw panel with message payload
 *
 * Draw the given message payload into the raw window.
 *
 * @param ui UI structure pointer
 * @param msg Message data to draw
 * @return 0 in all cases
 */
int
call_flow_draw_raw(ui_t *ui, sip_msg_t *msg);

/**
 * @brief Draw raw panel with RTCP data
 *
 * Draw the given stream data into the raw window.
 *
 * @param ui UI structure pointer
 * @param rtcp stream containing the RTCP conection data
 * @return 0 in all cases
 */
int
call_flow_draw_raw_rtcp(ui_t *ui, rtp_stream_t *rtcp);

/**
 * @brief Handle Call flow extended key strokes
 *
 * This function will manage the custom keybindings of the panel.
 * This function return one of the values defined in @key_handler_ret
 *
 * @param ui UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
int
call_flow_handle_key(ui_t *ui, int key);

/**
 * @brief Request the panel to show its help
 *
 * This function will request to panel to show its help (if any) by
 * invoking its help function.
 *
 * @param ui UI structure pointer
 * @return 0 if the screen has help, -1 otherwise
 */
int
call_flow_help(ui_t *ui);

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
 * @param ui UI structure pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 */
void
call_flow_column_add(ui_t *ui, const char *callid, address_t addr);

/**
 * @brief Get a flow column data
 *
 * @param ui UI structure pointer
 * @param callid Call-Id header of SIP payload
 * @param addr Address:port string
 * @return column structure pointer or NULL if not found
 */
call_flow_column_t *
call_flow_column_get(ui_t *ui, const char *callid, address_t address);

/**
 * @brief Move selected cursor to given arrow
 *
 * This function will move the cursor to given arrow, taking into account
 * selected line and scrolling position.
 *
 * @param ui UI structure pointer
 * @param arrowindex Position to move the cursor
 */
void
call_flow_move(ui_t *ui, int arrowindex);

/*
 * @brief Return selected flow arrow
 *
 * User can select an arrow to compare times or payload with another
 * arrow. Don't confuse this with the current arrow (where the cursor is)
 *
 * @param ui UI Structure pointer
 * @return user selected arrow
 */
call_flow_arrow_t *
call_flow_arrow_selected(ui_t *ui);

/**
 * @brief Return timestamp for given arrow
 *
 * This function is a wrapper to return arrow timestamp no matter what
 * type of arrow is passed. Arrows of different types store their times
 * in different locations.
 *
 * If pointer is invalid of arrow type doesn't match anything known, the
 * timestamp returned structure will be zero'd
 *
 * @param arrow Arrow structure pointer
 * @return timestamp for given arrow
 */
struct timeval
call_flow_arrow_time(call_flow_arrow_t *arrow);

/**
 * @brief Sort arrows by timestamp
 *
 * This function acts as sorter for arrows vector. Each time a new arrow
 * is appended, it's sorted based on its timestamp.
 *
 * @param vector Arrows vector pointer
 * @param item Call Flow arrow structure pointer
 */
void
call_flow_arrow_sorter(vector_t *vector, void *item);

/**
 * @brief Filter displayed arrows based on configuration
 */
int
call_flow_arrow_filter(void *item);

#endif /* __SNGREP_UI_CALL_FLOW_H */
