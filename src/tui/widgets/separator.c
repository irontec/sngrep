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
 * @file separator.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 */

#include "config.h"
#include "tui/theme.h"
#include "tui/widgets/separator.h"

// Class definition
G_DEFINE_TYPE(SngSeparator, sng_separator, SNG_TYPE_WIDGET)

SngWidget *
sng_separator_new()
{
    return g_object_new(
        SNG_TYPE_SEPARATOR,
        "visible", TRUE,
        "height", 1,
        "hexpand", TRUE,
        "can-focus", FALSE,
        NULL
    );
}

static gint
sng_separator_draw(SngWidget *widget)
{
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wattron(win, COLOR_PAIR(CP_BLUE_ON_DEF));
    whline(win, ACS_HLINE, sng_widget_get_width(widget));

    // Chain up parent draw
    return SNG_WIDGET_CLASS(sng_separator_parent_class)->draw(widget);
}

static void
sng_separator_init(G_GNUC_UNUSED SngSeparator *self)
{
}

static void
sng_separator_class_init(SngSeparatorClass *klass)
{
    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_separator_draw;
}
