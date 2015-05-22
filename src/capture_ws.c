/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2015 Irontec SL. All rights reserved.
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
 * @file capture_ws.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in capture_ws.h
 *
 */

#include "config.h"
#include "capture.h"
#include "capture_ws.h"

int
capture_ws_check_packet(u_char *msg_payload, uint32_t *size_payload)
{
    int offset = 0;
    u_char ws_fin;
    u_char ws_opcode;
    u_char ws_mask;
    uint8_t ws_len;
    u_char ws_mask_key[4];
    int i;

    /**
     * WSocket header definition according to RFC 6455
     *     0                   1                   2                   3
     *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     *    +-+-+-+-+-------+-+-------------+-------------------------------+
     *    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     *    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     *    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     *    | |1|2|3|       |K|             |                               |
     *    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     *    |     Extended payload length continued, if payload len == 127  |
     *    + - - - - - - - - - - - - - - - +-------------------------------+
     *    |                               |Masking-key, if MASK set to 1  |
     *    +-------------------------------+-------------------------------+
     *    | Masking-key (continued)       |          Payload Data         |
     *    +-------------------------------- - - - - - - - - - - - - - - - +
     *    :                     Payload Data continued ...                :
     *    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     *    |                     Payload Data continued ...                |
     *    +---------------------------------------------------------------+
     */

    // Flags && Opcode
    ws_fin = (*msg_payload & WH_FIN) >> 4;
    ws_opcode = *msg_payload & WH_OPCODE;
    offset++;

    // Only interested in Ws text packets
    if (ws_opcode != WS_OPCODE_TEXT)
        return 0;

    // Masked flag && Payload len
    ws_mask = (*(msg_payload + offset) & WH_MASK) >> 4;
    ws_len = (*(msg_payload + offset) & WH_LEN);
    offset++;

    // Skip Payload len
    switch (ws_len) {
        // Extended
        case 126:
            offset += 2;
            break;
        case 127:
            offset += 8;
            break;
    }

    // Get Masking key if mask is enabled
    if (ws_mask) {
        memcpy(ws_mask_key, (msg_payload + offset), 4);
        offset += 4;
    }

    *size_payload -= offset;

    if (*size_payload > 0) {
        // Set the Websocket payload as new payload
        for (i = 0; i < *size_payload; i++)
            msg_payload[i] = msg_payload[i + offset];

        // If mask is enabled, unmask the payload
        if (ws_mask) {
            for (i = 0; i < *size_payload; i++)
                msg_payload[i] = msg_payload[i] ^ ws_mask_key[i % 4];
        }
        msg_payload[*size_payload - 1] = '\0';
    }
    return 1;
}
