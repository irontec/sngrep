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
#include "tui/widgets/orientable.h"

G_BEGIN_DECLS

// Class declaration
#define SNG_TYPE_BOX sng_box_get_type()
G_DECLARE_DERIVABLE_TYPE(SngBox, sng_box, SNG, BOX, SngContainer)

struct _SngBoxClass
{
    //! Parent class
    SngContainerClass parent;
};

typedef struct
{
    gint top;
    gint bottom;
    gint right;
    gint left;
} SngBoxPadding;

SngWidget *
sng_box_new(SngOrientation orientation);

SngWidget *
sng_box_new_full(SngOrientation orientation, gint spacing, gint padding);

void
sng_box_set_orientation(SngBox *box, SngOrientation orientation);

void
sng_box_set_padding(SngBox *box, SngBoxPadding padding);

void
sng_box_set_padding_full(SngBox *box, gint top, gint bottom, gint left, gint right);

SngBoxPadding
sng_box_get_padding(SngBox *box);

void
sng_box_pack_start(SngBox *box, SngWidget *widget);

void
sng_box_set_background(SngBox *box, chtype background);

void
sng_box_set_border(SngBox *box, gboolean border);

void
sng_box_set_label(SngBox *box, const gchar *label);

G_END_DECLS

#endif    /* __SNGREP_BOX_H__ */
