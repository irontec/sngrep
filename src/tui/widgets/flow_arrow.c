/**************************************************************************
 **
 ** sngrep - SIP Messages flow arrow
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
 * @file flow_arrow.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include "tui/widgets/flow_arrow.h"

/**
 * @brief Call Flow arrow information
 */
typedef struct
{
    //! Parent object attributes
    SngWidget parent;
    //! Arrow direction
    SngFlowArrowDir dir;
    //! Source column for this arrow
    SngFlowColumn *scolumn;
    //! Destination column for this arrow
    SngFlowColumn *dcolumn;
    //! Arrow selected flag
    gboolean selected;
} SngFlowArrowPrivate;

// Class Definition
G_DEFINE_TYPE_WITH_PRIVATE(SngFlowArrow, sng_flow_arrow, SNG_TYPE_WIDGET)

SngFlowArrowDir
sng_flow_arrow_get_direction(SngFlowArrow *arrow)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(arrow);
    return priv->dir;
}

void
sng_flow_arrow_set_direction(SngFlowArrow *arrow, SngFlowArrowDir dir)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(arrow);
    priv->dir = dir;
}

SngFlowColumn *
sng_flow_arrow_get_src_column(SngFlowArrow *arrow)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(arrow);
    return priv->scolumn;
}

void
sng_flow_arrow_set_src_column(SngFlowArrow *arrow, SngFlowColumn *column)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(arrow);
    priv->scolumn = column;
}

SngFlowColumn *
sng_flow_arrow_get_dst_column(SngFlowArrow *arrow)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(arrow);
    return priv->dcolumn;
}

void
sng_flow_arrow_set_dst_column(SngFlowArrow *arrow, SngFlowColumn *column)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(arrow);
    priv->dcolumn = column;
}

gboolean
sng_flow_arrow_is_selected(SngFlowArrow *arrow)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(arrow);
    return priv->selected;
}

void
sng_flow_arrow_set_selected(SngFlowArrow *arrow, gboolean selected)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(arrow);
    priv->selected = selected;
}

gint64
sng_flow_arrow_get_time(SngFlowArrow *arrow)
{
    g_return_val_if_fail(arrow != NULL, 0);

    SngFlowArrowClass *klass = SNG_FLOW_ARROW_GET_CLASS(arrow);
    if (klass->get_time != NULL) {
        return klass->get_time(arrow);
    }

    return 0;
}

static void
sng_flow_arrow_size_request(SngWidget *widget)
{
    SngFlowColumn *src_column = sng_flow_arrow_get_src_column(SNG_FLOW_ARROW(widget));
    SngFlowColumn *dst_column = sng_flow_arrow_get_dst_column(SNG_FLOW_ARROW(widget));
    g_return_if_fail(src_column != NULL);
    g_return_if_fail(dst_column != NULL);

    // Calculate Arrow X position TODO: Use column vertical line instead of X pos
    sng_widget_set_position(
        widget,
        MIN(
            sng_widget_get_xpos(SNG_WIDGET(src_column)),
            sng_widget_get_xpos(SNG_WIDGET(dst_column))
        ) + 21,
        sng_widget_get_ypos(widget)
    );

    // Calculate Arrow Width
    sng_widget_set_width(
        widget,
        sng_widget_get_preferred_width(widget)
    );
}

static gint
sng_flow_arrow_get_preferred_width(SngWidget *widget)
{
    SngFlowArrowPrivate *priv = sng_flow_arrow_get_instance_private(SNG_FLOW_ARROW(widget));
    g_return_val_if_fail(priv != NULL, 0);
    g_return_val_if_fail(priv->scolumn != NULL, 0);
    g_return_val_if_fail(priv->dcolumn != NULL, 0);

    gint src_xpos = sng_widget_get_xpos(SNG_WIDGET(priv->scolumn));
    gint dst_xpos = sng_widget_get_xpos(SNG_WIDGET(priv->dcolumn));

    if (src_xpos == dst_xpos)
        return 4;
    else
        return MAX(src_xpos - dst_xpos, dst_xpos - src_xpos) - 3;

}

static void
sng_flow_arrow_class_init(SngFlowArrowClass *klass)
{
    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->size_request = sng_flow_arrow_size_request;
    widget_class->preferred_width = sng_flow_arrow_get_preferred_width;
}

static void
sng_flow_arrow_init(G_GNUC_UNUSED SngFlowArrow *flow_arrow)
{
}

