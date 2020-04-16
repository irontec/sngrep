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
 * @file menu_bar.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Common process for Window menu and menu items
 *
 * This file contains common functions shared by all panels.
 */

#ifndef __SNGREP_MENU_BAR_H__
#define __SNGREP_MENU_BAR_H__

#include <glib.h>
#include <glib-object.h>
#include "tui/widgets/container.h"
#include "tui/widgets/menu.h"

G_BEGIN_DECLS

#define MENU_WIDTH  20

#define TUI_TYPE_MENU_BAR menu_bar_get_type()
G_DECLARE_FINAL_TYPE(MenuBar, menu_bar, TUI, MENU_BAR, Container)

struct _MenuBar
{
    //! Parent object attributes
    Container parent;
    //! Selected menu (-1 for none)
    gint selected;
};

SngWidget *
menu_bar_new();

void
menu_bar_free(MenuBar *menu);

G_END_DECLS

#endif /* __SNGREP_MENU_BAR_H__ */
