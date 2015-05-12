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
 * @file media.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in media.h
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "media.h"

void
media_init()
{

}

sdp_media_t *
media_create(const char *address, int port)
{
    sdp_media_t *media = malloc(sizeof(sdp_media_t));

    if (!media)
        return NULL;

    memset(media, 0, sizeof(sdp_media_t));

    media->addr1 = strdup(address);
    media->port1 = port;
    return media;

}

sdp_media_t *
media_add(sdp_media_t *media, const char *addr2, int port2)
{
    if (!media || !addr2)
        return NULL;

    media->addr2 = strdup(addr2);
    media->port2 = port2;
    return media;

}

sdp_media_t *
media_find(sdp_media_t *media, const char *addr, int port)
{
    sdp_media_t *m;

    if (!media)
        return NULL;

    for (m = media; m; m = m->next) {
        if (!strcmp(m->addr1, addr) && m->port1 == port)
            return m;
        if (m->addr2 && !strcmp(m->addr2, addr) && m->port2 == port)
            return m;
    }

    return NULL;
}

sdp_media_t *
media_find_pair(sdp_media_t *media, const char *addr1, int port1, const char *addr2, int port2)
{
    sdp_media_t *m;

    if (!media)
        return NULL;

    for (m = media; m; m = m->next) {
        if (!strcmp(m->addr1, addr1)  && m->port1 == port1)
            if (m->addr2 && addr2 && !strcmp(m->addr2, addr2) && m->port2 == port2)
                return m;
            if (!m->addr2 && !addr2 && m->port2 == port2)
                return m;
    }

    return NULL;
}

