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
 * @file packet_sdp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Funtions to manage SDP protocol
 */
#include "config.h"
#include <glib.h>
#include "packet_sdp.h"

/**
 * @brief Known RTP encodings
 *
 * This values has been interpreted from:
 * https://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml
 * https://tools.ietf.org/html/rfc3551#section-6
 *
 * Alias names for each RTP encoding name are sngrep developers personal
 * preferrence and may or may not match reality.
 *
 */
SDPFormat formats[] = {
    { 0,    "PCMU/8000",    "g711u"     },
    { 3,    "GSM/8000",     "gsm"       },
    { 4,    "G723/8000",    "g723"      },
    { 5,    "DVI4/8000",    "dvi"       },
    { 6,    "DVI4/16000",   "dvi"       },
    { 7,    "LPC/8000",     "lpc"       },
    { 8,    "PCMA/8000",    "g711a"     },
    { 9,    "G722/8000",    "g722"      },
    { 10,   "L16/44100",    "l16"       },
    { 11,   "L16/44100",    "l16"       },
    { 12,   "QCELP/8000",   "qcelp"     },
    { 13,   "CN/8000",      "cn"        },
    { 14,   "MPA/90000",    "mpa"       },
    { 15,   "G728/8000",    "g728"      },
    { 16,   "DVI4/11025",   "dvi"       },
    { 17,   "DVI4/22050",   "dvi"       },
    { 18,   "G729/8000",    "g729"      },
    { 25,   "CelB/90000",   "celb"      },
    { 26,   "JPEG/90000",   "jpeg"      },
    { 28,   "nv/90000",     "nv"        },
    { 31,   "H261/90000",   "h261"      },
    { 32,   "MPV/90000",    "mpv"       },
    { 33,   "MP2T/90000",   "mp2t"      },
    { 34,   "H263/90000",   "h263"      },
    { 0, NULL, NULL }
};

void
packet_parse_sdp(PacketDissector *handler, Packet *packet, GByteArray *data)
{

}

