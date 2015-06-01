/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
 * @file sip_attr.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in sip_attr.h
 *
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "option.h"
#include "sip_attr.h"

static sip_attr_hdr_t attrs[SIP_ATTR_SENTINEL] = {
    { .id = SIP_ATTR_CALLINDEX,     .name = "index", .title = "Idx", .desc = "Call Index", .dwidth = 4 },
    { .id = SIP_ATTR_SIPFROM,       .name = "sipfrom", .desc = "SIP From", .dwidth = 30 },
    { .id = SIP_ATTR_SIPFROMUSER,   .name = "sipfromuser", .desc = "SIP From User", .dwidth = 20 },
    { .id = SIP_ATTR_SIPTO,         .name = "sipto", .desc = "SIP To", .dwidth = 30 },
    { .id = SIP_ATTR_SIPTOUSER,     .name = "siptouser", .desc = "SIP To User", .dwidth = 20 },
    { .id = SIP_ATTR_SRC,           .name = "src", .desc = "Source", .dwidth = 22 },
    { .id = SIP_ATTR_DST,           .name = "dst", .desc = "Destination", .dwidth = 22 },
    { .id = SIP_ATTR_CALLID,        .name = "callid", .desc = "Call-ID", .dwidth = 50 },
    { .id = SIP_ATTR_XCALLID,       .name = "xcallid", .desc = "X-Call-ID", .dwidth = 50 },
    { .id = SIP_ATTR_DATE,          .name = "date", .desc = "Date", .dwidth = 10 },
    { .id = SIP_ATTR_TIME,          .name = "time", .desc = "Time", .dwidth = 8 },
    { .id = SIP_ATTR_METHOD,        .name = "method", .desc = "Method", .dwidth = 15 },
    { .id = SIP_ATTR_SDP_ADDRESS,   .name = "sdpaddress", .desc = "SDP Address", .dwidth = 22 },
    { .id = SIP_ATTR_SDP_PORT,      .name = "sdpport", .desc = "SDP Port", .dwidth = 5 },
    { .id = SIP_ATTR_TRANSPORT,     .name = "transport", .title = "Trans", .desc = "Transport", .dwidth = 3 },
    { .id = SIP_ATTR_MSGCNT,        .name = "msgcnt", .title = "Msgs", .desc = "Message Count", .dwidth = 5 },
    { .id = SIP_ATTR_CALLSTATE,     .name = "state", .desc = "Call State", .dwidth = 10 },
    { .id = SIP_ATTR_CONVDUR,       .name = "convdur", .title = "ConvDur", .desc = "Conversation Duration", .dwidth = 7 },
    { .id = SIP_ATTR_TOTALDUR,      .name = "totaldur", .title = "TotalDur", .desc = "Total Duration", .dwidth = 8 }
};

sip_attr_hdr_t *
sip_attr_get_header(enum sip_attr_id id)
{
    return &attrs[id];
}

const char *
sip_attr_get_description(enum sip_attr_id id)
{
    sip_attr_hdr_t *header;
    if ((header = sip_attr_get_header(id))) {
        return header->desc;
    }
    return NULL;
}

const char *
sip_attr_get_title(enum sip_attr_id id)
{
    sip_attr_hdr_t *header;
    if ((header = sip_attr_get_header(id))) {
        if (header->title)
            return header->title;
        return header->desc;
    }
    return NULL;
}

const char *
sip_attr_get_name(enum sip_attr_id id)
{
    sip_attr_hdr_t *header;
    if ((header = sip_attr_get_header(id))) {
        return header->name;
    }
    return NULL;
}

int
sip_attr_get_width(enum sip_attr_id id)
{
    sip_attr_hdr_t *header;
    if ((header = sip_attr_get_header(id))) {
        return header->dwidth;
    }
    return 0;
}

enum sip_attr_id
sip_attr_from_name(const char *name) {
    int i;
    for (i = 0; i < SIP_ATTR_SENTINEL; i++)
    {
        if (!strcasecmp(name, attrs[i].name)) {
            return attrs[i].id;
        }
    }
    return -1;
}

void
sip_attr_list_destroy(char *attrs[])
{
    int i;
    for (i = 0; i < SIP_ATTR_SENTINEL; i++) {
        if (attrs[i])
            free(attrs[i]);
    }
}

void
sip_attr_set(char *attrs[], enum sip_attr_id id, const char *value)
{
    attrs[id] = strdup(value);
}

const char *
sip_attr_get(char *attrs[], enum sip_attr_id id)
{
    return attrs[id];
}


