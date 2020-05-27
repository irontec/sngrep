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
 * @file scrollbar.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in scrollbar.h
 */

#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include "tui/tui.h"
#include "tui/widgets/scrollbar.h"
#include "tui/widgets/scrollable.h"

typedef struct
{
    //! Scrollable content
    SngWidget *content;
    //! Vertical scroll widget
    SngWidget *vscroll;
    //! Horizontal scroll widget
    SngWidget *hscroll;
    SngBoxPadding padding;
} SngScrollablePrivate;

// Class definition
G_DEFINE_TYPE_WITH_PRIVATE(SngScrollable, sng_scrollable, SNG_TYPE_CONTAINER)

SngScrollbar *
sng_scrollable_get_vscroll(SngScrollable *scrollable)
{
    SngScrollablePrivate *priv = sng_scrollable_get_instance_private(scrollable);
    return SNG_SCROLLBAR(priv->vscroll);
}

SngScrollbar *
sng_scrollable_get_hscroll(SngScrollable *scrollable)
{
    SngScrollablePrivate *priv = sng_scrollable_get_instance_private(scrollable);
    return SNG_SCROLLBAR(priv->hscroll);
}

SngWidget *
sng_scrollable_get_content(SngScrollable *scrollable)
{
    SngScrollablePrivate *priv = sng_scrollable_get_instance_private(scrollable);
    return priv->content;
}

void
sng_scrollable_set_padding(SngScrollable *scrollable, gint top, gint bottom, gint left, gint right)
{
    SngScrollablePrivate *priv = sng_scrollable_get_instance_private(scrollable);
    priv->padding.top = top;
    priv->padding.bottom = bottom;
    priv->padding.left = left;
    priv->padding.right = right;
}

static void
sng_scrollable_size_request(SngWidget *widget)
{
    SngScrollablePrivate *priv = sng_scrollable_get_instance_private(SNG_SCROLLABLE(widget));

    // Determine internal scrollable area size
    // It must be at least of the size of the original widget
    sng_widget_set_size(
        priv->content,
        MAX(
            sng_widget_get_preferred_width(widget),
            sng_widget_get_width(widget)
        ),
        MAX(
            sng_widget_get_preferred_height(widget),
            sng_widget_get_height(widget)
        )
    );

    // Avoid both scrollbars collision
    gint collision_padding = 0;
    if (sng_widget_is_visible(priv->vscroll) && sng_widget_is_visible(priv->hscroll)) {
        collision_padding++;
    }

    // Vertical scrolling configuration
    sng_widget_set_size(
        priv->vscroll,
        1,
        sng_widget_get_height(widget) - priv->padding.top - priv->padding.bottom - collision_padding
    );
    sng_widget_set_position(
        priv->vscroll,
        sng_widget_get_xpos(widget),
        sng_widget_get_ypos(widget) + priv->padding.top
    );
    sng_widget_set_visible(
        priv->vscroll,
        sng_widget_get_height(priv->content) > sng_widget_get_height(widget)
    );
    sng_scrollbar_set_max_position(
        SNG_SCROLLBAR(priv->vscroll),
        sng_widget_get_height(priv->content) - sng_widget_get_height(widget) - collision_padding
    );

    // Horizontal scrolling configuration
    sng_widget_set_size(
        priv->hscroll,
        sng_widget_get_width(widget) - priv->padding.left - priv->padding.right - collision_padding,
        1
    );
    sng_widget_set_position(
        priv->hscroll,
        sng_widget_get_xpos(widget) + priv->padding.left + collision_padding,
        sng_widget_get_ypos(widget) + sng_widget_get_height(widget) - 1
    );
    sng_widget_set_visible(
        priv->hscroll,
        sng_widget_get_width(priv->content) > sng_widget_get_width(widget)
    );
    sng_scrollbar_set_max_position(
        SNG_SCROLLBAR(priv->hscroll),
        sng_widget_get_width(priv->content) - sng_widget_get_width(widget) - collision_padding
    );

    // Chain-up parent size request function
    SNG_WIDGET_CLASS(sng_scrollable_parent_class)->size_request(widget);
}

static void
sng_scrollable_realize(SngWidget *widget)
{
    SngScrollablePrivate *priv = sng_scrollable_get_instance_private(SNG_SCROLLABLE(widget));

    // Realize internal widget
    sng_widget_realize(priv->content);

    // Chain-up parent realize function
    SNG_WIDGET_CLASS(sng_scrollable_parent_class)->realize(widget);
}

static void
sng_scrollable_constructed(GObject *object)
{
    SngScrollable *scrollable = SNG_SCROLLABLE(object);
    SngScrollablePrivate *priv = sng_scrollable_get_instance_private(scrollable);

    // Initialize scrollbars containers
    priv->vscroll = sng_scrollbar_new(SNG_ORIENTATION_VERTICAL);
    priv->hscroll = sng_scrollbar_new(SNG_ORIENTATION_HORIZONTAL);

    // Create the scrollable area (may be greater than widget size)
    priv->content = sng_widget_new();

    // Add them to the widget
    sng_container_add(SNG_CONTAINER(object), priv->vscroll);
    sng_container_add(SNG_CONTAINER(object), priv->hscroll);

    // Chain-up parent constructed function
    G_OBJECT_CLASS(sng_scrollable_parent_class)->constructed(object);
}

static void
sng_scrollable_class_init(SngScrollableClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = sng_scrollable_constructed;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->size_request = sng_scrollable_size_request;
    widget_class->realize = sng_scrollable_realize;
}

static void
sng_scrollable_init(SngScrollable *scrollable)
{
}
