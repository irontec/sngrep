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
G_DEFINE_TYPE(SngMenu, sng_menu, SNG_TYPE_CONTAINER)

SngWidget *
sng_menu_new(const gchar *title)
{
    return g_object_new(
        TUI_TYPE_MENU,
        "visible", FALSE,
        "title", title,
        "floating", TRUE,
        NULL
    );
}

void
sng_menu_free(SngMenu *menu)
{
    g_object_unref(menu);
}

const gchar *
sng_menu_get_title(SngMenu *menu)
{
    return menu->title;
}

static void
sng_menu_realize(SngWidget *widget)
{
    GList *children = sng_container_get_children(SNG_CONTAINER(widget));
    gint height = g_list_length(children) + 2;
    gint width = MENU_WIDTH + 2 + 6 + 2;
    for (GList *l = children; l != NULL; l = l->next) {
        SngMenuItem *item = SNG_MENU_ITEM(l->data);
        if (item->text != NULL) {
            width = MAX(width, (gint) strlen(item->text) + 2 + 6 + 2);
        }
    }

    WINDOW *win = newpad(height, width);
    sng_widget_set_size(widget, width, height);
    sng_widget_set_ncurses_window(widget, win);
}

static void
sng_menu_draw(SngWidget *widget)
{
    SngMenu *menu = SNG_MENU(widget);

    // Set menu background color
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wbkgd(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
    box(win, 0, 0);

    // Get menu popup width
    gint width = sng_widget_get_width(widget);

    GList *children = sng_container_get_children(SNG_CONTAINER(widget));
    for (gint i = 0; i < (gint) g_list_length(children); i++) {
        SngMenuItem *item = SNG_MENU_ITEM(g_list_nth_data(children, i));

        if (item->text != NULL) {
            if (menu->selected == i) {
                wattron(win, COLOR_PAIR(CP_WHITE_ON_DEF));
            }
            mvwprintw(win, i + 1, 1,
                      "%-*s %-6s",
                      width - 6 - 2 - 1,
                      item->text,
                      (item->action != ACTION_NONE) ? key_action_key_str(item->action) : ""
            );
        } else {
            mvwhline(win, i + 1, 0, ACS_HLINE, width);
            mvwaddch(win, i + 1, 0, ACS_LTEE);
            mvwaddch(win, i + 1, width - 1, ACS_RTEE);
        }
        wattron(win, COLOR_PAIR(CP_BLACK_ON_CYAN));
    }
}

static void
sng_menu_key_pressed(SngWidget *widget, gint key)
{
    SngMenu *menu = SNG_MENU(widget);
    GList *children = sng_container_get_children(SNG_CONTAINER(widget));

    // Check actions for this key
    SngAction action = ACTION_NONE;
    while ((action = key_find_action(key, action)) != ACTION_NONE) {
        // Check if we handle this action
        switch (action) {
            case ACTION_DOWN:
                menu->selected = CLAMP(
                    menu->selected + 1,
                    0,
                    (gint) g_list_length(children) - 1
                );
                break;
            case ACTION_UP:
                menu->selected = CLAMP(
                    menu->selected - 1,
                    0,
                    (gint) g_list_length(children) - 1
                );
                break;
            case ACTION_BEGIN:
                menu->selected = 0;
                break;
            case ACTION_END:
                menu->selected = g_list_length(children) - 1;
                break;
            case ACTION_RIGHT:
            case ACTION_LEFT:
                sng_widget_key_pressed(sng_widget_get_parent(widget), key);
                break;
            case ACTION_CONFIRM:
                sng_widget_lose_focus(widget);
                sng_menu_item_activate(g_list_nth_data(children, menu->selected));
                break;
            case ACTION_CANCEL:
                sng_widget_lose_focus(widget);
                break;
            default:
                continue;
        }
        break;
    }

    SngMenuItem *item = SNG_MENU_ITEM(sng_container_get_child(SNG_CONTAINER(widget), menu->selected));
    if (item->text == NULL) {
        sng_menu_key_pressed(widget, key);
    }
}

static void
sng_menu_clicked(SngWidget *widget, MEVENT mevent)
{
    SngMenu *menu = SNG_MENU(widget);
    GList *children = sng_container_get_children(SNG_CONTAINER(widget));

    menu->selected = CLAMP(
        mevent.y - sng_widget_get_ypos(widget) - 1,
        0,
        (gint) g_list_length(children) - 1
    );

    SngMenuItem *item = SNG_MENU_ITEM(sng_container_get_child(SNG_CONTAINER(widget), menu->selected));
    sng_widget_lose_focus(widget);
    sng_menu_item_activate(item);
}

static void
sng_menu_focus_lost(SngWidget *widget)
{
    // Hide menu
    sng_widget_hide(widget);
    // Chain-up parent focus lost function
    SNG_WIDGET_CLASS(sng_menu_parent_class)->focus_lost(widget);
}

static void
sng_menu_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngMenu *menu = SNG_MENU(self);

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
sng_menu_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngMenu *menu = SNG_MENU(self);
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
sng_menu_class_init(SngMenuClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = sng_menu_set_property;
    object_class->get_property = sng_menu_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->realize = sng_menu_realize;
    widget_class->draw = sng_menu_draw;
    widget_class->key_pressed = sng_menu_key_pressed;
    widget_class->clicked = sng_menu_clicked;
    widget_class->focus_lost = sng_menu_focus_lost;


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
sng_menu_init(G_GNUC_UNUSED SngMenu *self)
{
}
