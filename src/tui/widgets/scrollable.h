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
 * @file scrollbar.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Scrollbar function and structures
 *
 */

#ifndef __SNGREP_SCROLLABLE_H__
#define __SNGREP_SCROLLABLE_H__

#include <glib.h>
#include "tui/widgets/scrollbar.h"
#include "tui/widgets/container.h"

G_BEGIN_DECLS

// Class declaration
#define SNG_TYPE_SCROLLABLE sng_scrollable_get_type()
G_DECLARE_DERIVABLE_TYPE(SngScrollable, sng_scrollable, SNG, SCROLLABLE, SngContainer)

struct _SngScrollableClass
{
    //! Parent class
    SngContainerClass parent;
};

SngScrollbar *
sng_scrollable_get_vscroll(SngScrollable *scrollable);

SngScrollbar *
sng_scrollable_get_hscroll(SngScrollable *scrollable);

SngWidget *
sng_scrollable_get_content(SngScrollable *scrollable);

void
sng_scrollable_set_padding(SngScrollable *scrollable, gint top, gint bottom, gint left, gint right);

G_END_DECLS

#endif /*__SNGREP_SCROLLABLE_H__*/
