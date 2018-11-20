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
#ifndef __SNGREP_CALL_FLOW_WIN_H
#define __SNGREP_CALL_FLOW_WIN_H

#include <glib.h>
#include "ncurses/manager.h"
#include "ncurses/scrollbar.h"
#include "group.h"

//! Sorter declaration of struct _CallFlowInfo
typedef struct _CallFlowWinInfo CallFlowWinInfo;
//! Sorter declaration of struct _CallFlowColumn
typedef struct _CallFlowColumn CallFlowColumn;
//! Sorter declaration of struct _CallFlowArrow
typedef struct _CallFlowArrow CallFlowArrow;

/**
 * @brief Call flow arrow types
 */
enum CallFlowArrowType {
    CF_ARROW_SIP,
    CF_ARROW_RTP,
    CF_ARROW_RTCP,
};

/**
 * @brief Call flow arrow directions
 */
enum CallFlowArrowDir {
    CF_ARROW_RIGHT = 0,
    CF_ARROW_LEFT,
    CF_ARROW_SPIRAL
};

/**
 * @brief Call Flow arrow information
 */
struct _CallFlowArrow {
    //! Type of arrow
    enum CallFlowArrowType type;
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
    enum CallFlowArrowDir dir;
    //! Source column for this arrow
    CallFlowColumn *scolumn;
    //! Destination column for this arrow
    CallFlowColumn *dcolumn;
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
struct _CallFlowColumn {
    //! Address header for this column
    Address addr;
    //! Alias for the given address
    gchar *alias;
    //! Call Ids
    GList *callids;
};

/**
 * @brief Call flow Extended status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct _CallFlowWinInfo {
    //! Window to display SIP payload
    WINDOW *raw_win;
    //! Window to diplay arrows
    WINDOW *flow_win;
    //! Group of calls displayed on the panel
    CallGroup *group;
    //! List of arrows (call_flow_arrow_t *)
    GPtrArray *arrows;
    //! List of displayed arrows
    GPtrArray *darrows;
    //! First displayed arrow in the list
    guint first_idx;
    //! Current arrow index where the cursor is
    guint cur_idx;
    //! Selected arrow to compare
    gint selected;
    //! Current line for scrolling
    Scrollbar scroll;
    //! List of columns in the panel
    GPtrArray *columns;
    //! Max callids per column
    guint maxcallids;
    //! Print timestamp next to the arrow
    gboolean arrowtime;
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
Window *
call_flow_win_new();

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param ui UI structure pointer
 */
void
call_flow_win_free(Window *window);

/**
 * @brief Set the group call of the panel
 *
 * This function will access the panel information and will set the
 * group call pointer to the processed calls.
 *
 * @param group Call group pointer to be set in the internal info struct
 */
void
call_flow_win_set_group(Window *window, CallGroup *group);

#endif /* __SNGREP_CALL_FLOW_WIN_H */
