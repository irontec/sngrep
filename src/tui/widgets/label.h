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
 * @file label.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_LABEL_H__
#define __SNGREP_LABEL_H__

#include <glib.h>
#include <glib-object.h>
#include "tui/widgets/widget.h"

G_BEGIN_DECLS

// Class declaration
#define SNG_TYPE_LABEL sng_label_get_type()
G_DECLARE_FINAL_TYPE(SngLabel, sng_label, SNG, LABEL, SngWidget)

struct _SngLabel
{
    //! Parent object attributes
    SngWidget parent;
    //! Item text
    gchar *text;
};

SngWidget *
sng_label_new(const gchar *text);

void
sng_label_set_text(SngLabel *label, const gchar *text);

const gchar *
sng_label_get_text(SngLabel *label);

G_END_DECLS

#endif    /* __SNGREP_LABEL_H__ */
