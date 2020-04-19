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
#include <string.h>
#include <panel.h>
#include "glib-extra/glist.h"
#include "glib-extra/glib_enum_types.h"
#include "tui/widgets/window.h"
#include "tui/keybinding.h"

typedef struct
{
    //! Curses panel pointer
    PANEL *panel;
    //! Focusable widget chain
    GList *focus_chain;
    //! Default focus widget
    SngWidget *focus_default;
    //! Current focused widget
    SngWidget *focus;
} SngWindowPrivate;

// Window class definition
G_DEFINE_TYPE_WITH_PRIVATE(SngWindow, sng_window, SNG_TYPE_BOX)

SngWindow *
sng_window_new(gint height, gint width)
{
    SngWindow *window = g_object_new(
        SNG_TYPE_WINDOW,
        "height", height,
        "width", width,
        "vexpand", TRUE,
        "hexpand", TRUE,
        NULL
    );

    return window;
}

PANEL *
sng_window_get_ncurses_panel(SngWindow *window)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    return priv->panel;
}

void
sng_window_set_default_focus(SngWindow *window, SngWidget *widget)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    priv->focus_default = widget;
}

SngWidget *
sng_window_focused_widget(SngWindow *window)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    return priv->focus;
}

void
sng_window_set_focused_widget(SngWindow *window, SngWidget *widget)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);

    // SngWidget already has the focus
    if (priv->focus == widget) {
        return;
    }

    // Remove focus from previous focused widget
    if (priv->focus != NULL) {
        sng_widget_focus_lost(priv->focus);
    }

    // Focus new widget
    priv->focus = widget;
    sng_widget_focus_gain(widget);
}

static void
sng_window_focus_default_widget(SngWindow *window)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    sng_widget_grab_focus(priv->focus_default);
}

void
sng_window_focus_next(SngWindow *window)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    GList *current = g_list_find(priv->focus_chain, priv->focus);
    if (current == NULL) {
        sng_window_set_focused_widget(window, priv->focus_default);
        return;
    }

    do {
        if (current->next == NULL) {
            current = g_list_first(priv->focus_chain);
        } else {
            current = (current->next)
                      ? current->next
                      : g_list_first(priv->focus_chain);
        }
    } while (!sng_widget_is_visible(current->data));

    sng_window_set_focused_widget(window, current->data);
}

void
sng_window_focus_prev(SngWindow *window)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    GList *current = g_list_find(priv->focus_chain, priv->focus);
    do {
        if (current->prev == NULL) {
            current = g_list_last(priv->focus_chain);
        } else {
            current = (current->prev)
                      ? current->prev
                      : g_list_last(priv->focus_chain);
        }
    } while (!sng_widget_is_visible(current->data));

    sng_window_set_focused_widget(window, current->data);
}

static void
sng_window_realize(SngWidget *widget)
{
    // Get current screen dimensions
    gint maxx, maxy, xpos = 0, ypos = 0;
    getmaxyx(stdscr, maxy, maxx);

    gint height = sng_widget_get_height(widget);
    gint width = sng_widget_get_width(widget);

    // If panel doesn't fill the screen center it
    if (height != maxy) {
        ypos = ABS((maxy - height) / 2);
    }
    if (width != maxx) {
        xpos = ABS((maxx - width) / 2);
    }
    sng_widget_set_position(widget, xpos, ypos);

    // Enable keystrokes in ncurses window
    WINDOW *win = newwin(height, width, ypos, xpos);
    sng_widget_set_ncurses_window(widget, win);
    wtimeout(win, 0);
    keypad(win, TRUE);

    // Chain-up parent realize
    SNG_WIDGET_CLASS(sng_window_parent_class)->realize(widget);

    // Create a new panel on top of panel stack
    SngWindow *window = SNG_WINDOW(widget);
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    priv->panel = new_panel(win);
    set_panel_userptr(priv->panel, window);

    // Focus default widget
    sng_window_set_focused_widget(window, priv->focus_default);
}

static void
sng_window_update_focus_chain(SngWindow *window, SngWidget *widget)
{
    if (sng_widget_can_focus(widget)) {
        SngWindowPrivate *priv = sng_window_get_instance_private(window);
        priv->focus_chain = g_list_append(priv->focus_chain, widget);
        g_signal_connect_swapped(widget, "lose-focus",
                                 G_CALLBACK(sng_window_focus_default_widget), window);
        g_signal_connect_swapped(widget, "grab-focus",
                                 G_CALLBACK(sng_window_set_focused_widget), window);
    }

    if (SNG_IS_CONTAINER(widget)) {
        GList *children = sng_container_get_children(SNG_CONTAINER(widget));
        for (GList *l = children; l != NULL; l = l->next) {
            sng_window_update_focus_chain(window, l->data);
        }
    }
}

static void
sng_window_add_widget(SngContainer *container, SngWidget *widget)
{
    // If widget can be focused, add it to the focus-chain list
    sng_window_update_focus_chain(
        SNG_WINDOW(container),
        widget
    );

    // Chain-up parent class add function
    SNG_CONTAINER_CLASS(sng_window_parent_class)->add(container, widget);
}

gint
sng_window_handle_mouse(SngWindow *window, MEVENT mevent)
{
    SngWidget *clicked_widget = sng_container_find_by_position(SNG_CONTAINER(window), mevent.x, mevent.y);
    if (clicked_widget != NULL) {
        sng_window_set_focused_widget(window, clicked_widget);
        return sng_widget_clicked(clicked_widget, mevent);
    }
    return KEY_HANDLED;
}

gint
sng_window_handle_key(SngWindow *window, gint key)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);

    // Check actions for this key
    KeybindingAction action = key_find_action(key, ACTION_UNKNOWN);
    if (action == ACTION_NEXT_FIELD) {
        sng_window_focus_next(window);
        return KEY_HANDLED;
    } else if (action == ACTION_PREV_FIELD) {
        sng_window_focus_prev(window);
        return KEY_HANDLED;
    } else {
        return sng_widget_key_pressed(priv->focus, key);
    }
}

static void
sng_window_constructed(GObject *object)
{
    // Realize window as soon as its constructed
    sng_widget_realize(SNG_WIDGET(object));

    // Chain-up parent constructed
    G_OBJECT_CLASS(sng_window_parent_class)->constructed(object);
}

static void
sng_window_finalize(GObject *self)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(SNG_WINDOW(self));

    // Deallocate ncurses pointers
    hide_panel(priv->panel);
    del_panel(priv->panel);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(sng_window_parent_class)->finalize(self);
}

static void
sng_window_class_init(SngWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = sng_window_constructed;
    object_class->finalize = sng_window_finalize;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->realize = sng_window_realize;

    SngContainerClass *container_class = SNG_CONTAINER_CLASS(klass);
    container_class->add = sng_window_add_widget;
}

static void
sng_window_init(SngWindow *self)
{
    // Set window as visible by default
    sng_widget_show(SNG_WIDGET(self));
}
