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
 * @file progress_bar.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_PROGRESS_BAR_H__
#define __SNGREP_PROGRESS_BAR_H__

#include <glib.h>
#include "tui/widgets/widget.h"

G_BEGIN_DECLS

// Class declaration
#define SNG_TYPE_PROGRESS_BAR sng_progress_bar_get_type()
G_DECLARE_FINAL_TYPE(SngProgressBar, sng_progress_bar, SNG, PROGRESS_BAR, SngWidget)

struct _SngProgressBar
{
    //! Parent object attributes
    SngWidget parent;
    //! Current progress position
    gdouble fraction;
    //! Determine if fraction should be displayed in the bar
    gboolean show_text;
};

SngWidget *
sng_progress_bar_new();

gdouble
sng_progress_bar_get_fraction(SngProgressBar *pbar);

void
sng_progress_bar_set_fraction(SngProgressBar *pbar, gdouble fraction);

gint
sng_progress_bar_set_show_text(SngProgressBar *pbar, gboolean show_text);

G_END_DECLS

#endif    /* __SNGREP_PROGRESS_BAR_H__ */
