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
 * @file menu.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Common process for Window menu and menu items
 *
 * This file contains common functions shared by all panels.
 */

#ifndef __SNGREP_MENU_H__
#define __SNGREP_MENU_H__

#include <glib.h>
#include <glib-object.h>
#include <ncurses.h>
#include "tui/widgets/window.h"
#include "tui/widgets/menu_bar.h"
#include "tui/widgets/menu_item.h"
#include "tui/widgets/container.h"
#include "tui/keybinding.h"

G_BEGIN_DECLS

// Menu class declaration
#define TUI_TYPE_MENU menu_get_type()
G_DECLARE_FINAL_TYPE(Menu, menu, TUI, MENU, SngContainer)

struct _Menu
{
    //! Parent object attributes
    SngContainer parent;
    //! Menu title displayed in bar
    const gchar *title;
    //! Selected item index
    gint selected;
};

SngWidget *
menu_new(const gchar *title);

void
menu_free(Menu *menu);

const gchar *
menu_get_title(Menu *menu);

G_END_DECLS

#endif /* __SNGREP_MENU_H__ */
