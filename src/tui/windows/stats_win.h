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
 * @file stats_win.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ui window for capture stats display
 */
#ifndef __SNGREP_STATS_WIN_H__
#define __SNGREP_STATS_WIN_H__

#include "tui/widgets/window.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_STATS stats_get_type()
G_DECLARE_FINAL_TYPE(StatsWindow, stats, TUI, STATS, SngAppWindow)


struct _StatsWindow
{
    //! Parent object attributes
    SngAppWindow parent;
};

/**
 * @brief Creates a new stats panel
 *
 * This function allocates all required memory for
 * displaying the stats panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 */
SngAppWindow *
stats_win_new();

G_END_DECLS

#endif /* __SNGREP_STATS_WIN_H__ */
