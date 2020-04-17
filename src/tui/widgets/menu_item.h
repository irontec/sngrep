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
 * @file menu_item.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Common process for Window menu and menu items
 *
 * This file contains common functions shared by all panels.
 */

#ifndef __SNGREP_MENU_ITEM_H__
#define __SNGREP_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>
#include <ncurses.h>
#include "tui/widgets/window.h"
#include "tui/keybinding.h"

G_BEGIN_DECLS

// MenuItem class declaration
#define SNG_TYPE_MENU_ITEM sng_menu_item_get_type()
G_DECLARE_FINAL_TYPE(SngMenuItem, sng_menu_item, SNG, MENU_ITEM, SngWidget)

struct _SngMenuItem
{
    //! Parent object attributes
    SngWidget parent;
    //! Item text
    const gchar *text;
    //! Mark this entry as enabled
    gboolean checked;
    //! Keybinding Action
    KeybindingAction action;
};

void
sng_menu_item_set_action(SngMenuItem *item, KeybindingAction action);

void
sng_menu_item_activate(SngMenuItem *item);

SngWidget *
sng_menu_item_new(const gchar *text);

void
sng_menu_item_free(SngMenuItem *item);

G_END_DECLS

#endif /* __SNGREP_MENU_ITEM_H__ */
