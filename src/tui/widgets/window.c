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
#include <ncurses.h>
#include <panel.h>
#include "window.h"
#include "tui/theme.h"

typedef struct
{
    //! Curses panel pointer
    PANEL *panel;
    //! Panel Type @see PanelTypes enum
    WindowType type;
    //! Flag this panel as redraw required
    gboolean changed;
    //! Current focused widget
    Widget *focus;
} WindowPrivate;

// Window class definition
G_DEFINE_TYPE_WITH_PRIVATE(Window, window, TUI_TYPE_WIDGET)

Window *
window_new(gint height, gint width)
{
    Window *window = g_object_new(
        TUI_TYPE_WINDOW,
        "window-height", height,
        "window-width", width,
        NULL
    );

    return window;
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
    return widget_get_ncurses_window(TUI_WIDGET(window));
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
    widget_set_width(TUI_WIDGET(window), width);
}

gint
window_get_width(Window *window)
{
    return widget_get_width(TUI_WIDGET(window));
}

void
window_set_height(Window *window, gint height)
{
    widget_set_height(TUI_WIDGET(window), height);
}

gint
window_get_height(Window *window)
{
    return widget_get_height(TUI_WIDGET(window));
}

Widget *
window_focused_widget(Window *window)
{
    WindowPrivate *priv = window_get_instance_private(window);
    return priv->focus;
}

void
window_set_focused_widget(Window *window, Widget *widget)
{
    WindowPrivate *priv = window_get_instance_private(window);

    // Widget already has the focus
    if (priv->focus == widget) {
        return;
    }

    // Check if the widget wants to be focused
    if (widget_focus_gain(widget)) {
        // Remove focus from previous focused widget
        widget_focus_lost(priv->focus);
        // Set the new focus
        priv->focus = widget;
    }
}

gboolean
window_redraw(Window *window)
{
    g_return_val_if_fail(TUI_IS_WINDOW(window), FALSE);

    WindowPrivate *priv = window_get_instance_private(window);
    // If ui has changed, force redraw. Don't even ask.
    if (priv->changed) {
        priv->changed = FALSE;
        return TRUE;
    }

    WindowClass *klass = TUI_WINDOW_GET_CLASS(window);
    if (klass->redraw != NULL) {
        return klass->redraw(window);
    }

    return TRUE;
}

int
window_draw(Window *window)
{
    g_return_val_if_fail(TUI_IS_WINDOW(window), FALSE);
    WindowPrivate *priv = window_get_instance_private(window);
    priv->changed = TRUE;
    return widget_draw(TUI_WIDGET(window));
}

int
window_resize(Window *window)
{
    g_return_val_if_fail(TUI_IS_WINDOW(window), FALSE);

    WindowClass *klass = TUI_WINDOW_GET_CLASS(window);
    if (klass->resize != NULL) {
        return klass->resize(window);
    }

    return 0;
}

void
window_help(Window *window)
{
    g_return_if_fail(TUI_IS_WINDOW(window));

    // Disable input timeout
    nocbreak();
    cbreak();

    WindowClass *klass = TUI_WINDOW_GET_CLASS(window);
    if (klass->help != NULL) {
        klass->help(window);
    }
}

gint
window_handle_mouse(Window *window, MEVENT mevent)
{
    WindowPrivate *priv = window_get_instance_private(window);
    priv->changed = TRUE;
    Widget *clicked_widget = widget_find_by_position(TUI_WIDGET(window), mevent.x, mevent.y);
    if (clicked_widget != NULL) {
        window_set_focused_widget(window, clicked_widget);
        return widget_clicked(clicked_widget, mevent);
    }
    return KEY_HANDLED;
}

gint
window_handle_key(Window *window, gint key)
{
    WindowPrivate *priv = window_get_instance_private(window);
    priv->changed = TRUE;
    return widget_key_pressed(priv->focus, key);
}

