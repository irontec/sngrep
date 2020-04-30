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
 * @file flow_arrow.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_FLOW_ARROW_H__
#define __SNGREP_FLOW_ARROW_H__

#include <glib.h>
#include <glib-object.h>
#include "tui/widgets/flow_column.h"
#include "tui/widgets/widget.h"

G_BEGIN_DECLS

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
} SngFlowArrowDir;

#define SNG_TYPE_FLOW_ARROW sng_flow_arrow_get_type()
G_DECLARE_DERIVABLE_TYPE(SngFlowArrow, sng_flow_arrow, SNG, FLOW_ARROW, SngWidget)

struct _SngFlowArrowClass
{
    //! Parent class
    SngWidgetClass parent;
    //! Get arrow time
    guint64 (*get_time)(SngFlowArrow *arrow);
    //! Get arrow details
    const gchar *(*get_detail)(SngFlowArrow *arrow);
};

SngFlowArrowDir
sng_flow_arrow_get_direction(SngFlowArrow *arrow);

void
sng_flow_arrow_set_direction(SngFlowArrow *arrow, SngFlowArrowDir dir);

SngFlowColumn *
sng_flow_arrow_get_src_column(SngFlowArrow *arrow);

void
sng_flow_arrow_set_src_column(SngFlowArrow *arrow, SngFlowColumn *column);

SngFlowColumn *
sng_flow_arrow_get_dst_column(SngFlowArrow *arrow);

void
sng_flow_arrow_set_dst_column(SngFlowArrow *arrow, SngFlowColumn *column);

gboolean
sng_flow_arrow_is_selected(SngFlowArrow *arrow);

void
sng_flow_arrow_set_selected(SngFlowArrow *arrow, gboolean selected);

gint64
sng_flow_arrow_get_time(SngFlowArrow *arrow);

#endif    /* __SNGREP_FLOW_ARROW_H__ */
