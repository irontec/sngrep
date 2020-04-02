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
 * @file menu.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in menu.h
 */

#include "config.h"
#include <panel.h>
#include "glib-extra/glib.h"
#include "tui/theme.h"
#include "tui/widgets/window.h"
#include "tui/widgets/menu.h"

enum
{
    PROP_TITLE = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

// Menu class definition
G_DEFINE_TYPE(Menu, menu, TUI_TYPE_CONTAINER)

Widget *
menu_new(const gchar *title)
{
    Widget *widget = g_object_new(
        TUI_TYPE_MENU,
        "title", title,
        NULL
    );
    return widget;
}

void
menu_free(Menu *menu)
{
    g_object_unref(menu);
}

const gchar *
menu_get_title(Menu *menu)
{
    return menu->title;
}

static gint
menu_draw(Widget *widget)
{
    Menu *menu = TUI_MENU(widget);
    GList *items = container_get_children(TUI_CONTAINER(widget));

    gint height = g_list_length(items) + 2;
    gint width = MENU_WIDTH + 2 + 6 + 2;
    for (GList *l = items; l != NULL; l = l->next) {
        MenuItem *item = TUI_MENU_ITEM(l->data);
        if (item->text != NULL) {
            width = MAX(width, (gint) strlen(item->text) + 2 + 6 + 2);
        }
    }

    WINDOW *win = newpad(height, width);
    wbkgd(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
    box(win, 0, 0);

    widget_set_size(widget, width, height);
    widget_set_ncurses_window(widget, win);

    gint i = 0;
    for (GList *l = items; l != NULL; l = l->next, i++) {
        MenuItem *item = TUI_MENU_ITEM(l->data);

        if (item->text != NULL) {
            if (menu->selected == i) {
                wattron(win, COLOR_PAIR(CP_WHITE_ON_DEF));
            }
            mvwprintw(win, (gint) i + 1, 1,
                      "%-*s %-6s",
                      width - 6 - 2 - 1,
                      item->text,
                      (item->action != ACTION_UNKNOWN) ? key_action_key_str(item->action) : ""
            );
        } else {
            mvwhline(win, i + 1, 0, ACS_HLINE, width);
            mvwaddch(win, i + 1, 0, ACS_LTEE);
            mvwaddch(win, i + 1, width - 1, ACS_RTEE);
        }
        wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
    }

    copywin(
        widget_get_ncurses_window(widget),
        widget_get_ncurses_window(widget_get_toplevel(widget)),
        0, 0,
        widget_get_ypos(widget),
        widget_get_xpos(widget),
        getmaxy(win),
        widget_get_xpos(widget) + getmaxx(win) - 1,
        FALSE
    );

    return 0;
}

static gint
menu_key_pressed(Widget *widget, gint key)
{
    Menu *menu = TUI_MENU(widget);
    GList *items = container_get_children(TUI_CONTAINER(widget));

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                menu->selected = CLAMP(
                    menu->selected + 1,
                    0,
                    (gint) g_list_length(items) - 1
                );
                break;
            case ACTION_UP:
                menu->selected = CLAMP(
                    menu->selected - 1,
                    0,
                    (gint) g_list_length(items) - 1
                );
                break;
            case ACTION_BEGIN:
                menu->selected = 0;
                break;
            case ACTION_END:
                menu->selected = g_list_length(items) - 1;
                break;
            case ACTION_RIGHT:
            case ACTION_LEFT:
                widget_key_pressed(widget_get_parent(widget), key);
                break;
            default:
                continue;
        }
        break;
    }

    MenuItem *item = TUI_MENU_ITEM(container_get_child(TUI_CONTAINER(widget), menu->selected));
    if (item->text == NULL) {
        menu_key_pressed(widget, key);
    }

    return KEY_HANDLED;
}

static gint
menu_clicked(Widget *widget, MEVENT mevent)
{
    Menu *menu = TUI_MENU(widget);
    GList *items = container_get_children(TUI_CONTAINER(widget));

    menu->selected = CLAMP(
        mevent.y - widget_get_ypos(widget) - 1,
        0,
        (gint) g_list_length(items) - 1
    );

    MenuItem *item = TUI_MENU_ITEM(container_get_child(TUI_CONTAINER(widget), menu->selected));
    menu_item_activate(item);
    widget_hide(widget);
    return 0;
}

static void
menu_focus_lost(Widget *widget)
{
    widget_hide(widget);
}

static void
menu_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    Menu *menu = TUI_MENU(self);

    switch (property_id) {
        case PROP_TITLE:
            menu->title = g_strdup(g_value_get_string(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
menu_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    Menu *menu = TUI_MENU(self);
    switch (property_id) {
        case PROP_TITLE:
            g_value_set_string(value, menu->title);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}


static void
menu_class_init(MenuClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = menu_set_property;
    object_class->get_property = menu_get_property;

    WidgetClass *widget_class = TUI_WIDGET_CLASS(klass);
    widget_class->draw = menu_draw;
    widget_class->key_pressed = menu_key_pressed;
    widget_class->clicked = menu_clicked;
    widget_class->focus_lost = menu_focus_lost;


    obj_properties[PROP_TITLE] =
        g_param_spec_string("title",
                            "Menu title",
                            "Menu title",
                            "Untitled menu",
                            G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}

static void
menu_init(G_GNUC_UNUSED Menu *self)
{
}
