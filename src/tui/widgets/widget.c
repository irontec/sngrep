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
    SIG_REALIZE,
    SIG_DRAW,
    SIG_MAP,
    SIG_KEY_PRESSED,
    SIG_CLICKED,
    SIG_DESTROY,
    SIGS
};


enum
{
    PROP_HEIGHT = 1,
    PROP_WIDTH,
    PROP_MIN_HEIGHT,
    PROP_MIN_WIDTH,
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
    Widget *parent;
    //! Window for drawing this widget
    WINDOW *win;
    //! Dimensions of this widget
    gint height, width;
    //! Minimum Dimensions of this widget
    gint min_height, min_width;
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
widget_set_parent(Widget *widget, Widget *parent)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->parent = parent;
}

Widget *
widget_get_parent(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->parent;
}

Widget *
widget_get_toplevel(Widget *widget)
{
    while (widget_get_parent(widget) != NULL) {
        widget = widget_get_parent(widget);
    }
    return widget;
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

gboolean
widget_is_realized(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->win != NULL;
}

gboolean
widget_can_focus(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->can_focus;
}

gboolean
widget_has_focus(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->focused;
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
widget_set_min_size(Widget *widget, gint width, gint height)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->min_width = width;
    priv->min_height = height;
}

void
widget_set_min_width(Widget *widget, gint width)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->min_width = width;
}

gint
widget_get_min_width(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->min_width;
}

void
widget_set_min_height(Widget *widget, gint height)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->min_height = height;
}

gint
widget_get_min_height(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->min_height;
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

void
widget_set_floating(Widget *widget, gboolean floating)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->floating = floating;
}

gboolean
widget_is_floating(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    return priv->floating;
}

void
widget_realize(Widget *widget)
{
    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);

    if (klass->realize != NULL) {
        klass->realize(widget);
    }

    // Notify everyone we're being realized
    g_signal_emit(widget, signals[SIG_REALIZE], 0);
}

gint
widget_draw(Widget *widget)
{
    // Only for visible widgets
    if (!widget_is_visible(widget)) {
        return 0;
    }

    // Realize widget before drawing
    if (!widget_is_realized(widget)) {
        widget_realize(widget);
    }

    // Notify everyone we're being drawn
    g_signal_emit(widget, signals[SIG_DRAW], 0);

    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);
    if (klass->draw != NULL) {
        return klass->draw(widget);
    }

    return 0;
}

void
widget_map(Widget *widget)
{
    // Only for visible widgets
    if (!widget_is_visible(widget)) {
        return;
    }

    // Notify everyone we're being mapped
    g_signal_emit(widget, signals[SIG_MAP], 0);

    WidgetClass *klass = TUI_WIDGET_GET_CLASS(widget);
    if (klass->map != NULL) {
        klass->map(widget);
    }
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

    // Notify everyone we're being clicked
    g_signal_emit(widget, signals[SIG_CLICKED], 0);

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

    // Notify everyone we've received a new key
    g_signal_emit(widget, signals[SIG_KEY_PRESSED], 0);

    return hld;
}

static void
widget_base_realize(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);

    // Create widget window with requested dimensions
    if (priv->win == NULL) {
        priv->win = newpad(priv->height, priv->width);
    } else {
        wresize(priv->win, priv->height, priv->width);
    }
}

static int
widget_base_draw(G_GNUC_UNUSED Widget *widget)
{
    return 0;
}

static void
widget_base_map(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    Widget *parent = widget_get_parent(widget);
    if (priv->floating) {
        parent = widget_get_toplevel(widget);
    }

    // Topmost widget, just refresh its window
    if (parent == NULL) {
        touchwin(priv->win);
        return;
    }

    // Set copywin parameters
    WINDOW *srcwin = priv->win;
    WINDOW *dstwin = widget_get_ncurses_window(parent);
    gint sminrow = 0, smincol = 0;
    gint dminrow = priv->y - widget_get_ypos(parent);
    gint dmincol = priv->x - widget_get_xpos(parent);
    gint dmaxrow = dminrow + priv->height - 1;
    gint dmaxcol = dmincol + priv->width - 1;

    // Copy the widget in its parent widget ncurses window
    copywin(
        srcwin, dstwin,
        sminrow, smincol,
        dminrow, dmincol, dmaxrow, dmaxcol,
        FALSE
    );
}

static gboolean
widget_base_focus_gained(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->focused = TRUE;
    return priv->focused;
}

static void
widget_base_focus_lost(Widget *widget)
{
    WidgetPrivate *priv = widget_get_instance_private(widget);
    priv->focused = FALSE;
}

static gint
widget_base_key_pressed(Widget *widget, gint key)
{
    // Pass key to parent widget
    Widget *parent = widget_get_parent(widget);
    if (parent != NULL) {
        return widget_key_pressed(parent, key);
    }

    // No widget handled this key
    return KEY_NOT_HANDLED;
}

static void
widget_constructed(G_GNUC_UNUSED GObject *object)
{
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
        case PROP_MIN_HEIGHT:
            priv->min_height = g_value_get_int(value);
            break;
        case PROP_MIN_WIDTH:
            priv->min_width = g_value_get_int(value);
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
        case PROP_MIN_HEIGHT:
            g_value_set_int(value, priv->min_height);
            break;
        case PROP_MIN_WIDTH:
            g_value_set_int(value, priv->min_width);
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
widget_class_init(WidgetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = widget_constructed;
    object_class->dispose = widget_dispose;
    object_class->finalize = widget_finalize;
    object_class->set_property = widget_set_property;
    object_class->get_property = widget_get_property;

    klass->realize = widget_base_realize;
    klass->map = widget_base_map;
    klass->draw = widget_base_draw;
    klass->focus_gained = widget_base_focus_gained;
    klass->focus_lost = widget_base_focus_lost;
    klass->key_pressed = widget_base_key_pressed;

    obj_properties[PROP_HEIGHT] =
        g_param_spec_int("height",
                         "Widget Height",
                         "Widget height",
                         0,
                         getmaxy(stdscr),
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_WIDTH] =
        g_param_spec_int("width",
                         "Widget Width",
                         "Widget Width",
                         0,
                         getmaxx(stdscr),
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );
    obj_properties[PROP_MIN_HEIGHT] =
        g_param_spec_int("min-height",
                         "Widget Min Height",
                         "Widget Min height",
                         0,
                         getmaxy(stdscr),
                         0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_MIN_WIDTH] =
        g_param_spec_int("min-width",
                         "Widget Min Width",
                         "Widget Min Width",
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
                             "Floating Widget flag",
                             "Floating Widget flag",
                             FALSE,
                             G_PARAM_READWRITE
        );

    obj_properties[PROP_VISIBLE] =
        g_param_spec_boolean("visible",
                             "Visible Widget flag",
                             "Visible Widget flag",
                             FALSE,
                             G_PARAM_READWRITE
        );

    obj_properties[PROP_CAN_FOCUS] =
        g_param_spec_boolean("can-focus",
                             "Can Focus Widget flag",
                             "Can Focus Widget flag",
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
}
