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
#include <glib.h>
#include <glib-object.h>
#include <ncurses.h>
#include "widget.h"

enum
{
    SIG_REALIZE,
    SIG_DRAW,
    SIG_MAP,
    SIG_KEY_PRESSED,
    SIG_CLICKED,
    SIG_LOSE_FOCUS,
    SIG_GRAB_FOCUS,
    SIG_DESTROY,
    SIGS
};

enum
{
    PROP_HEIGHT = 1,
    PROP_WIDTH,
    PROP_VEXPAND,
    PROP_HEXPAND,
    PROP_FLOATING,
    PROP_VISIBLE,
    PROP_CAN_FOCUS,
    N_PROPERTIES
};

static guint signals[SIGS] = { 0 };

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
    //! Parent widget
    SngWidget *parent;
    //! Widget name
    const gchar *name;
    //! Window for drawing this widget
    WINDOW *win;
    //! Dimensions of this widget
    gint height, width;
    //! Position of this widget in screen
    gint x, y;
    //! Determine if this widget is displayed on the screen
    gboolean visible;
    //! Determine if the widget can be focused
    gboolean can_focus;
    //! Determine if this widget has window focus
    gboolean focused;
    //! Determine the fill mode in layouts
    gboolean vexpand, hexpand;
    //! Determine if the widget must be drawn on topmost layer
    gboolean floating;
    //! Determine if the widget is being destroyed
    gboolean destroying;
} SngWidgetPrivate;

// SngWidget class definition
G_DEFINE_TYPE_WITH_PRIVATE(SngWidget, sng_widget, G_TYPE_OBJECT)

SngWidget *
sng_widget_new()
{
    return g_object_new(
        SNG_TYPE_WIDGET,
        NULL
    );
}

void
sng_widget_free(SngWidget *widget)
{
    g_object_unref(widget);
}

void
sng_widget_destroy(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->destroying = TRUE;
}

gboolean
sng_widget_is_destroying(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->destroying;
}

void
sng_widget_set_parent(SngWidget *widget, SngWidget *parent)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->parent = parent;
}

SngWidget *
sng_widget_get_parent(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->parent;
}

void
sng_widget_set_name(SngWidget *widget, const gchar *name)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->name = name;
}

SngWidget *
sng_widget_get_toplevel(SngWidget *widget)
{
    while (sng_widget_get_parent(widget) != NULL) {
        widget = sng_widget_get_parent(widget);
    }
    return widget;
}

void
sng_widget_show(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->visible = TRUE;
}

void
sng_widget_hide(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->visible = FALSE;
}

gboolean
sng_widget_is_visible(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->visible;
}

gboolean
sng_widget_is_realized(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->win != NULL;
}

gboolean
sng_widget_can_focus(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->can_focus;
}

gboolean
sng_widget_has_focus(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->focused;
}

void
sng_widget_set_ncurses_window(SngWidget *widget, WINDOW *win)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    if (priv->win)
        delwin(priv->win);
    priv->win = win;
}

WINDOW *
sng_widget_get_ncurses_window(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->win;
}

void
sng_widget_set_size(SngWidget *widget, gint width, gint height)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->width = width;
    priv->height = height;
}

void
sng_widget_set_width(SngWidget *widget, gint width)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->width = width;
}

gint
sng_widget_get_width(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->width;
}

void
sng_widget_set_height(SngWidget *widget, gint height)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->height = height;
}

gint
sng_widget_get_height(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->height;
}

void
sng_widget_set_position(SngWidget *widget, gint xpos, gint ypos)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->x = xpos;
    priv->y = ypos;
}

gint
sng_widget_get_xpos(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->x;
}

gint
sng_widget_get_ypos(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->y;
}

void
sng_widget_set_vexpand(SngWidget *widget, gboolean expand)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->vexpand = expand;
}

gboolean
sng_widget_get_vexpand(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->vexpand;
}

void
sng_widget_set_hexpand(SngWidget *widget, gboolean expand)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->hexpand = expand;
}

gboolean
sng_widget_get_hexpand(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->hexpand;
}

void
sng_widget_set_floating(SngWidget *widget, gboolean floating)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->floating = floating;
}

gboolean
sng_widget_is_floating(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    return priv->floating;
}

