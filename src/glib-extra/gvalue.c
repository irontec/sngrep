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
 * @file gvalue.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Helper functions for GValue
 */
#include "config.h"
#include "gvalue.h"

const gchar *
g_value_get_enum_nick(const GValue *value)
{
    GType enum_type = G_VALUE_TYPE(value);
    GEnumClass *enum_class = g_type_class_ref(enum_type);
    g_return_val_if_fail(enum_class != NULL, NULL);
    GEnumValue *enum_value = g_enum_get_value(enum_class, g_value_get_enum(value));
    g_return_val_if_fail(enum_value != NULL, NULL);
    const gchar *nick = enum_value->value_nick;
    g_type_class_unref(enum_class);
    return nick;
}
