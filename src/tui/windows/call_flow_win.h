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
#ifndef __SNGREP_CALL_FLOW_WIN_H__
#define __SNGREP_CALL_FLOW_WIN_H__

#include <glib.h>
#include "tui/tui.h"
#include "tui/widgets/scrollbar.h"
#include "storage/group.h"

//! Configurable some day
#define CF_COLUMN_WIDTH 30

#define WINDOW_TYPE_CALL_FLOW call_flow_win_get_type()
G_DECLARE_FINAL_TYPE(CallFlowWindow, call_flow_win, TUI, CALL_FLOW, SngWindow)

/**
 * @brief Call flow arrow types
 */
typedef enum
{
    CF_ARROW_SIP,
    CF_ARROW_RTP,
    CF_ARROW_RTCP,
} CallFlowArrowType;

/**
 * @brief Call flow arrow directions
 */
typedef enum
{
    CF_ARROW_DIR_ANY = 0,
    CF_ARROW_DIR_RIGHT,
    CF_ARROW_DIR_LEFT,
    CF_ARROW_DIR_SPIRAL_RIGHT,
    CF_ARROW_DIR_SPIRAL_LEFT
} CallFlowArrowDir;

typedef enum
{
    SETTING_ARROW_HIGHLIGH_BOLD,
    SETTING_ARROW_HIGHLIGH_REVERSE,
    SETTING_ARROW_HIGHLIGH_REVERSEBOLD,
} SettingArrowHighlight;

typedef enum
{
    SETTING_SDP_OFF,
    SETTING_SDP_FIRST,
    SETTING_SDP_FULL,
    SETTING_SDP_COMPRESSED,
} SettingSdpMode;

/**
 * @brief Structure to hold one column information
 *
 * One column has one address:port for packets source or destination matching
 * and a couple of Call-Ids. Limiting the number of Call-Ids each column have
 * we avoid having one column with lots of beginning or ending arrows of
 * different dialogs.
 *
 */
typedef struct _CallFlowColumn CallFlowColumn;
struct _CallFlowColumn
{
    //! Address header for this column
    Address addr;
    //! Alias for the given address
    const gchar *alias;
    //! Twin column for externip setting
    CallFlowColumn *twin;
    //! Column position on the screen
    guint pos;
};

/**
 * @brief Call Flow arrow information
 */
typedef struct
{
    //! Type of arrow
    CallFlowArrowType type;
    //! Item owner of this arrow
    void *item;
    //! Stream packet count for this arrow
    gint rtp_count;
    //! Stream arrow position
    gint rtp_ind_pos;
    //! Number of screen lines this arrow uses
    gint height;
    //! Line of flow window this line starts
    gint line;
    //! Arrow direction
    CallFlowArrowDir dir;
    //! Source column for this arrow
    CallFlowColumn *scolumn;
    //! Destination column for this arrow
    CallFlowColumn *dcolumn;
} CallFlowArrow;

/**
 * @brief Call flow Extended status information
 *
 * This data stores the actual status of the panel. It's stored in the
 * PANEL user pointer.
 */
struct _CallFlowWindow
{
    //! Parent object attributes
    SngWindow parent;
    //! Window to display SIP payload
    WINDOW *raw_win;
    //! Window to display arrows
    WINDOW *colunms_win;
    //! Window to display arrows
    WINDOW *arrows_win;
    //! Window pad to store all arrow
    WINDOW *arrows_pad;
    //! Window pad to store all columns
    WINDOW *columns_pad;
    //! Group of calls displayed on the panel
    CallGroup *group;
    //! List of arrows (call_flow_arrow_t *)
    GPtrArray *arrows;
    //! List of displayed arrows
    GPtrArray *darrows;
    //! Current arrow index where the cursor is
    gint cur_idx;
    //! Selected arrow to compare
    gint selected;
    //! Vertical Flow window scrollbar
    Scrollbar vscroll;
    //! Vertical Flow window scrollbar
    Scrollbar hscroll;
    //! List of columns in the panel
    GList *columns;
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
SngWindow *
call_flow_win_new();

/**
 * @brief Destroy panel
 *
 * This function will hide the panel and free all allocated memory.
 *
 * @param ui UI structure pointer
 */
void
call_flow_win_free(SngWindow *window);

/**
 * @brief Set the group call of the panel
 *
 * This function will access the panel information and will set the
 * group call pointer to the processed calls.
 *
 * @param group Call group pointer to be set in the internal info struct
 */
void
call_flow_win_set_group(SngWindow *window, CallGroup *group);

G_END_DECLS

#endif /* __SNGREP_CALL_FLOW_WIN_H__ */