void
sng_widget_realize(SngWidget *widget)
{
    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);

    // Determine widget size before realize
    sng_widget_size_request(widget);

    if (sng_widget_is_realized(widget)) {
        return;
    }

    if (klass->realize != NULL) {
        klass->realize(widget);
    }

    // Notify everyone we're being realized
    g_signal_emit(widget, signals[SIG_REALIZE], 0);
}

void
sng_widget_draw(SngWidget *widget)
{
    g_return_if_fail(widget != NULL);

    // Only for visible widgets
    if (!sng_widget_is_visible(widget)) {
        return;
    }

    // Realize widget before drawing
    sng_widget_realize(widget);

    // Notify everyone we're being drawn
    g_signal_emit(widget, signals[SIG_DRAW], 0);

    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->draw != NULL) {
        klass->draw(widget);
    }
}

void
sng_widget_map(SngWidget *widget)
{
    // Only for visible widgets
    if (!sng_widget_is_visible(widget)) {
        return;
    }

    // Notify everyone we're being mapped
    g_signal_emit(widget, signals[SIG_MAP], 0);

    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->map != NULL) {
        klass->map(widget);
    }
}


void
sng_widget_focus_gain(SngWidget *widget)
{
    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->focus_gained != NULL) {
        klass->focus_gained(widget);
    }
}

void
sng_widget_focus_lost(SngWidget *widget)
{
    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->focus_lost != NULL) {
        klass->focus_lost(widget);
    }
}

void
sng_widget_lose_focus(SngWidget *widget)
{
    g_signal_emit(widget, signals[SIG_LOSE_FOCUS], 0);
}

void
sng_widget_grab_focus(SngWidget *widget)
{
    g_signal_emit(widget, signals[SIG_GRAB_FOCUS], 0);
}

void
sng_widget_clicked(SngWidget *widget, MEVENT event)
{
    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->clicked != NULL) {
        klass->clicked(widget, event);
    }

    // Notify everyone we're being clicked
    g_signal_emit(widget, signals[SIG_CLICKED], 0);
}

void
sng_widget_key_pressed(SngWidget *widget, gint key)
{
    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->key_pressed != NULL) {
        klass->key_pressed(widget, key);
    }

    // Notify everyone we've received a new key
    g_signal_emit(widget, signals[SIG_KEY_PRESSED], 0);
}

void
sng_widget_size_request(SngWidget *widget)
{
    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->size_request != NULL) {
        klass->size_request(widget);
    }
}

gint
sng_widget_get_preferred_height(SngWidget *widget)
{
    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->preferred_height != NULL) {
        return klass->preferred_height(widget);
    }
    return 0;
}

gint
sng_widget_get_preferred_width(SngWidget *widget)
{
    SngWidgetClass *klass = SNG_WIDGET_GET_CLASS(widget);
    if (klass->preferred_width != NULL) {
        return klass->preferred_width(widget);
    }
    return 0;
}

static void
sng_widget_base_realize(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);

    // Create widget window with requested dimensions
    if (priv->win == NULL) {
        priv->win = newpad(priv->height, priv->width);
    } else {
        wresize(priv->win, priv->height, priv->width);
    }
}

static void
sng_widget_base_draw(G_GNUC_UNUSED SngWidget *widget)
{
}

static void
sng_widget_base_size_request(G_GNUC_UNUSED SngWidget *widget)
{
}

static void
sng_widget_base_map(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    SngWidget *parent = sng_widget_get_parent(widget);
    if (priv->floating) {
        parent = sng_widget_get_toplevel(widget);
    }

    // Topmost widget, just refresh its window
    if (parent == NULL) {
        touchwin(priv->win);
        return;
    }

    // Set copywin parameters
    WINDOW *srcwin = priv->win;
    WINDOW *dstwin = sng_widget_get_ncurses_window(parent);
    gint sminrow = 0, smincol = 0;
    gint dminrow = priv->y - sng_widget_get_ypos(parent);
    gint dmincol = priv->x - sng_widget_get_xpos(parent);
    gint dmaxrow = dminrow + priv->height - 1;
    gint dmaxcol = dmincol + priv->width - 1;

    if (priv->name) {
        g_debug("Mapping widget %s at %d %d %d %d",
                priv->name,
                dminrow, dmincol, dmaxrow, dmaxrow
        );
    }

    // Copy the widget in its parent widget ncurses window
    copywin(
        srcwin, dstwin,
        sminrow, smincol,
        dminrow, dmincol, dmaxrow, dmaxcol,
        FALSE
    );
}

