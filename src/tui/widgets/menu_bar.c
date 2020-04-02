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
 * @file menu_bar.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in menu_bar.h
 */

#include "config.h"
#include "glib-extra/glib.h"
#include "tui/tui.h"
#include "tui/theme.h"
#include "tui/widgets/menu_bar.h"

// Menu class definition
G_DEFINE_TYPE(MenuBar, menu_bar, TUI_TYPE_CONTAINER)

static gint
menu_bar_clicked(Widget *widget, MEVENT mevent)
{
    MenuBar *menu_bar = TUI_MENU_BAR(widget);
    GList *menus = container_get_children(TUI_CONTAINER(widget));

    gint index = mevent.x / MENU_WIDTH;
    if (index < (gint) g_list_length(menus)) {

        menu_bar->selected = index;

        Widget *menu = container_get_child(
            TUI_CONTAINER(widget),
            menu_bar->selected
        );
        widget_show(menu);

        window_set_focused_widget(
            TUI_WINDOW(widget_get_parent(widget)),
            menu
        );
    } else {
        menu_bar->selected = -1;
    }

    return 0;
}

static gint
menu_bar_draw(Widget *widget)
{
    WINDOW *win = widget_get_ncurses_window(widget_get_parent(widget));
    wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
    window_clear_line(TUI_WINDOW(widget_get_parent(widget)), 0);

    // MenuBar is always one line height
    widget_set_size(widget, widget_get_width(widget_get_parent(widget)), 1);

    // Horizontal position for each menu
    gint xpos = 0;

    GList *menus = container_get_children(TUI_CONTAINER(widget));
    for (GList *l = menus; l != NULL; l = l->next) {
        Widget *menu = l->data;
        widget_set_position(menu, xpos, 1);
        xpos += MENU_WIDTH;

        if (widget_is_visible(menu)) {
            wattron(win, COLOR_PAIR(CP_WHITE_ON_DEF));
        } else {
            wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        }
        mvwprintw(win, 0, widget_get_xpos(menu), " %-*s", MENU_WIDTH, menu_get_title(TUI_MENU(menu)));
        wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        mvwaddwstr(win, 0, widget_get_xpos(menu) - 1, tui_acs_utf8(ACS_BOARD));
    }

    // Chain up parent draw
    TUI_WIDGET_CLASS(menu_bar_parent_class)->draw(widget);
    return 0;
}

static gint
menu_bar_key_pressed(Widget *widget, gint key)
{
    MenuBar *menu_bar = TUI_MENU_BAR(widget);
    GList *menus = container_get_children(TUI_CONTAINER(widget));

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RIGHT:
                menu_bar->selected = CLAMP(
                    menu_bar->selected + 1,
                    0,
                    (gint) g_list_length(menus) - 1
                );
                break;
            case ACTION_LEFT:
                menu_bar->selected = CLAMP(
                    menu_bar->selected - 1,
                    0,
                    (gint) g_list_length(menus) - 1
                );
                break;
            default:
                continue;
        }
        break;
    }

    Widget *menu = container_get_child(TUI_CONTAINER(widget), menu_bar->selected);
    widget_show(menu);

    window_set_focused_widget(
        TUI_WINDOW(widget_get_parent(widget)),
        menu
    );

    return KEY_HANDLED;
}

Widget *
menu_bar_new()
{
    return g_object_new(
        TUI_TYPE_MENU_BAR,
        NULL
    );
}

void
menu_bar_free(MenuBar *bar)
{
    g_object_unref(bar);
}

static void
menu_bar_constructed(GObject *object)
{
    Widget *widget = TUI_WIDGET(object);

    // MenuBar is always displayed in the topmost part of the screen
    widget_set_position(widget, 0, 0);
    // MenuBar is always visible
    widget_show(widget);

    // update the object state depending on constructor properties
    G_OBJECT_CLASS(menu_bar_parent_class)->constructed(object);
}


static void
menu_bar_init(MenuBar *self)
{
    self->selected = -1;
}

static void
menu_bar_class_init(MenuBarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = menu_bar_constructed;

    WidgetClass *widget_class = TUI_WIDGET_CLASS(klass);
    widget_class->draw = menu_bar_draw;
    widget_class->key_pressed = menu_bar_key_pressed;
    widget_class->clicked = menu_bar_clicked;
}
