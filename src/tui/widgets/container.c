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
 * @file container.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 */
#include "config.h"
#include "tui/widgets/container.h"
#include "glib-extra/gnode.h"

typedef struct
{
    //! Position to search
    gint x, y;
    //! Widget pointer if found, NULL if not found
    Widget *found;
} ContainerFindData;

typedef struct
{
    GList *children;
} ContainerPrivate;

// Class definition
G_DEFINE_TYPE_WITH_PRIVATE(Container, container, TUI_TYPE_WIDGET)

void
container_add(Container *container, Widget *child)
{
    ContainerClass *klass = TUI_CONTAINER_GET_CLASS(container);
    if (klass->add != NULL) {
        klass->add(container, child);
    }
}

void
container_remove(Container *container, Widget *child)
{
    ContainerClass *klass = TUI_CONTAINER_GET_CLASS(container);
    if (klass->remove != NULL) {
        klass->remove(container, child);
    }
}

void
container_foreach(Container *container, GFunc callback, gpointer user_data)
{
    ContainerPrivate *priv = container_get_instance_private(container);

    // Draw each of the container children
    g_list_foreach(
        priv->children,
        callback,
        user_data
    );
}

GList *
container_get_children(Container *container)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    return priv->children;
}

Widget *
container_get_child(Container *container, gint index)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    return g_list_nth_data(priv->children, index);
}

static gint
container_check_child_position(Widget *widget, gpointer data)
{
    ContainerFindData *find_data = data;

    if (widget_is_visible(widget) == FALSE) {
        return TRUE;
    }
    // Containers check their children first
    if (TUI_IS_CONTAINER(widget)) {
        if (find_data->found == NULL) {
            find_data->found = container_find_by_position(TUI_CONTAINER(widget), find_data->x, find_data->y);
        }
    }

    // Only check focusable widget position
    if (find_data->found == NULL && widget_can_focus(widget)) {
        // Then check if widget has been clicked
        if (find_data->x >= widget_get_xpos(widget)
            && find_data->x < widget_get_xpos(widget) + widget_get_width(widget)
            && find_data->y >= widget_get_ypos(widget)
            && find_data->y < widget_get_ypos(widget) + widget_get_height(widget)) {
            find_data->found = widget;
        }
    }

    return find_data->found == NULL;
}

Widget *
container_find_by_position(Container *container, gint x, gint y)
{
    ContainerFindData find_data = {
        .x = x,
        .y = y,
        .found = NULL,
    };

    ContainerPrivate *priv = container_get_instance_private(container);
    g_list_find_custom(
        priv->children,
        &find_data,
        (GCompareFunc) container_check_child_position
    );

    return find_data.found;
}

void
container_show_all(Container *container)
{
    // Show all children
    container_foreach(container, (GFunc) widget_show, NULL);
    // Show container itself
    widget_show(TUI_WIDGET(container));
}

static void
container_base_realize(Widget *widget)
{
    // Realize all children
    container_foreach(TUI_CONTAINER(widget), (GFunc) widget_realize, NULL);
    // Chain up parent class realize
    TUI_WIDGET_CLASS(container_parent_class)->realize(widget);
}

static gint
container_base_draw(Widget *widget)
{
    // Draw each of the container children
    container_foreach(TUI_CONTAINER(widget), (GFunc) widget_draw, NULL);
    //  Chain up parent class draw
    return TUI_WIDGET_CLASS(container_parent_class)->draw(widget);
}

static void
container_base_map(Widget *widget)
{
    // Map each of the container children
    container_foreach(TUI_CONTAINER(widget), (GFunc) widget_map, NULL);
    //  Chain up parent class map
    TUI_WIDGET_CLASS(container_parent_class)->map(widget);
}

static void
container_base_add(Container *container, Widget *widget)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    priv->children = g_list_append(priv->children, widget);
    widget_set_parent(widget, TUI_WIDGET(container));
}

static void
container_base_remove(G_GNUC_UNUSED Container *container, Widget *widget)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    priv->children = g_list_remove(priv->children, widget);
    widget_set_parent(widget, NULL);
}


static void
container_finalize(GObject *object)
{
    // Chain-up parent finalize function
    G_OBJECT_CLASS(container_parent_class)->finalize(object);
}

static void
container_class_init(ContainerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = container_finalize;

    WidgetClass *widget_class = TUI_WIDGET_CLASS(klass);
    widget_class->realize = container_base_realize;
    widget_class->draw = container_base_draw;
    widget_class->map = container_base_map;

    ContainerClass *container_class = TUI_CONTAINER_CLASS(klass);
    container_class->add = container_base_add;
    container_class->remove = container_base_remove;
}

static void
container_init(G_GNUC_UNUSED Container *container)
{
}
