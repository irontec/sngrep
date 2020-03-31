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
#include "tui/widget.h"

enum
{
    PROP_WIDGET_HEIGHT = 1,
    PROP_WIDGET_WIDTH,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
    //! Parent widget
    Widget *parent;
    //! Window for drawing this widget
    WINDOW *win;
    //! Dimensions of this widget
    gint height, width;
    //! Position of this widget in screen
    gint x, y;
} WidgetPrivate;

// Widget class definition
G_DEFINE_TYPE_WITH_PRIVATE(Widget, widget, G_TYPE_OBJECT)

Widget *
widget_new()
{
    return g_object_new( TUI_TYPE_WIDGET, NULL );
}

void
widget_free(Widget *widget)
{
    g_object_unref(widget);
}

WINDOW *
widget_get_ncurses_window(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->win;
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

gint
widget_draw(Widget *widget)
{
    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);
    if (klass->draw != NULL) {
        return klass->draw(widget);
    }

    return 0;
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

    // Get current screen dimensions
    gint maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);

    // If panel doesn't fill the screen center it
    if (priv->height != maxy) priv->x = abs((maxy - priv->height) / 2);
    if (priv->width != maxx) priv->y = abs((maxx - priv->width) / 2);

    priv->win = newwin(priv->height, priv->width, priv->x, priv->y);
    wtimeout(priv->win, 0);
    keypad(priv->win, TRUE);

    /* update the object state depending on constructor properties */
    G_OBJECT_CLASS(widget_parent_class)->constructed(object);
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
        case PROP_WIDGET_HEIGHT:
            priv->height = g_value_get_int(value);
            break;
        case PROP_WIDGET_WIDTH:
            priv->width = g_value_get_int(value);
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
        case PROP_WIDGET_HEIGHT:
            g_value_set_int(value, priv->height);
            break;
        case PROP_WIDGET_WIDTH:
            g_value_set_int(value, priv->width);
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
    object_class->finalize = widget_finalize;
    object_class->set_property = widget_set_property;
    object_class->get_property = widget_get_property;

    klass->draw = widget_base_draw;

    obj_properties[PROP_WIDGET_HEIGHT] =
        g_param_spec_int("height",
                         "Widget Height",
                         "Initial window height",
                         0,
                         getmaxy(stdscr),
                         getmaxy(stdscr),
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_WIDGET_WIDTH] =
        g_param_spec_int("width",
                         "Widget Width",
                         "Initial window Width",
                         0,
                         getmaxx(stdscr),
                         getmaxx(stdscr),
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );

}

static void
widget_init(Widget *self)
{
    WidgetPrivate *priv = widget_get_instance_private(self);
    // Initialize window position
    priv->x = priv->y = 0;
}
