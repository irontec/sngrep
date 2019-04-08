/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
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
#include "ncurses/window.h"
#include "ncurses/theme.h"

Window *
window_create(Window *window)
{
    // If ui has no panel
    if (!window->panel) {
        // Create the new panel for this ui
        if (window->create) {
            window->create(window);
        }
    }

    // Force screen draw for the first time
    window->changed = TRUE;

    // And return it
    return window;
}

void
window_destroy(Window *window)
{
    // If there is no ui panel, we're done
    if (!window || !window->panel)
        return;

    // Hide this panel before destroying
    hide_panel(window->panel);

    // If panel has a destructor function use it
    if (window->destroy) {
        window->destroy(window);
    }

    // Initialize panel pointer
    window->panel = NULL;
}

gboolean
window_redraw(Window *window)
{
    // Sanity check, this should not happen
    if (!window || !window->panel)
        return FALSE;

    // If ui has changed, force redraw. Don't even ask.
    if (window->changed) {
        window->changed = FALSE;
        return TRUE;
    }

    // Query the panel if its needs to be redrawn
    if (window->redraw) {
        return window->redraw(window);
    }
    return TRUE;
}

int
window_draw(Window *window)
{
    //! Sanity check, this should not happen
    if (!window || !window->panel)
        return -1;

    // Request the panel to draw on the scren
    if (window->draw) {
        return window->draw(window);
    } else {
        touchwin(window->win);
    }

    return 0;
}

int
window_resize(Window *window)
{
    //! Sanity check, this should not happen
    if (!window)
        return -1;

    // Notify the panel screen size has changed
    if (window->resize) {
        return window->resize(window);
    }

    return 0;
}

void
window_help(Window *window)
{
    // Disable input timeout
    nocbreak();
    cbreak();

    // If current window has help function
    if (window->help) {
        window->help(window);
    }
}

int
window_handle_key(Window *window, int key)
{
    int hld = KEY_NOT_HANDLED;
    // Request the panel to handle the key
    if (window->handle_key) {
        hld = window->handle_key(window, key);
    }
    // Force redraw when the user presses keys
    window->changed = TRUE;
    return hld;
}


void
window_init(Window *window, int height, int width)
{
    window->width = width;
    window->height = height;
    window->x = window->y = 0;

    // Get current screen dimensions
    gint maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);

    // If panel doesn't fill the screen center it
    if (window->height != maxy) window->x = abs((maxy - height) / 2);
    if (window->width != maxx) window->y = abs((maxx - width) / 2);

    window->win = newwin(height, width, window->x, window->y);
    wtimeout(window->win, 0);
    keypad(window->win, TRUE);

    window->panel = new_panel(window->win);
}

void
window_deinit(Window *window)
{
    // Deallocate panel window
    delwin(window->win);
    // Deallocate panel pointer
    del_panel(window->panel);
}

void
window_set_title(Window *window, const gchar *title)
{
    // FIXME Reverse colors on monochrome terminals
    if (!has_colors()) {
        wattron(window->win, A_REVERSE);
    }

    // Center the title on the window
    wattron(window->win, A_BOLD | COLOR_PAIR(CP_DEF_ON_BLUE));
    window_clear_line(window, 0);
    mvwprintw(window->win, 0, (window->width - strlen(title)) / 2, "%s", title);
    wattroff(window->win, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_BLUE));
}

void
window_clear_line(Window *window, int line)
{
    // We could do this with wcleartoel but we want to
    // preserve previous window attributes. That way we
    // can set the background of the line.
    mvwprintw(window->win, line, 0, "%*s", window->width, "");
}

void
window_draw_bindings(Window *window, const char **keybindings, int count)
{
    int key, xpos = 0;

    // Reverse colors on monochrome terminals
    if (!has_colors()) {
        wattron(window->win, A_REVERSE);
    }

    // Write a line all the footer width
    wattron(window->win, COLOR_PAIR(CP_DEF_ON_CYAN));
    window_clear_line(window, window->height - 1);

    // Draw keys and their actions
    for (key = 0; key < count; key += 2) {
        wattron(window->win, A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        mvwprintw(window->win, window->height - 1, xpos, "%-*s",
                  strlen(keybindings[key]) + 1, keybindings[key]);
        xpos += strlen(keybindings[key]) + 1;
        wattroff(window->win,A_BOLD | COLOR_PAIR(CP_WHITE_ON_CYAN));
        wattron(window->win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        mvwprintw(window->win, window->height - 1, xpos, "%-*s",
                  strlen(keybindings[key + 1]) + 1, keybindings[key + 1]);
        wattroff(window->win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        xpos += strlen(keybindings[key + 1]) + 3;
    }

    // Disable reverse mode in all cases
    wattroff(window->win, A_REVERSE | A_BOLD);
}
