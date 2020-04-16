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
 * @file box.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 */
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include "glib-extra/glib_enum_types.h"
#include "glib-extra/gnode.h"
#include "tui/theme.h"
#include "tui/widgets/box.h"

enum
{
    PROP_ORIENTATION = 1,
    PROP_SPACING,
    PROP_PADDING,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
    // Vertical or Horizontal box
    BoxOrientation orientation;
    // Space between children widgets
    gint spacing;
    // Padding at the beginning and end of box
    gint padding;
} BoxPrivate;

// Class definition
G_DEFINE_TYPE_WITH_PRIVATE(Box, box, TUI_TYPE_CONTAINER)

Widget *
box_new(BoxOrientation orientation)
{
    return box_new_full(orientation, 0, 0);
}

Widget *
box_new_full(BoxOrientation orientation, gint spacing, gint padding)
{
    return g_object_new(
        TUI_TYPE_BOX,
        "orientation", orientation,
        "spacing", spacing,
        "padding", padding,
        "vexpand", TRUE,
        "hexpand", TRUE,
        "can-focus", FALSE,
        NULL
    );
}

static void
box_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    BoxPrivate *priv = box_get_instance_private(TUI_BOX(object));
    switch (property_id) {
        case PROP_ORIENTATION:
            priv->orientation = g_value_get_enum(value);
            break;
        case PROP_SPACING:
            priv->spacing = g_value_get_int(value);
            break;
        case PROP_PADDING:
            priv->padding = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
box_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    BoxPrivate *priv = box_get_instance_private(TUI_BOX(object));
    switch (property_id) {
        case PROP_ORIENTATION:
            g_value_set_enum(value, priv->orientation);
            break;
        case PROP_SPACING:
            g_value_set_int(value, priv->spacing);
            break;
        case PROP_PADDING:
            g_value_set_int(value, priv->padding);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
box_realize_horizontal(Widget *widget)
{
    Box *box = TUI_BOX(widget);
    BoxPrivate *priv = box_get_instance_private(box);
    GNode *children = container_get_children(TUI_CONTAINER(widget));

    gint space = widget_get_width(widget)
                 - priv->padding * 2
                 - priv->spacing * g_node_n_children(children);

    gint exp_widget_cnt = 0;
    // Remove fixed space from non expanded widgets
    for (guint i = 0; i < g_node_n_children(children); i++) {
        Widget *child = g_node_nth_child_data(children, i);
        if (!widget_get_hexpand(child)) {
            space -= widget_get_width(child);
        } else {
            exp_widget_cnt++;
        }
    }

    // Calculate expanded children width and positions
    gint xpos = priv->padding;
    for (guint i = 0; i < g_node_n_children(children); i++) {
        Widget *child = g_node_nth_child_data(children, i);
        if (widget_get_hexpand(child)) {
            widget_set_size(child, space / exp_widget_cnt, widget_get_height(child));
        }
        if (widget_get_vexpand(child)) {
            widget_set_size(child, widget_get_width(child), widget_get_height(widget));
        }
        widget_set_position(child, xpos, widget_get_ypos(widget));
        xpos += widget_get_width(child) + priv->spacing;
    }
}

static void
box_realize_vertical(Widget *widget)
{
    Box *box = TUI_BOX(widget);
    BoxPrivate *priv = box_get_instance_private(box);
    GNode *children = container_get_children(TUI_CONTAINER(widget));

    // Set Children width based on expand flag
    gint space = widget_get_height(widget)
                 - priv->padding * 2
                 - priv->spacing * g_node_n_children(children);

    gint exp_widget_cnt = 0;
    // Remove fixed space from non expanded widgets
    for (guint i = 0; i < g_node_n_children(children); i++) {
        Widget *child = g_node_nth_child_data(children, i);
        if (!widget_get_vexpand(child)) {
            space -= widget_get_height(child);
        } else {
            exp_widget_cnt++;
        }
    }

    // Calculate expanded children height and positions
    gint ypos = priv->padding;
    for (guint i = 0; i < g_node_n_children(children); i++) {
        Widget *child = g_node_nth_child_data(children, i);
        if (widget_get_vexpand(child)) {
            widget_set_size(child, widget_get_width(child), space / exp_widget_cnt);
        }
        if (widget_get_hexpand(child)) {
            widget_set_size(child, widget_get_width(widget), widget_get_height(child));
        }
        widget_set_position(child, widget_get_xpos(widget), ypos);
        ypos += widget_get_height(child) + priv->spacing;
    }
}

static void
box_realize(Widget *widget)
{
    Box *box = TUI_BOX(widget);
    BoxPrivate *priv = box_get_instance_private(box);

    if (priv->orientation == BOX_ORIENTATION_VERTICAL) {
        box_realize_vertical(widget);
    } else {
        box_realize_horizontal(widget);
    }

    TUI_WIDGET_CLASS(box_parent_class)->realize(widget);
}

static gint
box_draw(Widget *widget)
{
    // Clear the window to draw children widgets
    WINDOW *win = widget_get_ncurses_window(widget);
    werase(win);
    return TUI_WIDGET_CLASS(box_parent_class)->draw(widget);
}

static void
box_class_init(BoxClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = box_set_property;
    object_class->get_property = box_get_property;

    WidgetClass *widget_class = TUI_WIDGET_CLASS(klass);
    widget_class->realize = box_realize;
    widget_class->draw = box_draw;

    obj_properties[PROP_ORIENTATION] =
        g_param_spec_enum("orientation",
                          "Box orientation",
                          "Box Layout orientation",
                          BOX_TYPE_ORIENTATION,
                          BOX_ORIENTATION_VERTICAL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT
        );

    obj_properties[PROP_SPACING] =
        g_param_spec_int("spacing",
                         "Box Spacing",
                         "Space between children widgets",
                         0,
                         G_MAXINT,
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_PADDING] =
        g_param_spec_int("padding",
                         "Box Padding",
                         "Padding at the beginning and end of box",
                         0,
                         G_MAXINT,
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}

static void
box_init(G_GNUC_UNUSED Box *box)
{
}