static void
sng_widget_base_focus_gained(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->focused = TRUE;
}

static void
sng_widget_base_focus_lost(SngWidget *widget)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(widget);
    priv->focused = FALSE;
}

static void
sng_widget_base_key_pressed(SngWidget *widget, gint key)
{
    // Pass key to parent widget
    SngWidget *parent = sng_widget_get_parent(widget);
    if (parent != NULL) {
        sng_widget_key_pressed(parent, key);
    }
}

static void
sng_widget_constructed(G_GNUC_UNUSED GObject *object)
{
}

static void
sng_widget_dispose(GObject *self)
{
    // Notify everyone we're being destroyed
    g_signal_emit(self, signals[SIG_DESTROY], 0);
    // Chain-up parent finalize function
    G_OBJECT_CLASS(sng_widget_parent_class)->dispose(self);
}

static void
sng_widget_finalize(GObject *self)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(SNG_WIDGET(self));
    // Deallocate ncurses pointers
    delwin(priv->win);
    // Chain-up parent finalize function
    G_OBJECT_CLASS(sng_widget_parent_class)->finalize(self);
}

static void
sng_widget_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(SNG_WIDGET(self));

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
        case PROP_FLOATING:
            priv->floating = g_value_get_boolean(value);
            break;
        case PROP_VISIBLE:
            priv->visible = g_value_get_boolean(value);
            break;
        case PROP_CAN_FOCUS:
            priv->can_focus = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_widget_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(SNG_WIDGET(self));

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
        case PROP_FLOATING:
            g_value_set_boolean(value, priv->floating);
            break;
        case PROP_VISIBLE:
            g_value_set_boolean(value, priv->visible);
            break;
        case PROP_CAN_FOCUS:
            g_value_set_boolean(value, priv->can_focus);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_widget_class_init(SngWidgetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = sng_widget_constructed;
    object_class->dispose = sng_widget_dispose;
    object_class->finalize = sng_widget_finalize;
    object_class->set_property = sng_widget_set_property;
    object_class->get_property = sng_widget_get_property;

    klass->realize = sng_widget_base_realize;
    klass->map = sng_widget_base_map;
    klass->draw = sng_widget_base_draw;
    klass->focus_gained = sng_widget_base_focus_gained;
    klass->focus_lost = sng_widget_base_focus_lost;
    klass->key_pressed = sng_widget_base_key_pressed;
    klass->size_request = sng_widget_base_size_request;
    klass->preferred_height = sng_widget_get_height;
    klass->preferred_width = sng_widget_get_width;

    obj_properties[PROP_HEIGHT] =
        g_param_spec_int("height",
                         "SngWidget Height",
                         "SngWidget height",
                         0,
                         getmaxy(stdscr),
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_WIDTH] =
        g_param_spec_int("width",
                         "SngWidget Width",
                         "SngWidget Width",
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

    obj_properties[PROP_FLOATING] =
        g_param_spec_boolean("floating",
                             "Floating SngWidget flag",
                             "Floating SngWidget flag",
                             FALSE,
                             G_PARAM_READWRITE
        );

    obj_properties[PROP_VISIBLE] =
        g_param_spec_boolean("visible",
                             "Visible SngWidget flag",
                             "Visible SngWidget flag",
                             FALSE,
                             G_PARAM_READWRITE
        );

    obj_properties[PROP_CAN_FOCUS] =
        g_param_spec_boolean("can-focus",
                             "Can Focus SngWidget flag",
                             "Can Focus SngWidget flag",
                             TRUE,
                             G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );

    signals[SIG_REALIZE] =
        g_signal_newv("realize",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
        );

    signals[SIG_DRAW] =
        g_signal_newv("draw",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
        );

    signals[SIG_MAP] =
        g_signal_newv("map",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
        );

    signals[SIG_KEY_PRESSED] =
        g_signal_newv("key-pressed",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
        );

    signals[SIG_CLICKED] =
        g_signal_newv("clicked",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
        );

    signals[SIG_LOSE_FOCUS] =
        g_signal_newv("lose-focus",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
        );

    signals[SIG_GRAB_FOCUS] =
        g_signal_newv("grab-focus",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
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
sng_widget_init(SngWidget *self)
{
    SngWidgetPrivate *priv = sng_widget_get_instance_private(self);
    // Initialize window position
    priv->x = priv->y = 0;
}
