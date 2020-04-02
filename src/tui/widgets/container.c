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
 * @file {file}
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include "tui/widgets/container.h"

typedef struct
{
    //! List of Widget children
    GNode *children;
} ContainerPrivate;

typedef struct
{
    //! Position to search
    gint x, y;
    //! Widget pointer if found, NULL if not found
    Widget *found;
} ContainerFindData;

// Class definition
G_DEFINE_TYPE_WITH_PRIVATE(Container, container, TUI_TYPE_WIDGET)

void
container_add_child(Container *container, Widget *child)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    priv->children = g_list_append(priv->children, (gpointer) child);
    widget_set_parent(child, TUI_WIDGET(container));
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

static void
container_check_child_position(Widget *widget, gpointer data)
{
    ContainerFindData *find_data = data;

    if (widget_is_visible(widget) == FALSE) {
        return;
    }

    if (TUI_IS_CONTAINER(widget)) {
        find_data->found = container_find_by_position(
            TUI_CONTAINER(widget),
            find_data->x,
            find_data->y
        );
    }

    if (find_data->found == NULL) {
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
    ContainerPrivate *priv = container_get_instance_private(container);
    ContainerFindData find_data = {
        .x = x,
        .y = y,
        .found = NULL,
    };

    g_list_foreach(priv->children,
                   (GFunc) container_check_child_position,
                   &find_data
    );

    return find_data.found;
}

static void
container_draw_child(Widget *widget, G_GNUC_UNUSED gpointer data)
{
    widget_draw(widget);
}

static gint
container_draw(Widget *widget)
{
    ContainerPrivate *priv = container_get_instance_private(TUI_CONTAINER(widget));
    g_list_foreach(
        priv->children,
        (GFunc) container_draw_child,
        NULL
    );

    return 0;
}

static void
container_finalize(GObject *object)
{
    ContainerPrivate *priv = container_get_instance_private(TUI_CONTAINER(object));
    g_list_free(priv->children);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(container_parent_class)->finalize(object);
}

static void
container_class_init(ContainerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = container_finalize;

    WidgetClass *widget_class = TUI_WIDGET_CLASS(klass);
    widget_class->draw = container_draw;
}

static void
container_init(Container *container)
{
    ContainerPrivate *priv = container_get_instance_private(container);
    priv->children = NULL;
}
