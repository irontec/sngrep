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
 * @file codec_g711u.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to decode g711u (ulaw) RTP packets
 *
 */

#include "config.h"
#include <glib.h>
#include "codec_g711u.h"

// @formatter:off
/** Expansion table, taken from wireshark G711utable.h */
static gint16 ulaw_exp_table[256] = {
       -32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,
       -23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764,
       -15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412,
       -11900,-11388,-10876,-10364, -9852, -9340, -8828, -8316,
        -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
        -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092,
        -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
        -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
        -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
        -1372, -1308, -1244, -1180, -1116, -1052,  -988,  -924,
         -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652,
         -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,
         -372,  -356,  -340,  -324,  -308,  -292,  -276,  -260,
         -244,  -228,  -212,  -196,  -180,  -164,  -148,  -132,
         -120,  -112,  -104,   -96,   -88,   -80,   -72,   -64,
          -56,   -48,   -40,   -32,   -24,   -16,    -8,     0,
        32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
        23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764,
        15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
        11900, 11388, 10876, 10364,  9852,  9340,  8828,  8316,
         7932,  7676,  7420,  7164,  6908,  6652,  6396,  6140,
         5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092,
         3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004,
         2876,  2748,  2620,  2492,  2364,  2236,  2108,  1980,
         1884,  1820,  1756,  1692,  1628,  1564,  1500,  1436,
         1372,  1308,  1244,  1180,  1116,  1052,   988,   924,
          876,   844,   812,   780,   748,   716,   684,   652,
          620,   588,   556,   524,   492,   460,   428,   396,
          372,   356,   340,   324,   308,   292,   276,   260,
          244,   228,   212,   196,   180,   164,   148,   132,
          120,   112,   104,    96,    88,    80,    72,    64,
           56,    48,    40,    32,    24,    16,     8,     0
};
// @formatter:on

gint16 *
codec_g711u_decode(GByteArray *input, gsize *decoded_len)
{
    g_return_val_if_fail(input != NULL, 0);

    *decoded_len = input->len * sizeof(gint16);
    gint16 *out = g_malloc(*decoded_len);


    for (guint i = 0; i < input->len; i++) {
        out[i] = ulaw_exp_table[input->data[i]];
    }

    return out;
}