void
window_set_title(Window *window, const gchar *title)
{
    WINDOW *win = widget_get_ncurses_window(TUI_WIDGET(window));

    // FIXME Reverse colors on monochrome terminals
    if (!has_colors()) {
        wattron(win, A_REVERSE);
    }

    // Center the title on the window
    wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_BLUE));
    window_clear_line(window, 0);
    mvwprintw(win, 0, (widget_get_width(TUI_WIDGET(window)) - strlen(title)) / 2, "%s", title);
    wattroff(win, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_BLUE));
}

void
window_clear_line(Window *window, gint line)
{
    // We could do this with wcleartoel but we want to
    // preserve previous window attributes. That way we
    // can set the background of the line.
    WINDOW *win = widget_get_ncurses_window(TUI_WIDGET(window));
    mvwprintw(win, line, 0, "%*s", widget_get_width(TUI_WIDGET(window)), "");
}

void
window_draw_bindings(Window *window, const char **keybindings, gint count)
{
    int key, xpos = 0;

    WINDOW *win = widget_get_ncurses_window(TUI_WIDGET(window));

    // Reverse colors on monochrome terminals
    if (!has_colors()) {
        wattron(win, A_REVERSE);
    }

    // Write a line all the footer width
    wattron(win, COLOR_PAIR(CP_DEF_ON_CYAN));
    window_clear_line(window, widget_get_height(TUI_WIDGET(window)) - 1);

    // Draw keys and their actions
    for (key = 0; key < count; key += 2) {
        wattron(win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        mvwprintw(win, widget_get_height(TUI_WIDGET(window)) - 1, xpos, "%-*s",
                  strlen(keybindings[key]) + 1, keybindings[key]);
        xpos += strlen(keybindings[key]) + 1;
        wattroff(win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        mvwprintw(win, widget_get_height(TUI_WIDGET(window)) - 1, xpos, "%-*s",
                  strlen(keybindings[key + 1]) + 1, keybindings[key + 1]);
        wattroff(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        xpos += strlen(keybindings[key + 1]) + 3;
    }

    // Disable reverse mode in all cases
    wattroff(win, A_REVERSE | A_BOLD);
}

static void
window_constructed(GObject *object)
{
    // Get current screen dimensions
    gint maxx, maxy, xpos = 0, ypos = 0;
    getmaxyx(stdscr, maxy, maxx);

    Widget *widget = TUI_WIDGET(object);
    gint height = widget_get_height(widget);
    gint width = widget_get_width(widget);

    // If panel doesn't fill the screen center it
    if (height != maxy) {
        xpos = ABS((maxy - height) / 2);
    }
    if (width != maxx) {
        ypos = ABS((maxx - width) / 2);
    }
    widget_set_position(widget, xpos, ypos);

    WINDOW *win = newwin(height, width, xpos, ypos);
    wtimeout(win, 0);
    keypad(win, TRUE);

    // Make window widgets visible by default
    widget_set_ncurses_window(widget, win);
    widget_show(widget);

    WindowPrivate *priv = window_get_instance_private(TUI_WINDOW(object));
    priv->panel = new_panel(win);
    top_panel(priv->panel);

    /* update the object state depending on constructor properties */
    G_OBJECT_CLASS(window_parent_class)->constructed(object);
}

static void
window_finalize(GObject *self)
{
    WindowPrivate *priv = window_get_instance_private(TUI_WINDOW(self));
    // Deallocate ncurses pointers
    hide_panel(priv->panel);
    del_panel(priv->panel);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(window_parent_class)->finalize(self);
}

static void
window_class_init(WindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = window_constructed;
    object_class->finalize = window_finalize;
}

static void
window_init(Window *self)
{
    WindowPrivate *priv = window_get_instance_private(self);
    // Force draw on new created windows
    priv->changed = TRUE;
    // Set window as default focused Widget
    priv->focus = TUI_WIDGET(self);
}
