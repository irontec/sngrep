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
    //! SngWidget pointer if found, NULL if not found
    SngWidget *found;
} ContainerFindData;

typedef struct
{
    GList *children;
} ContainerPrivate;

// Class definition
G_DEFINE_TYPE_WITH_PRIVATE(Container, container, SNG_TYPE_WIDGET)

void
container_add(Container *container, SngWidget *child)
{
    ContainerClass *klass = TUI_CONTAINER_GET_CLASS(container);
    if (klass->add != NULL) {
        klass->add(container, child);
    }
}

void
container_remove(Container *container, SngWidget *child)
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

SngWidget *
container_get_child(Container *container, gint index)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    return g_list_nth_data(priv->children, index);
}

static gint
container_check_child_position(SngWidget *widget, gpointer data)
{
    ContainerFindData *find_data = data;

    if (sng_widget_is_visible(widget) == FALSE) {
        return TRUE;
    }
    // Containers check their children first
    if (TUI_IS_CONTAINER(widget)) {
        if (find_data->found == NULL) {
            find_data->found = container_find_by_position(TUI_CONTAINER(widget), find_data->x, find_data->y);
        }
    }

    // Only check focusable widget position
    if (find_data->found == NULL && sng_widget_can_focus(widget)) {
        // Then check if widget has been clicked
        if (find_data->x >= sng_widget_get_xpos(widget)
            && find_data->x < sng_widget_get_xpos(widget) + sng_widget_get_width(widget)
            && find_data->y >= sng_widget_get_ypos(widget)
            && find_data->y < sng_widget_get_ypos(widget) + sng_widget_get_height(widget)) {
            find_data->found = widget;
        }
    }

    return find_data->found == NULL;
}

SngWidget *
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
    container_foreach(container, (GFunc) sng_widget_show, NULL);
    // Show container itself
    sng_widget_show(SNG_WIDGET(container));
}

static void
container_base_realize(SngWidget *widget)
{
    // Realize all children
    container_foreach(TUI_CONTAINER(widget), (GFunc) sng_widget_realize, NULL);
    // Chain up parent class realize
    SNG_WIDGET_CLASS(container_parent_class)->realize(widget);
}

static gint
container_base_draw(SngWidget *widget)
{
    // Draw each of the container children
    container_foreach(TUI_CONTAINER(widget), (GFunc) sng_widget_draw, NULL);
    //  Chain up parent class draw
    return SNG_WIDGET_CLASS(container_parent_class)->draw(widget);
}

static void
container_base_map(SngWidget *widget)
{
    // Map each of the container children
    container_foreach(TUI_CONTAINER(widget), (GFunc) sng_widget_map, NULL);
    //  Chain up parent class map
    SNG_WIDGET_CLASS(container_parent_class)->map(widget);
}

static void
container_base_add(Container *container, SngWidget *widget)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    priv->children = g_list_append(priv->children, widget);
    sng_widget_set_parent(widget, SNG_WIDGET(container));
}

static void
container_base_remove(G_GNUC_UNUSED Container *container, SngWidget *widget)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    priv->children = g_list_remove(priv->children, widget);
    sng_widget_set_parent(widget, NULL);
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

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
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
