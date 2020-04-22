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
} SngContainerFindData;

typedef struct
{
    GList *children;
} SngContainerPrivate;

// Class definition
G_DEFINE_TYPE_WITH_PRIVATE(SngContainer, sng_container, SNG_TYPE_WIDGET)

void
sng_container_add(SngContainer *container, SngWidget *child)
{
    SngContainerClass *klass = SNG_CONTAINER_GET_CLASS(container);
    if (klass->add != NULL) {
        klass->add(container, child);
    }
}

void
sng_container_remove(SngContainer *container, SngWidget *child)
{
    SngContainerClass *klass = SNG_CONTAINER_GET_CLASS(container);
    if (klass->remove != NULL) {
        klass->remove(container, child);
    }
}

void
sng_container_foreach(SngContainer *container, GFunc callback, gpointer user_data)
{
    SngContainerPrivate *priv = sng_container_get_instance_private(container);

    // Draw each of the container children
    g_list_foreach(
        priv->children,
        callback,
        user_data
    );
}

GList *
sng_container_get_children(SngContainer *container)
{
    SngContainerPrivate *priv = sng_container_get_instance_private(container);
    return priv->children;
}

SngWidget *
sng_container_get_child(SngContainer *container, gint index)
{
    SngContainerPrivate *priv = sng_container_get_instance_private(container);
    return g_list_nth_data(priv->children, index);
}

static gint
sng_container_check_child_position(SngWidget *widget, gpointer data)
{
    SngContainerFindData *find_data = data;

    if (sng_widget_is_visible(widget) == FALSE) {
        return TRUE;
    }
    // Containers check their children first
    if (SNG_IS_CONTAINER(widget)) {
        if (find_data->found == NULL) {
            find_data->found = sng_container_find_by_position(SNG_CONTAINER(widget), find_data->x, find_data->y);
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
sng_container_find_by_position(SngContainer *container, gint x, gint y)
{
    SngContainerFindData find_data = {
        .x = x,
        .y = y,
        .found = NULL,
    };

    SngContainerPrivate *priv = sng_container_get_instance_private(container);
    g_list_find_custom(
        priv->children,
        &find_data,
        (GCompareFunc) sng_container_check_child_position
    );

    return find_data.found;
}

void
sng_container_show_all(SngContainer *container)
{
    // Show all children
    sng_container_foreach(container, (GFunc) sng_widget_show, NULL);
    // Show container itself
    sng_widget_show(SNG_WIDGET(container));
}

static void
sng_container_base_realize(SngWidget *widget)
{
    // Realize all children
    sng_container_foreach(SNG_CONTAINER(widget), (GFunc) sng_widget_realize, NULL);
    // Chain up parent class realize
    SNG_WIDGET_CLASS(sng_container_parent_class)->realize(widget);
}

static void
sng_container_base_draw(SngWidget *widget)
{
    // Draw each of the container children
    sng_container_foreach(SNG_CONTAINER(widget), (GFunc) sng_widget_draw, NULL);
    //  Chain up parent class draw
    SNG_WIDGET_CLASS(sng_container_parent_class)->draw(widget);
}

static void
sng_container_base_map(SngWidget *widget)
{
    // Map each of the container children
    sng_container_foreach(SNG_CONTAINER(widget), (GFunc) sng_widget_map, NULL);
    //  Chain up parent class map
    SNG_WIDGET_CLASS(sng_container_parent_class)->map(widget);
}

static void
sng_container_base_add(SngContainer *container, SngWidget *widget)
{
    SngContainerPrivate *priv = sng_container_get_instance_private(container);
    priv->children = g_list_append(priv->children, widget);
    sng_widget_set_parent(widget, SNG_WIDGET(container));
}

static void
sng_container_base_remove(G_GNUC_UNUSED SngContainer *container, SngWidget *widget)
{
    SngContainerPrivate *priv = sng_container_get_instance_private(container);
    priv->children = g_list_remove(priv->children, widget);
    sng_widget_set_parent(widget, NULL);
}

static void
sng_container_finalize(GObject *object)
{
    // Chain-up parent finalize function
    G_OBJECT_CLASS(sng_container_parent_class)->finalize(object);
}

static void
sng_container_class_init(SngContainerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = sng_container_finalize;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->realize = sng_container_base_realize;
    widget_class->draw = sng_container_base_draw;
    widget_class->map = sng_container_base_map;

    SngContainerClass *container_class = SNG_CONTAINER_CLASS(klass);
    container_class->add = sng_container_base_add;
    container_class->remove = sng_container_base_remove;
}

static void
sng_container_init(G_GNUC_UNUSED SngContainer *container)
{
}
