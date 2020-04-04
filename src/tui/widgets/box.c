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
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
    BoxOrientation orientation;
} BoxPrivate;

// Class definition
G_DEFINE_TYPE_WITH_PRIVATE(Box, box, TUI_TYPE_CONTAINER)

Widget *
box_new(BoxOrientation orientation)
{
    return g_object_new(
        TUI_TYPE_BOX,
        "orientation", orientation,
        "vexpand", TRUE,
        "hexpand", TRUE,
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
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static gint
box_draw(Widget *widget)
{
    Box *box = TUI_BOX(widget);
    BoxPrivate *priv = box_get_instance_private(box);
    GNode *children = container_get_children(TUI_CONTAINER(widget));

    // Set Children width based on expand flag
    if (priv->orientation == BOX_ORIENTATION_VERTICAL) {
        gint space = widget_get_height(widget);
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
        gint ypos = 0;
        for (guint i = 0; i < g_node_n_children(children); i++) {
            Widget *child = g_node_nth_child_data(children, i);
            if (widget_get_vexpand(child)) {
                widget_set_size(child, widget_get_width(child), space / exp_widget_cnt);
            }
            if (widget_get_hexpand(child)) {
                widget_set_size(child, widget_get_width(widget), widget_get_height(child));
            }
            widget_set_position(child, widget_get_xpos(widget), ypos);
            ypos += widget_get_height(child);
        }
    } else {
        gint space = widget_get_width(widget);
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
        gint xpos = 0;
        for (guint i = 0; i < g_node_n_children(children); i++) {
            Widget *child = g_node_nth_child_data(children, i);
            if (widget_get_hexpand(child)) {
                widget_set_size(child, space / exp_widget_cnt, widget_get_height(child));
            }
            if (widget_get_vexpand(child)) {
                widget_set_size(child, widget_get_width(child), widget_get_height(widget));
            }
            widget_set_position(child, xpos, widget_get_ypos(widget));
            xpos += widget_get_width(child);
        }
    }

    TUI_WIDGET_CLASS(box_parent_class)->draw(widget);
    werase(widget_get_ncurses_window(widget));
    return 0;
}

static void
box_class_init(BoxClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = box_set_property;
    object_class->get_property = box_get_property;

    WidgetClass *widget_class = TUI_WIDGET_CLASS(klass);
    widget_class->draw = box_draw;

    obj_properties[PROP_ORIENTATION] =
        g_param_spec_enum("orientation",
                          "Box orientation",
                          "Box Layout orientation",
                          BOX_TYPE_ORIENTATION,
                          BOX_ORIENTATION_VERTICAL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT
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
