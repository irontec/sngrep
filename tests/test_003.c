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
 * @file test_003.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * Basic Call Flow testing
 */

const char keys[] =
        {
          /* Select some dialogs */
          32, 107, 32, 107,
          /* Enter Call Flow */
          10, 27,
          /* Move arrow messages */
          107, 107, 107, 107, 107, 106,
          107, 107, 107, 106, 106, 106,
          106, 106, 106, 106, 106, 106,
          106, 106, 106, 106, 106, 106,
          4, 4, 2, 2, 4, 2, 4, 2,
          /* Reenter Call Flow */
          10,
          /* Swap colors */
          'c', 'c', 'c', 'c', 'c', 'c',
          /* Enter Extended Call Flow */
          'x', 'x', 'x', 'x',
          /* Compress column display */
          's',
          /* Toggle media display */
          'd', 'd', 'd', 'd', 'd',
          /* Toggle Only SDP */
          'D',
          /* Enter Call Raw (all dialogs) */
          'R', 27,
          /* Enter Call Raw (single message) */
          10, 27,
          /* Enter help screen */
          'h', 100,
          /* Toggle raw screen */
          't', 't', 't', 't',
          /* Change Raw size */
          '0', '0', '0', '0', '0', '0', '0', '0',
          '9', '9', '9', '9', '9', '9', '9', '9',
          /*  Reset Raw size */
          'T', 'T',
          /* Leave Call Flow */
          27,
          /* Exit */
          27,
          10,
          0
        };

#include "test_input.c"
