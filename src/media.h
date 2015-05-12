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
 * @file media.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage call media
 *
 */

#ifndef __SNGREP_MEDIA_H_
#define __SNGREP_MEDIA_H_

#include "config.h"

//! Shorter declaration of sip_media structure
typedef struct sdp_media sdp_media_t;

struct sdp_media
{
    //! SDP Addresses
    char *addr1;
    char *addr2;
    //! SDP ports
    int port1;
    int port2;
    //! Packets from addr1 to addt2
    int txcnt;
    //! Packets from addr2 to addr1
    int rvcnt;

    //! Next media in same call
    sdp_media_t *next;
};

sdp_media_t *
media_create(const char *address, int port);

sdp_media_t *
media_add(sdp_media_t *media, const char *addr2, int port2);

sdp_media_t *
media_find(sdp_media_t *media, const char *addr, int port);

sdp_media_t *
media_find_pair(sdp_media_t *media, const char *addr1, int port1, const char *addr2, int port2);

#endif /* __SNGREP_MEDIA_H_ */
