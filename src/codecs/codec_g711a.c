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
 * @file codec_g711a.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to decode g711a (alaw) RTP packets
 *
 */

#include "config.h"
#include <glib.h>
#include "codec_g711a.h"

// @formatter:off
/** Expansion table, taken from wireshark G711atable.h */
static gint16 alaw_exp_table[256] = {
      -5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736,
      -7552, -7296, -8064, -7808, -6528, -6272, -7040, -6784,
      -2752, -2624, -3008, -2880, -2240, -2112, -2496, -2368,
      -3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392,
     -22016,-20992,-24064,-23040,-17920,-16896,-19968,-18944,
     -30208,-29184,-32256,-31232,-26112,-25088,-28160,-27136,
     -11008,-10496,-12032,-11520, -8960, -8448, -9984, -9472,
     -15104,-14592,-16128,-15616,-13056,-12544,-14080,-13568,
       -344,  -328,  -376,  -360,  -280,  -264,  -312,  -296,
       -472,  -456,  -504,  -488,  -408,  -392,  -440,  -424,
        -88,   -72,  -120,  -104,   -24,    -8,   -56,   -40,
       -216,  -200,  -248,  -232,  -152,  -136,  -184,  -168,
      -1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184,
      -1888, -1824, -2016, -1952, -1632, -1568, -1760, -1696,
       -688,  -656,  -752,  -720,  -560,  -528,  -624,  -592,
       -944,  -912, -1008,  -976,  -816,  -784,  -880,  -848,
       5504,  5248,  6016,  5760,  4480,  4224,  4992,  4736,
       7552,  7296,  8064,  7808,  6528,  6272,  7040,  6784,
       2752,  2624,  3008,  2880,  2240,  2112,  2496,  2368,
       3776,  3648,  4032,  3904,  3264,  3136,  3520,  3392,
      22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944,
      30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
      11008, 10496, 12032, 11520,  8960,  8448,  9984,  9472,
      15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,
        344,   328,   376,   360,   280,   264,   312,   296,
        472,   456,   504,   488,   408,   392,   440,   424,
         88,    72,   120,   104,    24,     8,    56,    40,
        216,   200,   248,   232,   152,   136,   184,   168,
       1376,  1312,  1504,  1440,  1120,  1056,  1248,  1184,
       1888,  1824,  2016,  1952,  1632,  1568,  1760,  1696,
        688,   656,   752,   720,   560,   528,   624,   592,
        944,   912,  1008,   976,   816,   784,   880,   848
};
// @formatter:on

gint16 *
codec_g711a_decode(GByteArray *input, gsize *decoded_len)
{
    g_return_val_if_fail(input != NULL, 0);

    *decoded_len = input->len * sizeof(gint16);
    gint16 *out = g_malloc(*decoded_len);


    for (guint i = 0; i < input->len; i++) {
        out[i] = alaw_exp_table[input->data[i]];
    }

    return out;
}
