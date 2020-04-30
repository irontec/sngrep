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
 * @file orientable.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_ORIENTABLE_H__
#define __SNGREP_ORIENTABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum
{
    SNG_ORIENTATION_HORIZONTAL,
    SNG_ORIENTATION_VERTICAL
} SngOrientation;

#define SNG_TYPE_ORIENTABLE sng_orientable_get_type()
G_DECLARE_INTERFACE(SngOrientable, sng_orientable, SNG, ORIENTABLE, GObject)

struct _SngOrientableInterface
{
    //! Parent interface
    GTypeInterface parent_iface;
};

void
sng_orientable_set_orientation(SngOrientable *orientable, SngOrientation  orientation);

SngOrientation
sng_orientable_get_orientation (SngOrientable *orientable);

G_END_DECLS

#endif    /* __SNGREP_ORIENTABLE_H__ */
