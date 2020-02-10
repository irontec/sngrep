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
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <panel.h>
#include "ncurses/window.h"
#include "ncurses/theme.h"

enum
{
    PROP_WINDOW_HEIGHT = 1,
    PROP_WINDOW_WIDTH,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
    //! Curses panel pointer
    PANEL *panel;
    //! Window for the curses panel
    WINDOW *win;
    //! Height of the curses window
    gint height;
    //! Width of the curses window
    gint width;
    //! Vertical starting position of the window
    gint x;
    //! Horizontal starting position of the window
    gint y;
    //! Panel Type @see PanelTypes enum
    WindowType type;
    //! Flag this panel as redraw required
    gboolean changed;
} WindowPrivate;

// Window class definition
G_DEFINE_TYPE_WITH_PRIVATE(Window, window, G_TYPE_OBJECT)

Window *
window_new(gint height, gint width)
{
    Window *window = g_object_new(
        NCURSES_TYPE_WINDOW,
        "window-height", height,
        "window-width", width,
        NULL
    );

    return window;
}

void
window_free(Window *window)
{
    g_object_unref(window);
}

PANEL *
window_get_ncurses_panel(Window *window)
{
    WindowPrivate *priv = window_get_instance_private(window);
    return priv->panel;
}

WINDOW *
window_get_ncurses_window(Window *window)
{
    WindowPrivate *priv = window_get_instance_private(window);
    return priv->win;
}

void
window_set_window_type(Window *window, WindowType type)
{
    WindowPrivate *priv = window_get_instance_private(window);
    priv->type = type;
}

guint
window_get_window_type(Window *window)
{
    WindowPrivate *priv = window_get_instance_private(window);
    return priv->type;
}

void
window_set_width(Window *window, gint width)
{
    WindowPrivate *priv = window_get_instance_private(window);
    priv->width = width;
}

gint
window_get_width(Window *window)
{
    WindowPrivate *priv = window_get_instance_private(window);
    return priv->width;
}

void
window_set_height(Window *window, gint height)
{
    WindowPrivate *priv = window_get_instance_private(window);
    priv->height = height;
}

gint
window_get_height(Window *window)
{
    WindowPrivate *priv = window_get_instance_private(window);
    return priv->height;
}

gboolean
window_redraw(Window *window)
{
    g_return_val_if_fail(NCURSES_IS_WINDOW(window), FALSE);

    WindowPrivate *priv = window_get_instance_private(window);
    // If ui has changed, force redraw. Don't even ask.
    if (priv->changed) {
        priv->changed = FALSE;
        return TRUE;
    }

    WindowClass *klass = NCURSES_WINDOW_GET_CLASS(window);
    if (klass->redraw != NULL) {
        return klass->redraw(window);
    }

    return TRUE;
}

int
window_draw(Window *window)
{
    g_return_val_if_fail(NCURSES_IS_WINDOW(window), FALSE);

    WindowClass *klass = NCURSES_WINDOW_GET_CLASS(window);
    if (klass->draw != NULL) {
        return klass->draw(window);
    }

    return 0;
}

int
window_resize(Window *window)
{
    g_return_val_if_fail(NCURSES_IS_WINDOW(window), FALSE);

    WindowClass *klass = NCURSES_WINDOW_GET_CLASS(window);
    if (klass->resize != NULL) {
        return klass->resize(window);
    }

    return 0;
}

void
window_help(Window *window)
{
    g_return_if_fail(NCURSES_IS_WINDOW(window));

    // Disable input timeout
    nocbreak();
    cbreak();

    WindowClass *klass = NCURSES_WINDOW_GET_CLASS(window);
    if (klass->help != NULL) {
        klass->help(window);
    }
}

gint
window_handle_key(Window *window, gint key)
{
    gint hld = KEY_NOT_HANDLED;

    WindowClass *klass = NCURSES_WINDOW_GET_CLASS(window);
    if (klass->handle_key != NULL) {
        hld = klass->handle_key(window, key);
    }

    // Force redraw when the user presses keys
    WindowPrivate *priv = window_get_instance_private(NCURSES_WINDOW(window));
    priv->changed = TRUE;
    return hld;
}

