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
 * @file window.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in ui_panel.h
 */

#include "config.h"
#include <ncurses.h>
#include "glib-extra/glib.h"
#include "widget.h"

enum
{
    SIG_DESTROY,
    SIGS
};


enum
{
    PROP_HEIGHT = 1,
    PROP_WIDTH,
    PROP_VEXPAND,
    PROP_HEXPAND,
    N_PROPERTIES
};

static guint signals[SIGS] = { 0 };

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
    //! Parent widget
    GNode *node;
    //! Window for drawing this widget
    WINDOW *win;
    //! Dimensions of this widget
    gint height, width;
    //! Position of this widget in screen
    gint x, y;
    //! Determine if this widget is displayed on the screen
    gboolean visible;
    //! Determine the fill mode in layouts
    gboolean vexpand, hexpand;
} WidgetPrivate;

// Widget class definition
G_DEFINE_TYPE_WITH_PRIVATE(Widget, widget, G_TYPE_OBJECT)

Widget *
widget_new(const Widget *parent)
{
    return g_object_new(
        TUI_TYPE_WIDGET,
        "parent", parent,
        NULL
    );
}

void
widget_destroy(Widget *widget)
{
    g_object_unref(widget);
}

void
widget_show(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->visible = TRUE;
}

void
widget_hide(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->visible = FALSE;
}

gboolean
widget_is_visible(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->visible;
}

Widget *
widget_get_toplevel(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    GNode *root = g_node_get_root(priv->node);
    return (root != NULL) ? root->data : NULL;
}

Widget *
widget_get_parent(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return g_node_parent_data(priv->node);
}

GNode *
widget_get_node(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->node;
}

void
widget_set_ncurses_window(Widget *widget, WINDOW *win)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    if (priv->win)
        delwin(priv->win);
    priv->win = win;
}

WINDOW *
widget_get_ncurses_window(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->win;
}

void
widget_set_size(Widget *widget, gint width, gint height)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->width = width;
    priv->height = height;
}

void
widget_set_width(Widget *widget, gint width)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->width = width;
}

gint
widget_get_width(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->width;
}

void
widget_set_height(Widget *widget, gint height)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->height = height;
}

gint
widget_get_height(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->height;
}

void
widget_set_position(Widget *widget, gint xpos, gint ypos)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->x = xpos;
    priv->y = ypos;
}

gint
widget_get_xpos(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->x;
}

gint
widget_get_ypos(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->y;
}

void
widget_set_vexpand(Widget *widget, gboolean expand)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->vexpand = expand;
}

gboolean
widget_get_vexpand(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->vexpand;
}

void
widget_set_hexpand(Widget *widget, gboolean expand)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->hexpand = expand;
}

gboolean
widget_get_hexpand(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->hexpand;
}

gint
widget_draw(Widget *widget)
{
    // Only for visible widgets
    if (!widget_is_visible(widget)) {
        return 0;
    }

    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);
    if (klass->draw != NULL) {
        return klass->draw(widget);
    }

    return 0;
}

gboolean
widget_focus_gain(Widget *widget)
{
    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);
    if (klass->focus_gained != NULL) {
        return klass->focus_gained(widget);
    }
    return TRUE;
}

void
widget_focus_lost(Widget *widget)
{
    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);
    if (klass->focus_lost != NULL) {
        klass->focus_lost(widget);
    }
}

gint
widget_clicked(Widget *widget, MEVENT event)
{
    gint hld = KEY_NOT_HANDLED;

    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);
    if (klass->clicked != NULL) {
        hld = klass->clicked(widget, event);
    }

    return hld;
}

gint
widget_key_pressed(Widget *widget, gint key)
{
    gint hld = KEY_NOT_HANDLED;

    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);
    if (klass->key_pressed != NULL) {
        hld = klass->key_pressed(widget, key);
    }

    return hld;
}

static gint
widget_base_draw(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    touchwin(priv->win);
    return 0;
}

static void
widget_constructed(GObject *object)
{
    WidgetPrivate *priv = widget_get_instance_private(TUI_WIDGET(object));
    // Create widget window with requested dimensions
    if (priv->win == NULL) {
        priv->win = newwin(priv->height, priv->width, priv->y, priv->x);
    }
}

static void
widget_dispose(GObject *self)
{
    // Notify everyone we're being destroyed
    g_signal_emit(self, signals[SIG_DESTROY], 0);
    // Chain-up parent finalize function
    G_OBJECT_CLASS(widget_parent_class)->dispose(self);
}

static void
widget_finalize(GObject *self)
{
    WidgetPrivate *priv = widget_get_instance_private(TUI_WIDGET(self));
    // Deallocate ncurses pointers
    delwin(priv->win);
    // Chain-up parent finalize function
    G_OBJECT_CLASS(widget_parent_class)->finalize(self);
}

static void
widget_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    WidgetPrivate *priv = widget_get_instance_private(TUI_WIDGET(self));

    switch (property_id) {
        case PROP_HEIGHT:
            priv->height = g_value_get_int(value);
            break;
        case PROP_WIDTH:
            priv->width = g_value_get_int(value);
            break;
        case PROP_VEXPAND:
            priv->vexpand = g_value_get_boolean(value);
            break;
        case PROP_HEXPAND:
            priv->hexpand = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
widget_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    WidgetPrivate *priv = widget_get_instance_private(TUI_WIDGET(self));

    switch (property_id) {
        case PROP_HEIGHT:
            g_value_set_int(value, priv->height);
            break;
        case PROP_WIDTH:
            g_value_set_int(value, priv->width);
            break;
        case PROP_VEXPAND:
            g_value_set_boolean(value, priv->vexpand);
            break;
        case PROP_HEXPAND:
            g_value_set_boolean(value, priv->hexpand);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
widget_class_init(WidgetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = widget_constructed;
    object_class->dispose = widget_dispose;
    object_class->finalize = widget_finalize;
    object_class->set_property = widget_set_property;
    object_class->get_property = widget_get_property;

    klass->draw = widget_base_draw;

    obj_properties[PROP_HEIGHT] =
        g_param_spec_int("height",
                         "Widget Height",
                         "Initial window height",
                         0,
                         getmaxy(stdscr),
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_WIDTH] =
        g_param_spec_int("width",
                         "Widget Width",
                         "Initial window Width",
                         0,
                         getmaxx(stdscr),
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_VEXPAND] =
        g_param_spec_boolean("vexpand",
                            "Vertical Expansion",
                             "Vertical Expansion",
                            FALSE,
                            G_PARAM_READWRITE
        );

    obj_properties[PROP_HEXPAND] =
        g_param_spec_boolean("hexpand",
                             "Horizontal Expansion",
                             "Horizontal Expansion",
                             FALSE,
                             G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );

    signals[SIG_DESTROY] =
        g_signal_newv("destroy",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
        );

}

static void
widget_init(Widget *self)
{
    WidgetPrivate *priv = widget_get_instance_private(self);
    // Initialize window position
    priv->x = priv->y = 0;
    // Set our node for widget hierarchy
    priv->node = g_node_new(self);
}
