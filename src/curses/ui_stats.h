/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2016 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2016 Irontec SL. All rights reserved.
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
 * @file ui_stats.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ui window for capture stats display
 */
#ifndef __UI_STATS_H
#define __UI_STATS_H

/**
 * @brief Creates a new stats panel
 *
 * This function allocates all required memory for
 * displaying the stats panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @return a panel pointer
 */
PANEL *
stats_create();

/**
 * @brief Destroy stats  panel
 *
 * This function do the final cleanups for this panel
 */
void
stats_destroy();

/**
 * @brief Manage pressed keys for stats panel
 *
 * This function is called by UI manager every time a
 * key is pressed. This allow the filter panel to manage
 * its own keys.
 * If this function return 0, the key will not be handled
 * by ui manager. Otherwise the return will be considered
 * a key code.
 *
 * @param panel Filter panel pointer
 * @param key   key code
 * @return 0 if the key is handled, keycode otherwise
 */
int
stats_handle_key(PANEL *panel, int key);



#endif /* __UI_STATS_H */
