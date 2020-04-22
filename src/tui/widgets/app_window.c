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
#include <panel.h>
#include "glib-extra/glist.h"
#include "glib-extra/glib_enum_types.h"
#include "tui/theme.h"
#include "tui/widgets/window.h"
#include "tui/keybinding.h"
#include "tui/widgets/app_window.h"

enum
{
    PROP_WINDOW_TYPE = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
    //! Window type
    SngAppWindowType type;
    //! Flag this panel as redraw required
    gboolean changed;
} SngAppWindowPrivate;

// Window class definition
G_DEFINE_TYPE_WITH_PRIVATE(SngAppWindow, sng_app_window, SNG_TYPE_WINDOW)

SngAppWindow *
sng_app_window_new(gint height, gint width)
{
    SngAppWindow *window = g_object_new(
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
sng_app_window_set_window_type(SngAppWindow *window, SngAppWindowType type)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(window);
    priv->type = type;
}

guint
sng_app_window_get_window_type(SngAppWindow *window)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(window);
    return priv->type;
}

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
sng_app_window_set_title(SngAppWindow *window, const gchar *title)
{
    WINDOW *win = sng_widget_get_ncurses_window(SNG_WIDGET(window));

    // FIXME Reverse colors on monochrome terminals
    if (!has_colors()) {
        wattron(win, A_REVERSE);
    }

    // Center the title on the window
    wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_BLUE));
    sng_app_window_clear_line(window, 0);
    mvwprintw(win, 0, (sng_widget_get_width(SNG_WIDGET(window)) - strlen(title)) / 2, "%s", title);
    wattroff(win, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_BLUE));
}

void
sng_app_window_clear_line(SngAppWindow *window, gint line)
{
    // We could do this with wcleartoel but we want to
    // preserve previous window attributes. That way we
    // can set the background of the line.
    WINDOW *win = sng_widget_get_ncurses_window(SNG_WIDGET(window));
    mvwprintw(win, line, 0, "%*s", sng_widget_get_width(SNG_WIDGET(window)), "");
}

void
sng_app_window_draw_bindings(SngAppWindow *window, const char **keybindings, gint count)
{
    int key, xpos = 0;

    WINDOW *win = sng_widget_get_ncurses_window(SNG_WIDGET(window));

    // Reverse colors on monochrome terminals
    if (!has_colors()) {
        wattron(win, A_REVERSE);
    }

    // Write a line all the footer width
    wattron(win, COLOR_PAIR(CP_DEF_ON_CYAN));
    sng_app_window_clear_line(window, sng_widget_get_height(SNG_WIDGET(window)) - 1);

    // Draw keys and their actions
    for (key = 0; key < count; key += 2) {
        wattron(win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        mvwprintw(win, sng_widget_get_height(SNG_WIDGET(window)) - 1, xpos, "%-*s",
                  strlen(keybindings[key]) + 1, keybindings[key]);
        xpos += strlen(keybindings[key]) + 1;
        wattroff(win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        mvwprintw(win, sng_widget_get_height(SNG_WIDGET(window)) - 1, xpos, "%-*s",
                  strlen(keybindings[key + 1]) + 1, keybindings[key + 1]);
        wattroff(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        xpos += strlen(keybindings[key + 1]) + 3;
    }

    // Disable reverse mode in all cases
    wattroff(win, A_REVERSE | A_BOLD);
}

static void
sng_app_window_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(SNG_APP_WINDOW(object));
    switch (property_id) {
        case PROP_WINDOW_TYPE:
            priv->type = g_value_get_enum(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_app_window_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(SNG_APP_WINDOW(object));
    switch (property_id) {
        case PROP_WINDOW_TYPE:
            g_value_set_enum(value, priv->type);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_app_window_class_init(SngAppWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = sng_app_window_set_property;
    object_class->get_property = sng_app_window_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->map = sng_app_window_map;

    obj_properties[PROP_WINDOW_TYPE] =
        g_param_spec_enum("window-type",
                          "Window Type",
                          "Window type ",
                          SNG_TYPE_APP_WINDOW_TYPE,
                          0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}

static void
sng_app_window_init(SngAppWindow *self)
{
    SngAppWindowPrivate *priv = sng_app_window_get_instance_private(self);
    // Force draw on new created windows
    priv->changed = TRUE;
}
