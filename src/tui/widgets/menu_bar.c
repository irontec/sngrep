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
G_DEFINE_TYPE(SngMenuBar, sng_menu_bar, SNG_TYPE_CONTAINER)

static void
sng_menu_bar_clicked(SngWidget *widget, MEVENT mevent)
{
    SngMenuBar *menu_bar = SNG_MENU_BAR(widget);
    GList *menus = sng_container_get_children(SNG_CONTAINER(widget));

    gint index = mevent.x / MENU_WIDTH;
    if (index < (gint) g_list_length(menus)) {

        menu_bar->selected = index;

        SngWidget *menu = sng_container_get_child(
            SNG_CONTAINER(widget),
            menu_bar->selected
        );
        sng_widget_show(menu);
        sng_widget_grab_focus(menu);
    } else {
        menu_bar->selected = -1;
    }
}

static void
sng_menu_bar_realize(SngWidget *widget)
{
    if (!sng_widget_is_realized(widget)) {
        WINDOW *win = newpad(
            sng_widget_get_height(widget),
            sng_widget_get_width(widget)
        );
        sng_widget_set_ncurses_window(widget, win);

    }
    SNG_WIDGET_CLASS(sng_menu_bar_parent_class)->realize(widget);
}

static void
sng_menu_bar_draw(SngWidget *widget)
{
    // Create a window to draw the menu bar
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wbkgd(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
    werase(win);

    GList *children = sng_container_get_children(SNG_CONTAINER(widget));
    for (GList *l = children; l != NULL; l = l->next) {
        SngWidget *menu = l->data;
        sng_widget_set_position(menu, getcurx(win), 1);

        if (sng_widget_is_visible(menu)) {
            wattron(win, COLOR_PAIR(CP_WHITE_ON_DEF));
        } else {
            wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        }
        wprintw(win, " %-*s", MENU_WIDTH, sng_menu_get_title(SNG_MENU(menu)));
        wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
        waddwstr(win, tui_acs_utf8(ACS_BOARD));
    }

    // Chain up parent draw
    SNG_WIDGET_CLASS(sng_menu_bar_parent_class)->draw(widget);
}

static void
sng_menu_bar_key_pressed(SngWidget *widget, gint key)
{
    SngMenuBar *menu_bar = SNG_MENU_BAR(widget);
    GList *children = sng_container_get_children(SNG_CONTAINER(widget));

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_RIGHT:
                menu_bar->selected = CLAMP(
                    menu_bar->selected + 1,
                    0,
                    (gint) g_list_length(children) - 1
                );
                break;
            case ACTION_LEFT:
                menu_bar->selected = CLAMP(
                    menu_bar->selected - 1,
                    0,
                    (gint) g_list_length(children) - 1
                );
                break;
            case ACTION_CANCEL:
                sng_widget_lose_focus(widget);
                break;
            default:
                continue;
        }
        break;
    }

    SngWidget *menu = sng_container_get_child(SNG_CONTAINER(widget), menu_bar->selected);
    sng_widget_show(menu);
    sng_widget_grab_focus(menu);
}

static void
sng_menu_bar_focus_gained(SngWidget *widget)
{
    SngMenuBar *menu_bar = SNG_MENU_BAR(widget);

    menu_bar->selected = 0;

    SngWidget *menu = sng_container_get_child(
        SNG_CONTAINER(widget),
        menu_bar->selected
    );
    sng_widget_show(menu);
    sng_widget_grab_focus(menu);
}

SngWidget *
sng_menu_bar_new()
{
    return g_object_new(
        TUI_TYPE_MENU_BAR,
        "height", 1,
        "hexpand", TRUE,
        NULL
    );
}

void
sng_menu_bar_free(SngMenuBar *bar)
{
    g_object_unref(bar);
}

static void
sng_menu_bar_constructed(GObject *object)
{
    SngWidget *widget = SNG_WIDGET(object);
    // MenuBar is always visible
    sng_widget_show(widget);
    // update the object state depending on constructor properties
    G_OBJECT_CLASS(sng_menu_bar_parent_class)->constructed(object);
}


static void
sng_menu_bar_init(SngMenuBar *self)
{
    self->selected = -1;
}

static void
sng_menu_bar_class_init(SngMenuBarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = sng_menu_bar_constructed;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->realize = sng_menu_bar_realize;
    widget_class->draw = sng_menu_bar_draw;
    widget_class->key_pressed = sng_menu_bar_key_pressed;
    widget_class->clicked = sng_menu_bar_clicked;
    widget_class->focus_gained = sng_menu_bar_focus_gained;
}
