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
 * @brief SngWidget that contains other widgets
 */
#ifndef __SNGREP_CONTAINER_H__
#define __SNGREP_CONTAINER_H__

#include <glib.h>
#include <glib-object.h>
#include "tui/widgets/widget.h"

G_BEGIN_DECLS

// Class declaration
#define SNG_TYPE_CONTAINER sng_container_get_type()
G_DECLARE_DERIVABLE_TYPE(SngContainer, sng_container, SNG, CONTAINER, SngWidget)

struct _SngContainerClass
{
    //! Parent class
    SngWidgetClass parent;
    //! Container class functions
    void (*add) (SngContainer *container, SngWidget *widget);
    void (*remove)(SngContainer *container, SngWidget *widget);
};

void
sng_container_add(SngContainer *container, SngWidget *child);

void
sng_container_remove(SngContainer *container, SngWidget *child);

void
sng_container_remove_all(SngContainer *container);

void
sng_container_foreach(SngContainer *container, GFunc callback, gpointer user_data);

GList *
sng_container_get_children(SngContainer *container);

void
sng_container_set_children(SngContainer *container, GList *children);

SngWidget *
sng_container_get_child(SngContainer *container, gint index);

SngWidget *
sng_container_find_by_position(SngContainer *container, gint x, gint y);

void
sng_container_show_all(SngContainer *container);


#endif    /* __SNGREP_CONTAINER_H__ */
