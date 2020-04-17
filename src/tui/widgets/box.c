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
    SngBoxOrientation orientation;
    // Space between children widgets
    gint spacing;
    // Padding at the beginning and end of box
    gint padding;
    // Background filler
    chtype background;
} SngBoxPrivate;

// Class definition
G_DEFINE_TYPE_WITH_PRIVATE(SngBox, sng_box, SNG_TYPE_CONTAINER)

SngWidget *
sng_box_new(SngBoxOrientation orientation)
{
    return sng_box_new_full(orientation, 0, 0);
}

SngWidget *
sng_box_new_full(SngBoxOrientation orientation, gint spacing, gint padding)
{
    return g_object_new(
        SNG_TYPE_BOX,
        "orientation", orientation,
        "spacing", spacing,
        "padding", padding,
        "vexpand", TRUE,
        "hexpand", TRUE,
        "can-focus", FALSE,
        NULL
    );
}

void
sng_box_pack_start(SngBox *box, SngWidget *widget)
{
    SngBoxPrivate *priv = sng_box_get_instance_private(box);

    if (priv->orientation == BOX_ORIENTATION_VERTICAL) {
        sng_widget_set_vexpand(widget, FALSE);
    } else {
        sng_widget_set_hexpand(widget, FALSE);
    }

    sng_container_add(SNG_CONTAINER(box), widget);
}

void
sng_box_set_background(SngBox *box, chtype background)
{
    SngBoxPrivate *priv = sng_box_get_instance_private(box);
    priv->background = background;
}

static void
sng_box_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngBoxPrivate *priv = sng_box_get_instance_private(SNG_BOX(object));
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
sng_box_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngBoxPrivate *priv = sng_box_get_instance_private(SNG_BOX(object));
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
sng_box_realize_horizontal(SngWidget *widget)
{
    SngBox *box = SNG_BOX(widget);
    SngBoxPrivate *priv = sng_box_get_instance_private(box);
    GList *children = sng_container_get_children(SNG_CONTAINER(widget));

    gint space = sng_widget_get_width(widget)
                 - priv->padding * 2
                 - priv->spacing * g_list_length(children);

    gint exp_widget_cnt = 0;
    // Remove fixed space from non expanded widgets
    for (GList *l = children; l != NULL; l = l->next) {
        SngWidget *child = l->data;
        if (!sng_widget_get_hexpand(child)) {
            space -= sng_widget_get_width(child);
        } else {
            exp_widget_cnt++;
        }
    }

    // Calculate expanded children width and positions
    gint xpos = priv->padding;
    for (GList *l = children; l != NULL; l = l->next) {
        SngWidget *child = l->data;
        if (sng_widget_get_hexpand(child)) {
            sng_widget_set_size(child, space / exp_widget_cnt, sng_widget_get_height(child));
        }
        if (sng_widget_get_vexpand(child)) {
            sng_widget_set_size(child, sng_widget_get_width(child), sng_widget_get_height(widget));
        }
        sng_widget_set_position(child, xpos, sng_widget_get_ypos(widget));
        xpos += sng_widget_get_width(child) + priv->spacing;
    }
}

static void
sng_box_realize_vertical(SngWidget *widget)
{
    SngBox *box = SNG_BOX(widget);
    SngBoxPrivate *priv = sng_box_get_instance_private(box);
    GList *children = sng_container_get_children(SNG_CONTAINER(widget));

    // Set Children width based on expand flag
    gint space = sng_widget_get_height(widget)
                 - priv->padding * 2
                 - priv->spacing * g_list_length(children);

    gint exp_widget_cnt = 0;
    // Remove fixed space from non expanded widgets
    for (GList *l = children; l != NULL; l = l->next) {
        SngWidget *child = l->data;
        if (!sng_widget_get_vexpand(child)) {
            space -= sng_widget_get_height(child);
        } else {
            exp_widget_cnt++;
        }
    }

    // Calculate expanded children height and positions
    gint ypos = priv->padding;
    for (GList *l = children; l != NULL; l = l->next) {
        SngWidget *child = l->data;
        if (sng_widget_get_vexpand(child)) {
            sng_widget_set_size(child, sng_widget_get_width(child), space / exp_widget_cnt);
        }
        if (sng_widget_get_hexpand(child)) {
            sng_widget_set_size(child, sng_widget_get_width(widget), sng_widget_get_height(child));
        }
        sng_widget_set_position(child, sng_widget_get_xpos(widget), ypos);
        ypos += sng_widget_get_height(child) + priv->spacing;
    }
}

static void
sng_box_realize(SngWidget *widget)
{
    SngBox *box = SNG_BOX(widget);
    SngBoxPrivate *priv = sng_box_get_instance_private(box);

    if (priv->orientation == BOX_ORIENTATION_VERTICAL) {
        sng_box_realize_vertical(widget);
    } else {
        sng_box_realize_horizontal(widget);
    }

    SNG_WIDGET_CLASS(sng_box_parent_class)->realize(widget);
}

static gint
sng_box_draw(SngWidget *widget)
{
    // Clear the window to draw children widgets
    SngBoxPrivate *priv = sng_box_get_instance_private(SNG_BOX(widget));
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wbkgd(win, priv->background);
    werase(win);
    return SNG_WIDGET_CLASS(sng_box_parent_class)->draw(widget);
}

static void
sng_box_class_init(SngBoxClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = sng_box_set_property;
    object_class->get_property = sng_box_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->realize = sng_box_realize;
    widget_class->draw = sng_box_draw;

    obj_properties[PROP_ORIENTATION] =
        g_param_spec_enum("orientation",
                          "Box orientation",
                          "Box Layout orientation",
                          SNG_TYPE_BOX_ORIENTATION,
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
sng_box_init(G_GNUC_UNUSED SngBox *box)
{
}
