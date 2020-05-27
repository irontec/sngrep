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
 * @file flow_viewer.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_FLOW_VIEWER_H__
#define __SNGREP_FLOW_VIEWER_H__

#include <glib.h>
#include <glib-object.h>
#include "storage/address.h"
#include "storage/group.h"
#include "tui/widgets/flow_arrow.h"
#include "tui/widgets/scrollable.h"
#include "tui/widgets/widget.h"

G_BEGIN_DECLS

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

#define SNG_TYPE_FLOW_VIEWER sng_flow_viewer_get_type()
G_DECLARE_FINAL_TYPE(SngFlowViewer, sng_flow_viewer, SNG, FLOW_VIWER, SngScrollable)

struct _SngFlowViewer
{
    //! Parent object attributes
    SngScrollable parent;
    //! Container for flow columns
    SngWidget *box_columns;
    //! Container for flow arrows
    SngWidget *box_arrows;
    //! Group of calls displayed on the panel
    CallGroup *group;
    //! Current arrow index where the cursor is
    SngWidget *current;
    //! Selected arrow to compare
    SngWidget *selected;
    //! Print timestamp next to the arrow
    gboolean arrowtime;
};

SngWidget *
sng_flow_viewer_new();

void
sng_flow_viewer_set_group(SngFlowViewer *flow_viewer, CallGroup *group);

CallGroup *
sng_flow_viewer_get_group(SngFlowViewer *flow_viewer);

SngFlowArrow *
sng_flow_viewer_get_current(SngFlowViewer *flow_viewer);

gint
sng_flow_viewer_columns_width(SngFlowViewer *flow_viewer);

#endif    /* __SNGREP_FLOW_VIEWER_H__ */
