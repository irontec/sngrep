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
 * @file test_005.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * Basic Column selection testing
 */

const char keys[] =
        {
         /* Show Raw panel */
         't',
         /* Cycle through fields */
         9, 9, 9, 9, 9, 9,
         9, 107, 107, 9, 106,
         /* Move through Column list */
         107, 107, 107, 107, 107, 106,
         107, 107, 107, 107, 107, 107,
         107, 107, 107, 107, 107, 107,
         107, 107, 107, 107, 107, 107,
         107, 107, 107, 107, 107, 107,
         107, 107, 107, 106, 106, 106,
         106, 106, 106, 106, 106, 106,
         106, 106, 106, 106, 106, 106,
         106, 106, 106, 106, 106, 106,
         106, 106, 106, 106, 106, 106,
         106, 106, 106, 106, 106, 106,
         106, 106, 106, 106, 106, 106,
         /* Move items up and down */
         '-', '-', '-', '-',
         107, 107,
         '+', '+', '+', '+',
         107, 107,
         '-', '-', '-', '-',
         /* Apply new settings */
         10,
         /* Exit */
         27,
         10,
         0
        };

#include "test_input.c"
