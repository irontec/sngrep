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
 * @file separator.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_SEPARATOR_H__
#define __SNGREP_SEPARATOR_H__

#include <glib.h>
#include <glib-object.h>
#include "tui/widgets/widget.h"

G_BEGIN_DECLS

// Class declaration
#define SNG_TYPE_SEPARATOR sng_separator_get_type()
G_DECLARE_FINAL_TYPE(SngSeparator, sng_separator, SNG, SEPARATOR, SngWidget)

struct _SngSeparator
{
    //! Parent object attributes
    SngWidget parent;
};

SngWidget *
sng_separator_new();

G_END_DECLS

#endif    /* __SNGREP_SEPARATOR_H__ */
