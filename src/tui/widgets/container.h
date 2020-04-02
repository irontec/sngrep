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
 * @file container.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Widget that contains other widgets
 */
#ifndef __SNGREP_CONTAINER_H__
#define __SNGREP_CONTAINER_H__

#include <glib.h>
#include <glib-object.h>
#include "tui/widgets/widget.h"

G_BEGIN_DECLS

// Class declaration
#define TUI_TYPE_CONTAINER container_get_type()
G_DECLARE_DERIVABLE_TYPE(Container, container, TUI, CONTAINER, Widget)

struct _ContainerClass
{
    //! Parent class
    WidgetClass parent;
};

void
container_add_child(Container *container, Widget *child);

GNode *
container_get_children(Container *container);

Widget *
container_get_child(Container *container, gint index);

Widget *
container_find_by_position(Container *container, gint x, gint y);


#endif    /* __SNGREP_CONTAINER_H__ */
