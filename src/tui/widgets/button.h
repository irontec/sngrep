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
 * @file button.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_BUTTON_H__
#define __SNGREP_BUTTON_H__

#include <glib.h>
#include <glib-object.h>
#include <form.h>
#include "tui/widgets/label.h"

G_BEGIN_DECLS

// Class declaration
#define SNG_TYPE_BUTTON sng_button_get_type()
G_DECLARE_FINAL_TYPE(SngButton, sng_button, SNG, BUTTON, SngLabel)

struct _SngButton
{
    //! Parent object attributes
    SngLabel parent;
};

SngWidget *
sng_button_new();

void
sng_button_set_text(SngButton *button, const gchar *text);

const gchar *
sng_button_get_text(SngButton *button);

G_END_DECLS

#endif    /* __SNGREP_BUTTON_H__ */
