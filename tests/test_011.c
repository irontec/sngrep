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
 * @file test_011.c
 * @author Evgeny Khramtsov <evgeny.khramtsov@nordigy.ru>
 *
 * IP-IP tunnel test from ipip.pcap
 */

const char keys[] =
        {
          /* Enter Call Flow */
          10,
          /* Leave Call Flow */
          27,
          /* Exit */
          27,
          10,
          0
        };

#define TEST_PCAP_INPUT "ipip.pcap"

#include "test_input.c"
