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
 * @file progress_bar.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in progress_bar.h
 */
#include "config.h"
#include <glib-object.h>
#include "tui/tui.h"
#include "tui/widgets/progress_bar.h"

// Class definition
G_DEFINE_TYPE(SngProgressBar, sng_progress_bar, SNG_TYPE_WIDGET)

SngWidget *
sng_progress_bar_new()
{
    return g_object_new(
        SNG_TYPE_PROGRESS_BAR,
        "height", 1,
        "hexpand", TRUE,
        NULL
    );
}

gdouble
sng_progress_bar_get_fraction(SngProgressBar *pbar)
{
    return pbar->fraction;
}

void
sng_progress_bar_set_fraction(SngProgressBar *pbar, gdouble fraction)
{
    pbar->fraction = fraction;
}

gint
sng_progress_bar_set_show_text(SngProgressBar *pbar, gboolean show_text)
{
    pbar->show_text = show_text;
}

static void
sng_progress_bar_draw(SngWidget *widget)
{
    SngProgressBar *pbar = SNG_PROGRESS_BAR(widget);

    WINDOW *win = sng_widget_get_ncurses_window(widget);
    gint width = sng_widget_get_width(widget);

    tui_whline(win, 0, 0, '-', width);
    mvwaddch(win, 0, 0, '[');
    mvwaddch(win, 0, width - 1, ']');
    tui_whline(win, 0, 1, ACS_CKBOARD, pbar->fraction * (width - 2));

    if (pbar->show_text) {
        mvwprintw(win, 0, width / 2 - 4, " %.2f%% ", pbar->fraction * 100);
    }

    // Chain-up parent draw function
    SNG_WIDGET_CLASS(sng_progress_bar_parent_class)->draw(widget);
}

static void
sng_progress_bar_class_init(SngProgressBarClass *klass)
{
    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_progress_bar_draw;
}

static void
sng_progress_bar_init(G_GNUC_UNUSED SngProgressBar *pbar)
{
    pbar->show_text = TRUE;
}
