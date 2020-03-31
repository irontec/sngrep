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
 * @file scrollbar.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in scrollbar.h
 */

#include "config.h"
#include "tui/tui.h"
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
    // Get window available space
    gint height, width;
    getmaxyx(scrollbar.win, height, width);
    gint scroll_height = height - (scrollbar.preoffset + scrollbar.postoffset);

    // Display the scrollbar left or right
    gint scroll_xpos = (scrollbar.dock == SB_LEFT) ? 0 : width - 1;

    // Initialize scrollbar line
    mvwvline(scrollbar.win, scrollbar.preoffset, scroll_xpos, ACS_VLINE, scroll_height);

    // How long the scroll will be
    gint scroll_len = CLAMP(
        ((scroll_height * 1.0f) / scrollbar.max) * scroll_height,
        1,
        scroll_height
    );

    // Where will the scroll start
    gint scroll_ypos;
    if (scrollbar.pos == 0) {
        scroll_ypos = 0;
    } else if (scrollbar.pos + scroll_height >= scrollbar.max) {
        scroll_ypos = scroll_height - scroll_len - scrollbar.postoffset;
    } else {
        scroll_ypos = (scroll_height - 1.0f) * (scrollbar.pos * 1.0f / scrollbar.max);
    }

    // Draw the N blocks of the scrollbar
    for (gint line = scrollbar.preoffset; line <= scroll_len; line++) {
        mvwaddwstr(
            scrollbar.win,
            line + scroll_ypos,
            scroll_xpos,
            tui_acs_utf8(ACS_BOARD)
        );
    }
}

static void
scrollbar_horizontal_draw(Scrollbar scrollbar)
{
    // Get window available space
    gint height, width;
    getmaxyx(scrollbar.win, height, width);
    gint scroll_width = width - (scrollbar.preoffset + scrollbar.postoffset);

    // Display the scrollbar top or bottom
    gint scroll_ypos = (scrollbar.dock == SB_TOP) ? 0 : height - 1;

    // Initialize scrollbar line
    mvwhline(scrollbar.win, scroll_ypos, scrollbar.preoffset, ACS_HLINE, scroll_width);

    // How long the scroll will be
    gint scroll_len = CLAMP(
        ((scroll_width * 1.0f) / scrollbar.max) * scroll_width,
        1,
        scroll_width
    );

    // Where will the scroll start
    gint scroll_xpos;
    if (scrollbar.pos == 0) {
        scroll_xpos = 0;
    } else if (scrollbar.pos + scroll_width >= scrollbar.max) {
        scroll_xpos = scroll_width - scroll_len;
    } else {
        scroll_xpos = (scroll_width - 1.0f) * (scrollbar.pos * 1.0f / scrollbar.max);
    }

    // Draw the N blocks of the scrollbar
    for (gint line = scrollbar.preoffset; line <= scroll_len; line++) {
        mvwaddwstr(
            scrollbar.win,
            scroll_ypos,
            line + scroll_xpos,
            tui_acs_utf8(ACS_CKBOARD)
        );
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
        return scrollbar.max > getmaxy(scrollbar.win)
                               - scrollbar.preoffset - scrollbar.postoffset;
    } else {
        return scrollbar.max > getmaxx(scrollbar.win)
                               - scrollbar.preoffset - scrollbar.postoffset;
    }
}
