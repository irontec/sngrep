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

// Class definition
G_DEFINE_TYPE(Container, container, TUI_TYPE_WIDGET)

void
container_add_child(Container *container, Widget *child)
{
    g_node_append(
        widget_get_node(TUI_WIDGET(container)),
        widget_get_node(child)
    );
}

GNode *
container_get_children(Container *container)
{
    return widget_get_node(TUI_WIDGET(container));
}

Widget *
container_get_child(Container *container, gint index)
{
    return g_node_nth_child_data(
        widget_get_node(TUI_WIDGET(container)),
        index
    );
}

static void
container_check_child_position(GNode *node, gpointer data)
{
    ContainerFindData *find_data = data;
    Widget *widget = node->data;

    if (widget_is_visible(widget) == FALSE) {
        return;
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
}

Widget *
container_find_by_position(Container *container, gint x, gint y)
{
    ContainerFindData find_data = {
        .x = x,
        .y = y,
        .found = NULL,
    };

    g_node_children_foreach(
        widget_get_node(TUI_WIDGET(container)),
        G_TRAVERSE_ALL,
        container_check_child_position,
        &find_data
    );

    return find_data.found;
}

static void
container_show_child(GNode *child, G_GNUC_UNUSED gpointer data)
{
    widget_show(child->data);
}

void
container_show_all(Container *container)
{
    // TODO maybe it's more logical if this function shows
    // TODO all children instead of the first level one

    // Show all container children
    g_node_children_foreach(
        widget_get_node(TUI_WIDGET(container)),
        G_TRAVERSE_ALL,
        container_show_child,
        NULL
    );

    // Show container itself
    widget_show(TUI_WIDGET(container));
}

static void
container_realize_child(GNode *node, G_GNUC_UNUSED gpointer data)
{
    widget_realize(node->data);
}

static void
container_realize(Widget *widget)
{
    // Draw each of the container children
    g_node_children_foreach(
        widget_get_node(widget),
        G_TRAVERSE_ALL,
        container_realize_child,
        NULL
    );

    TUI_WIDGET_CLASS(container_parent_class)->realize(widget);
}

static void
container_draw_child(GNode *node, G_GNUC_UNUSED gpointer data)
{
    widget_draw(node->data);
}

static gint
container_draw(Widget *widget)
{
    // Draw each of the container children
    g_node_children_foreach(
        widget_get_node(widget),
        G_TRAVERSE_ALL,
        container_draw_child,
        NULL
    );

    return TUI_WIDGET_CLASS(container_parent_class)->draw(widget);
}

static void
container_map_child(GNode *node, G_GNUC_UNUSED gpointer data)
{
    if (!widget_is_floating(node->data)) {
        widget_map(node->data);
    }
}

static void
container_map(Widget *widget)
{
    // Draw each of the container children
    g_node_children_foreach(
        widget_get_node(widget),
        G_TRAVERSE_ALL,
        container_map_child,
        NULL
    );

    TUI_WIDGET_CLASS(container_parent_class)->map(widget);
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
    widget_class->realize = container_realize;
    widget_class->draw = container_draw;
    widget_class->map = container_map;
}

static void
container_init(G_GNUC_UNUSED Container *container)
{
}
