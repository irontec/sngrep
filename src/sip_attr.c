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
#include "option.h"
#include "sip_attr.h"

static sip_attr_hdr_t attrs[] =
    {
          {
            .id = SIP_ATTR_SIPFROM,
            .name = "sipfrom",
            .desc = "SIP From",
            .dwidth = 30, },
          {
            .id = SIP_ATTR_SIPTO,
            .name = "sipto",
            .desc = "SIP To",
            .dwidth = 30, },
          {
            .id = SIP_ATTR_SRC,
            .name = "src",
            .desc = "Source",
            .dwidth = 22, },
          {
            .id = SIP_ATTR_SRC_HOST,
            .name = "srchost",
            .desc = "Source Host",
            .dwidth = 16, },
          {
            .id = SIP_ATTR_DST,
            .name = "dst",
            .desc = "Destination",
            .dwidth = 22, },
          {
            .id = SIP_ATTR_DST_HOST,
            .name = "dsthost",
            .desc = "Destination Host",
            .dwidth = 16, },
          {
            .id = SIP_ATTR_CALLID,
            .name = "callid",
            .desc = "Call-ID",
            .dwidth = 50, },
          {
            .id = SIP_ATTR_XCALLID,
            .name = "xcallid",
            .desc = "X-Call-ID",
            .dwidth = 50, },
          {
            .id = SIP_ATTR_DATE,
            .name = "date",
            .desc = "Date",
            .dwidth = 10, },
          {
            .id = SIP_ATTR_TIME,
            .name = "time",
            .desc = "Time",
            .dwidth = 8, },
          {
            .id = SIP_ATTR_METHOD,
            .name = "method",
            .desc = "Method",
            .dwidth = 15, },
          {
            .id = SIP_ATTR_REQUEST,
            .name = "request",
            .desc = "Request",
            .dwidth = 3, },
          {
            .id = SIP_ATTR_CSEQ,
            .name = "CSeq",
            .desc = "CSeq",
            .dwidth = 6, },
          {
            .id = SIP_ATTR_SDP,
            .name = "sdp",
            .desc = "Has SDP",
            .dwidth = 3, },
          {
            .id = SIP_ATTR_SDP_ADDRESS,
            .name = "sdpaddress",
            .desc = "SDP Address",
            .dwidth = 22, },
          {
            .id = SIP_ATTR_SDP_PORT,
            .name = "sdpport",
            .desc = "SDP Port",
            .dwidth = 5, },
          {
            .id = SIP_ATTR_TRANSPORT,
            .name = "transport",
            .desc = "Trans",
            .dwidth = 3, },
          {
            .id = SIP_ATTR_STARTING,
            .name = "starting",
            .desc = "Starting",
            .dwidth = 10, },
          {
            .id = SIP_ATTR_MSGCNT,
            .name = "msgcnt",
            .desc = "Msgs",
            .dwidth = 5, },
          {
            .id = SIP_ATTR_CALLSTATE,
            .name = "state",
            .desc = "Call State",
            .dwidth = 10 } };

sip_attr_hdr_t *
sip_attr_get_header(enum sip_attr_id id)
{
    int i;
    for (i = 0; i < SIP_ATTR_SENTINEL; i++) {
        if (id == attrs[i].id) {
            return &attrs[i];
        }
    }
    return NULL;
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
sip_attr_from_name(const char *name)
{
    int i;
    for (i = 0; i < SIP_ATTR_SENTINEL; i++) {
        if (!strcasecmp(name, attrs[i].name)) {
            return attrs[i].id;
        }
    }
    return -1;
}

void
sip_attr_list_destroy(sip_attr_t *list)
{
    sip_attr_t *attr;

    // If attribute already exists change its value
    while (list) {
        attr = list;
        list = attr->next;
        // Free attribute value
        free(attr->value);
        // Free attribute structure
        free(attr);
    }
}

void
sip_attr_set(sip_attr_t **list, enum sip_attr_id id, const char *value)
{
    sip_attr_t *attr;

    // If attribute already exists change its value
    for (attr = *list; attr; attr = attr->next) {
        if (id == attr->hdr->id) {
            // Free previous value
            free(attr->value);
            // Store the new value
            attr->value = strdup(value);
            return;
        }
    }

    // Otherwise add a new attribute
    if (!(attr = malloc(sizeof(sip_attr_t))))
        return;

    attr->hdr = sip_attr_get_header(id);
    attr->value = strdup(value);
    attr->next = *list;
    *list = attr;
}

const char *
sip_attr_get(sip_attr_t *list, enum sip_attr_id id)
{
    sip_attr_t *attr;
    for (attr = list; attr; attr = attr->next) {
        if (id == attr->hdr->id) {
            return attr->value;
        }
    }
    return NULL;
}

void
call_set_attribute(sip_call_t *call, enum sip_attr_id id, const char *value)
{
    sip_attr_set(&call->attrs, id, value);
}

const char *
call_get_attribute(sip_call_t *call, enum sip_attr_id id)
{
    if (!call)
        return NULL;

    switch (id) {
    case SIP_ATTR_MSGCNT:
    case SIP_ATTR_CALLSTATE:
        return sip_attr_get(call->attrs, id);
    case SIP_ATTR_STARTING:
        return msg_get_attribute(call_get_next_msg(call, NULL), SIP_ATTR_METHOD);
    default:
        return msg_get_attribute(call_get_next_msg(call, NULL), id);
    }

    return NULL;
}

void
msg_set_attribute(sip_msg_t *msg, enum sip_attr_id id, const char *value)
{
    sip_attr_set(&msg->attrs, id, value);
}

const char *
msg_get_attribute(sip_msg_t *msg, enum sip_attr_id id)
{
    if (!msg)
        return NULL;

    return sip_attr_get(msg->attrs, id);
}

int
sip_check_call_ignore(sip_call_t *call)
{
    int i;
    char filter_option[80];
    const char *filter;

    // Check if an ignore option exists
    for (i = 0; i < SIP_ATTR_SENTINEL; i++) {
        if (is_ignored_value(attrs[i].name, call_get_attribute(call, attrs[i].id))) {
            return 1;
        }
    }

    // Check enabled filters
    if (is_option_enabled("filter.enable")) {
        if ((filter = get_option_value("filter.sipfrom")) && strlen(filter)) {
            if (strstr(call_get_attribute(call, SIP_ATTR_SIPFROM), filter) == NULL) {
                return 1;
            }
        }
        if ((filter = get_option_value("filter.sipto")) && strlen(filter)) {
            if (strstr(call_get_attribute(call, SIP_ATTR_SIPTO), filter) == NULL) {
                return 1;
            }
        }
        if ((filter = get_option_value("filter.src")) && strlen(filter)) {
            if (strncasecmp(filter, call_get_attribute(call, SIP_ATTR_SRC), strlen(filter))) {
                return 1;
            }
        }
        if ((filter = get_option_value("filter.dst")) && strlen(filter)) {
            if (strncasecmp(filter, call_get_attribute(call, SIP_ATTR_DST), strlen(filter))) {
                return 1;
            }
        }

        // Check if a filter option exists
        memset(filter_option, 0, sizeof(filter_option));
        sprintf(filter_option, "filter.%s", call_get_attribute(call, SIP_ATTR_STARTING));
        if (!is_option_enabled(filter_option)) {
            return 1;
        }
    }
    return 0;
}
