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
 * @file menu_item.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in menu.h
 */

#include "config.h"
#include "glib-extra/glib.h"
#include "menu_item.h"

enum
{
    SIG_ACTIVATE,
    SIGS
};

static guint signals[SIGS] = { 0 };

// Menu item class definition
G_DEFINE_TYPE(MenuItem, menu_item, TUI_TYPE_WIDGET)

void
menu_item_set_action(MenuItem *item, KeybindingAction action)
{
    item->action = action;
}

void
menu_item_activate(MenuItem *item)
{
    g_signal_emit(TUI_WIDGET(item), signals[SIG_ACTIVATE], 0);
}

Widget *
menu_item_new(Widget *parent, const gchar *text)
{
    Widget *widget = g_object_new(
        TUI_TYPE_MENU_ITEM,
        "parent", parent,
        NULL
    );

    // TODO
    MenuItem *item = TUI_MENU_ITEM(widget);
    item->text = text;
    return widget;
}

void
menu_item_free(MenuItem *item)
{
    g_object_unref(item);
}

static void
menu_item_init(G_GNUC_UNUSED MenuItem *self)
{
}

static void
menu_item_class_init(MenuItemClass *klass)
{
    signals[SIG_ACTIVATE] =
        g_signal_newv("activate",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     NULL,
                     NULL, NULL,
                     NULL,
                     G_TYPE_NONE, 0, NULL
        );
}
