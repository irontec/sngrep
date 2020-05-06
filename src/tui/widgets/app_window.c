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
 * @file app_window.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in app_window.h
 */

#include "config.h"
#include <string.h>
#include "glib-extra/glist.h"
#include "glib-extra/glib_enum_types.h"
#include "tui/theme.h"
#include "tui/widgets/menu_bar.h"
#include "tui/widgets/window.h"
#include "tui/keybinding.h"
#include "tui/widgets/app_window.h"

typedef struct
{
    //! Flag this panel as redraw required
    gboolean changed;
    //! Window Top menu bar
    SngWidget *menu_bar;
    //! Window content
    SngWidget *content_box;
    //! Window Bottom button bar
    SngWidget *button_bar;
} SngAppWindowPrivate;

// Window class definition
G_DEFINE_TYPE_WITH_PRIVATE(SngAppWindow, sng_app_window, SNG_TYPE_WINDOW)

gboolean
sng_app_window_redraw(SngAppWindow *app_window)
{
    g_return_val_if_fail(SNG_IS_WINDOW(app_window), FALSE);

    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(app_window);
    // If ui has changed, force redraw. Don't even ask.
    if (priv->changed) {
        priv->changed = FALSE;
        return TRUE;
    }

    SngAppWindowClass *klass = SNG_APP_WINDOW_GET_CLASS(app_window);
    if (klass->redraw != NULL) {
        return klass->redraw(app_window);
    }

    return TRUE;
}

static void
sng_app_window_map_floating_child(SngWidget *widget, gpointer data)
{
    if (SNG_IS_CONTAINER(widget)) {
        GList *children = sng_container_get_children(SNG_CONTAINER(widget));
        for (GList *l = children; l != NULL; l = l->next) {
            sng_app_window_map_floating_child(l->data, data);
        }
    }

    if (sng_widget_is_floating(widget)) {
        sng_widget_map(widget);
    }
}

static void
sng_app_window_map_floating(SngAppWindow *window)
{
    sng_container_foreach(SNG_CONTAINER(window), (GFunc) sng_app_window_map_floating_child, NULL);
}

static void
sng_app_window_map(SngWidget *widget)
{
    SNG_WIDGET_CLASS(sng_app_window_parent_class)->map(widget);
    // Map all floating widgets
    sng_app_window_map_floating(SNG_APP_WINDOW(widget));
}

int
sng_app_window_resize(SngAppWindow *window)
{
    g_return_val_if_fail(SNG_IS_WINDOW(window), FALSE);

    SngAppWindowClass *klass = SNG_APP_WINDOW_GET_CLASS(window);
    if (klass->resize != NULL) {
        return klass->resize(window);
    }

    return 0;
}

void
sng_app_window_help(SngAppWindow *window)
{
    g_return_if_fail(SNG_IS_APP_WINDOW(window));

    // Disable input timeout
    nocbreak();
    cbreak();

    SngAppWindowClass *klass = SNG_APP_WINDOW_GET_CLASS(window);
    if (klass->help != NULL) {
        klass->help(window);
    }
}

void
sng_app_window_add_menu(SngAppWindow *window, SngMenu *menu)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(window);
    sng_container_add(SNG_CONTAINER(priv->menu_bar), SNG_WIDGET(menu));
}

void
sng_app_window_add_button(SngAppWindow *window, const gchar *label, SngAction action)
{
    // Build button label
    g_autoptr(GString) text = g_string_new(NULL);
    g_string_printf(text, "<%d>%s <%d>%s",
                    COLOR_PAIR(CP_WHITE_ON_CYAN) | A_BOLD, key_action_key_str(action),
                    COLOR_PAIR(CP_BLACK_ON_CYAN), label);

    // Create new button
    SngWidget *button = sng_button_new(text->str);

    // Add button to bar
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(window);
    sng_box_pack_start(SNG_BOX(priv->button_bar), button);

    // Connect button activate signal
    g_signal_connect(button, "activate",
                     G_CALLBACK(sng_window_handle_action),
                     GINT_TO_POINTER(action));
}

SngWidget *
sng_app_window_get_content(SngAppWindow *app_window)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(app_window);
    return priv->content_box;
}

static void
sng_app_window_contructed(GObject *object)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(SNG_APP_WINDOW(object));

    // Add boxes to main app window
    sng_box_pack_start(SNG_BOX(object), priv->menu_bar);
    sng_container_add(SNG_CONTAINER(object), priv->content_box);
    sng_box_pack_start(SNG_BOX(object), priv->button_bar);

    // Default application window bindings
    sng_widget_bind_action(SNG_WIDGET(object), ACTION_SHOW_HELP,
                           G_CALLBACK(sng_app_window_help), NULL);
}

static void
sng_app_window_class_init(SngAppWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = sng_app_window_contructed;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->map = sng_app_window_map;
}

static void
sng_app_window_init(SngAppWindow *self)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(self);
    // Force draw on new created windows
    priv->changed = TRUE;

    // Initialize menu bar
    priv->menu_bar = sng_menu_bar_new();

    // Initialize main window content box
    priv->content_box = sng_box_new(SNG_ORIENTATION_VERTICAL);

    // Initialize button bar
    priv->button_bar = sng_box_new_full(SNG_ORIENTATION_HORIZONTAL, 3, 0);
    sng_widget_set_vexpand(priv->button_bar, FALSE);
    sng_widget_set_height(priv->button_bar, 1);
    sng_box_set_background(SNG_BOX(priv->button_bar), COLOR_PAIR(CP_WHITE_ON_CYAN));
}
