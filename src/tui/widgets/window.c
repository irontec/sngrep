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
 */

#include "config.h"
#include <panel.h>
#include "glib-extra/glist.h"
#include "glib-extra/glib_enum_types.h"
#include "tui/keybinding.h"
#include "tui/widgets/button.h"
#include "tui/widgets/label.h"
#include "tui/widgets/separator.h"
#include "tui/widgets/window.h"

enum
{
    PROP_TITLE = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
    //! Curses panel pointer
    PANEL *panel;
    //! Window title
    const gchar *title;
    //! Title label widget
    SngWidget *lb_title;
    //! Button bar
    SngWidget *button_bar;
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

void
sng_window_update(SngWindow *window)
{
    SngWidget *widget = SNG_WIDGET(window);
    // Update all window widgets
    sng_widget_update(widget);
    // Request size of all widgets
    sng_widget_size_request(widget);
    // Create internal ncurses data
    sng_widget_realize(widget);
    // Draw each widget in their internal window
    sng_widget_draw(widget);
    // Map internal windows to visible panel windows
    sng_widget_map(widget);
}

PANEL *
sng_window_get_ncurses_panel(SngWindow *window)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    return priv->panel;
}

void
sng_window_set_title(SngWindow *window, const gchar *title)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    if (title != NULL) {
        priv->title = title;
        priv->lb_title = sng_label_new(title);
        sng_label_set_align(SNG_LABEL(priv->lb_title), SNG_ALIGN_CENTER);
        sng_box_pack_start(SNG_BOX(window), priv->lb_title);
        sng_box_pack_start(SNG_BOX(window), sng_separator_new(SNG_ORIENTATION_HORIZONTAL));
    }
}

const gchar *
sng_window_get_title(SngWindow *window)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    return priv->title;
}

void
sng_window_add_button(SngWindow *window, SngButton *button)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    if (priv->button_bar == NULL) {
        priv->button_bar = sng_box_new(SNG_ORIENTATION_HORIZONTAL);
    }

    // Add button to the bar
    sng_container_add(SNG_CONTAINER(priv->button_bar), SNG_WIDGET(button));
}

void
sng_window_set_default_focus(SngWindow *window, SngWidget *widget)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    priv->focus_default = widget;
    if (priv->focus == NULL) {
        priv->focus = priv->focus_default;
    }
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
sng_window_size_request(SngWidget *widget)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(SNG_WINDOW(widget));

    gint height = sng_widget_get_height(widget);
    gint width = sng_widget_get_width(widget);
    gint maxx = getmaxx(stdscr);
    gint maxy = getmaxy(stdscr);

    // If window doesn't fill the screen
    if (height != maxy || width != maxx) {
        // Center window in screen
        sng_widget_set_position(
            widget,
            ABS((maxx - width) / 2),
            ABS((maxy - height) / 2)
        );
    }

    // Calculate bar padding
    if (priv->button_bar != NULL) {
        SngBoxPadding box_padding = sng_box_get_padding(SNG_BOX(priv->button_bar));
        gint padding = sng_widget_get_width(widget);
        padding -= 5;

        GList *buttons = sng_container_get_children(SNG_CONTAINER(priv->button_bar));
        for (GList *l = buttons; l != NULL; l = l->next) {
            padding -= sng_label_get_text_len(
                sng_label_get_text(SNG_LABEL(l->data))
            );
        }

        // Apply half padding to the left and half to the right
        box_padding.left = box_padding.right = padding / 2;
        sng_box_set_padding(SNG_BOX(priv->button_bar), box_padding);
    }

    // Chain-up parent size request
    SNG_WIDGET_CLASS(sng_window_parent_class)->size_request(widget);
}

