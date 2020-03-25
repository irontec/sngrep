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
 * @file codec.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage codecs and decode data
 */

#include "config.h"
#include <glib.h>
#include "codecs/codec_g711a.h"
#include "codecs/codec_g711u.h"
#ifdef WITH_G729
#include "codecs/codec_g729.h"
#endif
#include "glib-extra/glib.h"
#include "storage/stream.h"
#include "codec.h"

GQuark
codec_error_quark()
{
    return g_quark_from_static_string("codec");
}

static void
codec_generate_silence(GByteArray *payload, gint samples)
{
    g_autofree guint8 *silence = g_malloc0(sizeof(guint8) * samples);
    g_byte_array_append(payload, silence, samples);
}

GByteArray *
codec_stream_decode(Stream *stream, GByteArray *decoded, GError **error)
{
    g_return_val_if_fail(stream != NULL, NULL);

    if (decoded == NULL) {
        decoded = g_byte_array_new();
    }

    g_autoptr(GByteArray) rtp_payload = g_byte_array_new();
    guint64 prev = 0;
    gint ptime = 20;
    for (guint i = 0; i < g_ptr_array_len(stream->packets); i++) {
        Packet *packet = g_ptr_array_index(stream->packets, i);
        PacketRtpData *rtp = packet_get_protocol_data(packet, PACKET_PROTO_RTP);
        g_byte_array_append(
            rtp_payload,
            g_bytes_get_data(rtp->payload, NULL),
            g_bytes_get_size(rtp->payload)
        );
        if (prev > 0) {
            gint diff = (packet_time(packet) - prev) / G_MSEC_PER_SEC;
            if (diff > ptime * 2) {
                codec_generate_silence(rtp_payload, diff);
            }
        }
        prev = packet_time(packet);
    }

    // Already decoded all byte streams
    if (g_byte_array_len(rtp_payload) <= g_byte_array_len(decoded) / 2) {
        return decoded;
    }

    // Remove already decoded bytes
    g_byte_array_remove_range(rtp_payload, 0, g_byte_array_len(decoded) / 2);

    gint16 *dbytes = NULL;
    gsize dlen = 0;
    switch (stream->fmtcode) {
        case RTP_CODEC_G711A:
            dbytes = codec_g711a_decode(rtp_payload, &dlen);
            break;
        case RTP_CODEC_G711U:
            dbytes = codec_g711u_decode(rtp_payload, &dlen);
            break;
#ifdef WITH_G729
        case RTP_CODEC_G729:
            dbytes = codec_g729_decode(rtp_payload, &dlen);
            break;
#endif
        default:
            g_set_error(error,
                        CODEC_ERROR,
                        CODEC_ERROR_INVALID_FORMAT,
                        "Unsupported RTP payload type %d",
                        stream->fmtcode);
            return decoded;
    }

    g_byte_array_append(decoded, (const guint8 *) dbytes, (guint) dlen);
    return decoded;
}
