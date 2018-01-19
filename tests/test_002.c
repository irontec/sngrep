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
 * @file test_002.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * Basic Call list testing
 */

const char keys[] =
        {
          /* Swap some options */
          'c', 'l', 'p', 'p',
          /* Select some dialogs */
          107, 32, 107, 107, 32, 107, 107, 107, 32, 107, 107,
          /* Enter Call Flow */
          10, 27,
          /* Enter Call Raw */
          'R', 27,
          /* Enter Filter screen */
          'F', 27,
          /* Enter Column screen */
          't', 27,
          /* Unselect some dialogs */
          32, 107, 32,
          /* Move beyond list limits */
          107, 107, 107, 107, 107, 106,
          107, 107, 107, 106, 106, 106,
          106, 106, 106, 106, 106, 106,
          106, 106, 106, 106, 106, 106,
          4, 4, 2, 2, 4, 2, 4, 2,
          /* Enter help screen */
          'h', 100,
          /* Enter save screen */
          's', 20, 30, 40, 50, 27,
          /* Enter display filter */
          '/', 20, 30, 40, 40, 10,
          '/', 27,
          /* Enter Call Flow once again */
          10, 27,
          /* Exit */
          27,
          10,
          0
        };

#include "test_input.c"
