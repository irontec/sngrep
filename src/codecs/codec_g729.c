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
 * @file codec_g729.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to decode G.729 RTP packets
 *
 */

#include "config.h"
#include <glib.h>
#include <bcg729/decoder.h>
#include "codec_g729.h"

gint16 *
codec_g729_decode(GByteArray *input, gsize *decoded_len)
{
    g_return_val_if_fail(input != NULL, NULL);

    bcg729DecoderChannelContextStruct *ctx = initBcg729DecoderChannel();
    g_return_val_if_fail(ctx != NULL, NULL);

    *decoded_len = (input->len / 10) * 80 * 2 * sizeof(gint16);
    gint16 *out = g_malloc(*decoded_len);

    for (guint i = 0; i < (input->len / 10); i++) {
        bcg729Decoder(ctx, input->data + i * 10, 10, 0, 0, 0, out + i * 80);
    }

    closeBcg729DecoderChannel(ctx);
    return out;
}
