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

scrollbar_t
ui_set_scrollbar(WINDOW *win, int alignment, int dock)
{
    scrollbar_t sb;
    sb.win = win;
    sb.alignment = alignment;
    sb.dock = dock;
    return sb;
}


void
ui_scrollbar_draw(scrollbar_t sb)
{
    int height, width, cline, scrollen, scrollypos, scrollxpos;

    // Get window available space
    getmaxyx(sb.win, height, width);

    // If no even a screen has been filled, don't draw it
    if (sb.max < height)
        return;

    // Display the scrollbar left or right
    scrollxpos = (sb.dock == SB_LEFT) ? 0 : width - 1;

    // Initialize scrollbar line
    mvwvline(sb.win, 0, scrollxpos, ACS_VLINE, height);

    // How long the scroll will be
    if (!(scrollen = (height * 1.0f / sb.max * height) + 0.5))
        scrollen = 1;

    // Where will the scroll start
    scrollypos = height * (sb.pos * 1.0f / sb.max);

    // Draw the N blocks of the scrollbar
    for (cline = 0; cline < scrollen; cline++) {
        mvwaddch(sb.win, cline + scrollypos, scrollxpos, ACS_CKBOARD);
    }

}
