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
 * @file capture_ws.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage websocket protocol
 *
 */
#ifndef __SNGREP_CAPTURE_WS_H
#define __SNGREP_CAPTURE_WS_H

#define WH_FIN      0x80
#define WH_RSV      0x70
#define WH_OPCODE   0x0F
#define WH_MASK     0x80
#define WH_LEN      0x7F

#define WS_OPCODE_TEXT 0x1

int
capture_ws_check_packet(u_char *msg_payload, uint32_t *size_payload);

#endif /* __SNGREP_CAPTURE_WS_H */