static void
sng_window_realize(SngWidget *widget)
{
    SngWindow *window = SNG_WINDOW(widget);
    SngWindowPrivate *priv = sng_window_get_instance_private(window);

    // Only realize once
    if (!sng_widget_is_realized(widget)) {
        // Enable keystrokes in ncurses window
        WINDOW *win = newwin(
            sng_widget_get_height(widget),
            sng_widget_get_width(widget),
            sng_widget_get_ypos(widget),
            sng_widget_get_xpos(widget)
        );

        g_return_if_fail(win != NULL);
        sng_widget_set_ncurses_window(widget, win);
        wtimeout(win, 0);
        keypad(win, TRUE);

        // Create a new panel on top of panel stack
        priv->panel = new_panel(win);
        g_return_if_fail(priv->panel != NULL);
        set_panel_userptr(priv->panel, SNG_WINDOW(widget));

        // Focus default widget
        sng_window_set_focused_widget(window, priv->focus_default);
    }

    // Chain-up parent realize
    SNG_WIDGET_CLASS(sng_window_parent_class)->realize(widget);
}

static void
sng_window_draw(SngWidget *widget)
{
    // Chain up parent draw
    SNG_WIDGET_CLASS(sng_window_parent_class)->draw(widget);

    SngWindow *window = SNG_WINDOW(widget);
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    WINDOW *win = sng_widget_get_ncurses_window(widget);

    gint height = sng_widget_get_height(widget);
    gint width = sng_widget_get_width(widget);

    // Write Horizontal line for title
    if (priv->lb_title != NULL) {
        mvwaddch(win, 2, 0, ACS_LTEE);
        mvwaddch(win, 2, width - 1, ACS_RTEE);
    }

    // Write Horizontal line for Buttons
    if (priv->button_bar != NULL) {
        mvwaddch(win, height - 3, 0, ACS_LTEE);
        mvwaddch(win, height - 3, width - 1, ACS_RTEE);
    }

    sng_widget_draw(priv->focus);
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

void
sng_window_handle_mouse(SngWindow *window, MEVENT mevent)
{
    SngWidget *clicked_widget = sng_container_find_by_position(SNG_CONTAINER(window), mevent.x, mevent.y);
    if (clicked_widget != NULL) {
        sng_window_set_focused_widget(window, clicked_widget);
        sng_widget_clicked(clicked_widget, mevent);
    }
}

void
sng_window_handle_key(SngWindow *window, gint key)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(window);

    // Check actions for this key
    SngAction action = key_find_action(key, ACTION_NONE);
    if (action == ACTION_NEXT_FIELD) {
        sng_window_focus_next(window);
    } else if (action == ACTION_PREV_FIELD) {
        sng_window_focus_prev(window);
    } else {
        sng_widget_key_pressed(priv->focus, key);
    }
}

gboolean
sng_window_handle_action(gpointer widget, gpointer action)
{
    sng_widget_handle_action(
        sng_widget_get_toplevel(SNG_WIDGET(widget)),
        GPOINTER_TO_INT(action)
    );
}

static void
sng_window_constructed(GObject *object)
{
    SngWindowPrivate *priv = sng_window_get_instance_private(SNG_WINDOW(object));

    // Add button bar at the bottom of the screen
    if (priv->button_bar) {
        sng_box_pack_start(SNG_BOX(object), sng_separator_new(SNG_ORIENTATION_HORIZONTAL));
        sng_box_pack_start(SNG_BOX(object), priv->button_bar);
    }

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

    // Deallocate focus chain
    g_list_free(priv->focus_chain);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(sng_window_parent_class)->finalize(self);
}

static void
sng_window_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngWindow *window = SNG_WINDOW(object);
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    switch (property_id) {
        case PROP_TITLE:
            priv->title = g_value_get_string(value);
            sng_window_set_title(window, priv->title);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_window_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngWindow *window = SNG_WINDOW(object);
    SngWindowPrivate *priv = sng_window_get_instance_private(window);
    switch (property_id) {
        case PROP_TITLE:
            g_value_set_string(value, priv->title);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_window_class_init(SngWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = sng_window_constructed;
    object_class->finalize = sng_window_finalize;
    object_class->set_property = sng_window_set_property;
    object_class->get_property = sng_window_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->size_request = sng_window_size_request;
    widget_class->realize = sng_window_realize;
    widget_class->draw = sng_window_draw;

    SngContainerClass *container_class = SNG_CONTAINER_CLASS(klass);
    container_class->add = sng_window_add_widget;

    obj_properties[PROP_TITLE] =
        g_param_spec_string("title",
                            "Window title",
                            "Window title",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}

static void
sng_window_init(G_GNUC_UNUSED SngWindow *window)
{
}
