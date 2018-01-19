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
 * @file test_001.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * Test for adding a new attribute column and sorting using it.
 */

const char keys[] =
        {
          /* show sort menu */
          60,
          /* select sort field */
          106, 106, 10,
          /* Enter Column screen */
          116, 27,
          /* add a new column */
          106, 106, 106, 32,
          /* select new sort attribute */
          60, 106, 106, 106, 106, 106, 106, 10,
          /* swap order */
          122, 122,
          /* Leave */
          27, 10,
          0
        };

#include "test_input.c"
