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
 * @file box.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Container to manage layouts
 */
#ifndef __SNGREP_BOX_H__
#define __SNGREP_BOX_H__

#include <glib.h>
#include <glib-object.h>
#include "tui/widgets/container.h"

G_BEGIN_DECLS

typedef enum
{
  BOX_ORIENTATION_HORIZONTAL,
  BOX_ORIENTATION_VERTICAL
} BoxOrientation;

// Class declaration
#define TUI_TYPE_BOX box_get_type()
G_DECLARE_DERIVABLE_TYPE(Box, box, TUI, BOX, Container)

struct _BoxClass
{
    //! Parent class
    ContainerClass parent;
};

Widget *
box_new(BoxOrientation orientation);

Widget *
box_new_full(BoxOrientation orientation, gint spacing, gint padding);

G_END_DECLS

#endif    /* __SNGREP_BOX_H__ */
