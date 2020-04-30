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
 * @file orientable.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include "glib-extra/glib_enum_types.h"
#include "orientable.h"

G_DEFINE_INTERFACE(SngOrientable, sng_orientable, G_TYPE_OBJECT)

void
sng_orientable_set_orientation(SngOrientable *orientable, SngOrientation orientation)
{
    g_return_if_fail(SNG_IS_ORIENTABLE(orientable));
    g_object_set(orientable, "orientation", orientation, NULL);
}

SngOrientation
sng_orientable_get_orientation(SngOrientable *orientable)
{
    g_return_val_if_fail(SNG_IS_ORIENTABLE(orientable), SNG_ORIENTATION_HORIZONTAL);
    SngOrientation orientation;
    g_object_get(orientable, "orientation", &orientation, NULL);
    return orientation;
}

static void
sng_orientable_default_init(SngOrientableInterface *iface)
{
    g_object_interface_install_property(
        iface,
        g_param_spec_enum("orientation",
                          "Orientation",
                          "The orientation of the orientable",
                          SNG_TYPE_ORIENTATION,
                          SNG_ORIENTATION_VERTICAL,
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE)
    );
}
