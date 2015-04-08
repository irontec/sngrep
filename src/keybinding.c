/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2015 Irontec SL. All rights reserved.
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
 * @file group.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in keybinding.h
 *
 */

#include "config.h"
#include "keybinding.h"

//! sngrep keybindings
key_binding_t bindings[ACTION_SENTINEL];

void
key_bind_action(int action, int key)
{
    bindings[action].keys[bindings[action].bindcnt++] = key;
}

int
key_find_action(int key, int start)
{
    int i, j;
    for (i = start + 1; i < ACTION_SENTINEL; i++) {
        for (j = 0; j < bindings[i].bindcnt; j++)
        if (bindings[i].keys[j] == key)
            return i;
    }
    return -1;
}

