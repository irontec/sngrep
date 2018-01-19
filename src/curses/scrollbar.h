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
 * @file scrollbar.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Scrollbar function and structures
 *
 */

#ifndef __SNGREP_SCROLLBAR_H
#define __SNGREP_SCROLLBAR_H

#ifdef WITH_UNICODE
#define _X_OPEN_SOURCE_EXTENDED
#include <wctype.h>
#endif
#include <ncurses.h>

//! Shorter declaration of scrollbar
typedef struct scrollbar scrollbar_t;

//! Available scrollbar alignments
enum sb_alignment
{
    SB_HORIZONTAL,
    SB_VERTICAL
};

//! Available scrollbar positions
enum sb_dock
{
    SB_TOP,
    SB_BOTTOM,
    SB_LEFT,
    SB_RIGHT
};

/**
 * @brief Window scrollbar
 *
 * This struct contains the required information to draw
 * a scrollbar into a ncurses window
 */
struct scrollbar
{
    //! Ncurses window associated to this scrollbar
    WINDOW *win;
    //! Alignment
    enum sb_alignment alignment;
    //! Position
    enum sb_dock dock;
    //! Current scrollbar position
    int pos;
    //! Max scrollbar positions
    int max;
};

scrollbar_t
ui_set_scrollbar(WINDOW *win, int alignment, int dock);


void
ui_scrollbar_draw(scrollbar_t sb);

#endif /* __SNGREP_SCROLLBAR_H */
