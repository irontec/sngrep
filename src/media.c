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
 * @file media.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in media.h
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "media.h"
#include "rtp.h"
#include "util.h"

sdp_media_t *
media_create(struct sip_msg *msg)
{
    sdp_media_t *media;;

    // Allocate memory for this media structure
    if (!(media = sng_malloc(sizeof(sdp_media_t))))
        return NULL;

    // Initialize all fields
    media->msg = msg;
    media->formats = vector_create(0, 1);
    vector_set_destroyer(media->formats, vector_generic_destroyer);
    return media;
}

void
media_destroyer(void *item)
{
    sdp_media_t *media = (sdp_media_t *) item;
    if (!item)
        return;
    vector_destroy(media->formats);
    sng_free(media);
}

void
media_set_type(sdp_media_t *media, const char *type)
{
    strncpy(media->type, type, MEDIATYPELEN - 1);
}

void
media_set_address(sdp_media_t *media, address_t addr)
{
    media->address = addr;
}

void
media_set_prefered_format(sdp_media_t *media, uint32_t code)
{
    media->fmtcode = code;
}

void
media_add_format(sdp_media_t *media, uint32_t code, const char *format)
{
    sdp_media_fmt_t *fmt;

    if (!(fmt = sng_malloc(sizeof(sdp_media_fmt_t))))
        return;

    fmt->id = code;
    strcpy(fmt->format, format);
    vector_append(media->formats, fmt);
}

const char *
media_get_format(sdp_media_t *media, uint32_t code)
{
    sdp_media_fmt_t *format;
    vector_iter_t iter;
    iter = vector_iterator(media->formats);
    while ((format = vector_iterator_next(&iter))) {
        if (format->id == code)
            return format->format;
    }

    return "Unassigned";
}

const char *
media_get_prefered_format(sdp_media_t *media)
{
    const char *format;

    // Check if format is standard
    if ((format = rtp_get_standard_format(media->fmtcode))) {
        return format;
    }
    // Try to get format form SDP payload
    return media_get_format(media, media->fmtcode);
}

int
media_get_format_code(sdp_media_t *media)
{
    return media->fmtcode;
}



