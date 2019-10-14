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
 * @file codec_g711a.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to decode g711a (alaw) RTP packets
 *
 */

#ifndef __SNGREP_CODEC_H
#define __SNGREP_CODEC_H

#include "storage/stream.h"

//! Error reporting
#define CODEC_ERROR (codec_error_quark())

//! Error codes
enum codec_errors
{
    CODEC_ERROR_INVALID_FORMAT = 0,
};

/**
 * @brief Codec domain struct for GError
 */
GQuark
codec_error_quark();

/**
 * Decode stream rtp packets payload into raw byte array
 * @param stream Streams to decode payload
 * @param decoded Already decoded data from a previous call
 * @param error GError with failure description (optional)
 * @return a byte array of decoded rtp payload
 */
GByteArray *
codec_stream_decode(Stream *stream, GByteArray *decoded, GError **error);

#endif      // __SNGREP_CODEC_H
