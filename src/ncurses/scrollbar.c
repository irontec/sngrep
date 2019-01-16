/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
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
 * @file scrollbar.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in scrollbar.h
 */

#include "config.h"
#include "scrollbar.h"

Scrollbar
window_set_scrollbar(WINDOW *win, int alignment, int dock)
{
    Scrollbar scrollbar;
    scrollbar.win = win;
    scrollbar.alignment = alignment;
    scrollbar.dock = dock;
    scrollbar.pos = 0;
    scrollbar.max = 0;
    scrollbar.preoffset = 0;
    scrollbar.postoffset = 0;
    return scrollbar;
}

static void
scrollbar_vertical_draw(Scrollbar scrollbar)
{
    int height, width, cline, scrollen, scrollypos, scrollxpos;

    // Get window available space
    getmaxyx(scrollbar.win, height, width);
    height -= scrollbar.preoffset + scrollbar.postoffset;

    // Display the scrollbar left or right
    scrollxpos = (scrollbar.dock == SB_LEFT) ? 0 : width - 1;

    // Initialize scrollbar line
    mvwvline(scrollbar.win, scrollbar.preoffset, scrollxpos, ACS_VLINE, height);

    // How long the scroll will be
    if (!(scrollen = (height * 1.0f / scrollbar.max * height) + 0.5))
        scrollen = 1;

    // Where will the scroll start
    scrollypos = height * (scrollbar.pos * 1.0f / scrollbar.max);

    // Draw the N blocks of the scrollbar
    for (cline = scrollbar.preoffset; cline <= scrollen; cline++) {
        mvwaddch(scrollbar.win, cline + scrollypos, scrollxpos, ACS_CKBOARD);
    }
}

static void
scrollbar_horizontal_draw(Scrollbar scrollbar)
{
    int height, width, cline, scrollen, scrollypos, scrollxpos;

    // Get window available space
    getmaxyx(scrollbar.win, height, width);
    width -= scrollbar.preoffset + scrollbar.postoffset;

    // Display the scrollbar top or bottom
    scrollypos = (scrollbar.dock == SB_TOP) ? 0 : height - 1;

    // Initialize scrollbar line
    mvwhline(scrollbar.win, scrollypos, scrollbar.preoffset, ACS_HLINE, width);

    // How long the scroll will be
    if (!(scrollen = (width * 1.0f / scrollbar.max * width) + 0.5))
        scrollen = 1;

    // Where will the scroll start
    scrollxpos = width * (scrollbar.pos * 1.0f / scrollbar.max);

    // Draw the N blocks of the scrollbar
    for (cline = scrollbar.preoffset; cline <= scrollen; cline++) {
        mvwaddch(scrollbar.win, scrollypos, cline + scrollxpos, ACS_CKBOARD);
    }
}

void
scrollbar_draw(Scrollbar scrollbar)
{
    if (!scrollbar_visible(scrollbar))
        return;

    if (scrollbar.alignment == SB_VERTICAL) {
        scrollbar_vertical_draw(scrollbar);
    } else {
        scrollbar_horizontal_draw(scrollbar);
    }
}

gboolean
scrollbar_visible(Scrollbar scrollbar)
{

    if (scrollbar.alignment == SB_VERTICAL) {
        return scrollbar.max + 1 > getmaxy(scrollbar.win)
                                   - scrollbar.preoffset - scrollbar.postoffset;
    } else {
        return scrollbar.max + 1 > getmaxx(scrollbar.win)
                                   - scrollbar.preoffset - scrollbar.postoffset;
    }
}