void
window_set_title(Window *window, const gchar *title)
{
    WindowPrivate *priv = window_get_instance_private(window);

    // FIXME Reverse colors on monochrome terminals
    if (!has_colors()) {
        wattron(priv->win, A_REVERSE);
    }

    // Center the title on the window
    wattron(priv->win, A_BOLD | COLOR_PAIR(CP_DEF_ON_BLUE));
    window_clear_line(window, 0);
    mvwprintw(priv->win, 0, (priv->width - strlen(title)) / 2, "%s", title);
    wattroff(priv->win, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_BLUE));
}

void
window_clear_line(Window *window, gint line)
{
    // We could do this with wcleartoel but we want to
    // preserve previous window attributes. That way we
    // can set the background of the line.
    WindowPrivate *priv = window_get_instance_private(window);
    mvwprintw(priv->win, line, 0, "%*s", priv->width, "");
}

void
window_draw_bindings(Window *window, const char **keybindings, gint count)
{
    int key, xpos = 0;

    WindowPrivate *priv = window_get_instance_private(window);

    // Reverse colors on monochrome terminals
    if (!has_colors()) {
        wattron(priv->win, A_REVERSE);
    }

    // Write a line all the footer width
    wattron(priv->win, COLOR_PAIR(CP_DEF_ON_CYAN));
    window_clear_line(window, priv->height - 1);

    // Draw keys and their actions
    for (key = 0; key < count; key += 2) {
        wattron(priv->win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        mvwprintw(priv->win, priv->height - 1, xpos, "%-*s",
                  strlen(keybindings[key]) + 1, keybindings[key]);
        xpos += strlen(keybindings[key]) + 1;
        wattroff(priv->win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        wattron(priv->win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        mvwprintw(priv->win, priv->height - 1, xpos, "%-*s",
                  strlen(keybindings[key + 1]) + 1, keybindings[key + 1]);
        wattroff(priv->win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        xpos += strlen(keybindings[key + 1]) + 3;
    }

    // Disable reverse mode in all cases
    wattroff(priv->win, A_REVERSE | A_BOLD);
}


static gint
window_base_draw(Window *window)
{
    WindowPrivate *priv = window_get_instance_private(window);
    touchwin(priv->win);
    return 0;
}

static void
window_constructed(GObject *object)
{
    WindowPrivate *priv = window_get_instance_private(NCURSES_WINDOW(object));

    // Get current screen dimensions
    gint maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);

    // If panel doesn't fill the screen center it
    if (priv->height != maxy) priv->x = abs((maxy - priv->height) / 2);
    if (priv->width != maxx) priv->y = abs((maxx - priv->width) / 2);

    priv->win = newwin(priv->height, priv->width, priv->x, priv->y);
    wtimeout(priv->win, 0);
    keypad(priv->win, TRUE);

    priv->panel = new_panel(priv->win);

    /* update the object state depending on constructor properties */
    G_OBJECT_CLASS (window_parent_class)->constructed(object);
}

static void
window_finalize(GObject *self)
{
    WindowPrivate *priv = window_get_instance_private(NCURSES_WINDOW(self));
    // Hide the window
    hide_panel(priv->panel);

    // Deallocate ncurses pointers
    delwin(priv->win);
    del_panel(priv->panel);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(window_parent_class)->finalize(self);
}

static void
window_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    WindowPrivate *priv = window_get_instance_private(NCURSES_WINDOW(self));

    switch (property_id) {
        case PROP_WINDOW_HEIGHT:
            priv->height = g_value_get_int(value);
            break;
        case PROP_WINDOW_WIDTH:
            priv->width = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
window_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    WindowPrivate *priv = window_get_instance_private(NCURSES_WINDOW(self));

    switch (property_id) {
        case PROP_WINDOW_HEIGHT:
            g_value_set_int(value, priv->height);
            break;
        case PROP_WINDOW_WIDTH:
            g_value_set_int(value, priv->width);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
window_class_init(WindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = window_constructed;
    object_class->finalize = window_finalize;
    object_class->set_property = window_set_property;
    object_class->get_property = window_get_property;

    klass->draw = window_base_draw;

    obj_properties[PROP_WINDOW_HEIGHT] =
        g_param_spec_int("height",
                         "Window Height",
                         "Initial window height",
                         0,
                         getmaxy(stdscr),
                         getmaxy(stdscr),
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_WINDOW_WIDTH] =
        g_param_spec_int("width",
                         "Window Width",
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
window_init(Window *self)
{
    WindowPrivate *priv = window_get_instance_private(self);
    priv->x = priv->y = 0;
}
