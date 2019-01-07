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
 * @file attribute.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in attribute.h
 *
 */
#include "config.h"
#include <glib.h>
#include "glib-extra.h"
#include "attribute.h"
#include "packet/dissectors/packet_sip.h"
#include "ncurses/manager.h"

static GPtrArray *attributes = NULL;

Attribute *
attr_new(gchar *name, gchar *title, gchar *desc)
{
    Attribute *attr = g_malloc0(sizeof(Attribute));
    attr->name = name;
    attr->title = title;
    attr->desc = desc;
    attr->mutable = FALSE;
    return attr;
}

void
attr_set_color_func(Attribute *attr, AttributeColorFunc func)
{
    g_return_if_fail(attr != NULL);
    attr->colorFunc = func;
}

void
attr_set_getter_func(Attribute *attr, AttributeGetterFunc func)
{
    g_return_if_fail(attr != NULL);
    attr->getterFunc = func;
}

void
attr_set_mutable(Attribute *attr, gboolean mutable)
{
    g_return_if_fail(attr != NULL);
    attr->mutable = mutable;
}

Attribute *
attr_header(enum AttributeId id)
{
    //! FIXME Do not use id as index
    return g_ptr_array_index(attributes, id);
}

const gchar *
attr_description(enum AttributeId id)
{
    Attribute *header;
    if ((header = attr_header(id))) {
        return header->desc;
    }
    return NULL;
}

const gchar *
attr_title(enum AttributeId id)
{
    Attribute *header;
    if ((header = attr_header(id))) {
        if (header->title)
            return header->title;
        return header->desc;
    }
    return NULL;
}

const gchar *
attr_name(enum AttributeId id)
{
    Attribute *header;
    if ((header = attr_header(id))) {
        return header->name;
    }
    return NULL;
}

gint
attr_find_by_name(const gchar *name)
{
    for (guint i = 0; i < g_ptr_array_len(attributes); i++) {
        Attribute *attr = g_ptr_array_index(attributes, i);
        if (!g_strcmp0(name, attr->name)) {
            return i;
        }
    }
    return -1;
}

gint
attr_color(enum AttributeId id, const gchar *value)
{
    Attribute *header;

    if (!setting_enabled(SETTING_CL_COLORATTR))
        return 0;

    if ((header = attr_header(id))) {
        if (header->colorFunc) {
            return header->colorFunc(value);
        }
    }
    return 0;
}

const gchar *
attr_get_value(const gchar *name, Message *msg)
{
    g_return_val_if_fail(name != NULL, NULL);
    g_return_val_if_fail(msg != NULL, NULL);

    Attribute *attr = attr_header(attr_find_by_name(name));
    g_return_val_if_fail(attr != NULL, NULL);

    gchar *value = NULL;
    if (!attr->mutable) {
        value = msg_get_cached_attribute(msg, attr);
    }

    if (value == NULL && attr->getterFunc) {
        value = attr->getterFunc(attr, msg);
        msg_set_cached_attribute(msg, attr, value);
    }

    return value;
}

gint
attr_color_sip_method(const gchar *value)
{
    switch (sip_method_from_str(value)) {
        case SIP_METHOD_INVITE:
            return COLOR_PAIR(CP_RED_ON_DEF) | A_BOLD;
        case SIP_METHOD_NOTIFY:
            return COLOR_PAIR(CP_YELLOW_ON_DEF);
        case SIP_METHOD_OPTIONS:
            return COLOR_PAIR(CP_YELLOW_ON_DEF);
        case SIP_METHOD_REGISTER:
            return COLOR_PAIR(CP_MAGENTA_ON_DEF);
        case SIP_METHOD_SUBSCRIBE:
            return COLOR_PAIR(CP_BLUE_ON_DEF);
        default:
            return 0;
    }
}

gint
attr_color_call_state(const gchar *value)
{
    if (!strcmp(value, call_state_to_str(CALL_STATE_CALLSETUP)))
        return COLOR_PAIR(CP_YELLOW_ON_DEF);
    if (!strcmp(value, call_state_to_str(CALL_STATE_INCALL)))
        return COLOR_PAIR(CP_BLUE_ON_DEF);
    if (!strcmp(value, call_state_to_str(CALL_STATE_COMPLETED)))
        return COLOR_PAIR(CP_GREEN_ON_DEF);
    if (!strcmp(value, call_state_to_str(CALL_STATE_CANCELLED)))
        return COLOR_PAIR(CP_RED_ON_DEF);
    if (!strcmp(value, call_state_to_str(CALL_STATE_REJECTED)))
        return COLOR_PAIR(CP_RED_ON_DEF);
    if (!strcmp(value, call_state_to_str(CALL_STATE_BUSY)))
        return COLOR_PAIR(CP_MAGENTA_ON_DEF);
    if (!strcmp(value, call_state_to_str(CALL_STATE_DIVERTED)))
        return COLOR_PAIR(CP_CYAN_ON_DEF);
    return 0;
}

static gchar *
attr_regex_value_getter(Attribute *attr, Message *msg)
{
    gchar *ret = NULL;

    g_return_val_if_fail(msg != NULL, NULL);

    const gchar *payload = msg_get_payload(msg);
    g_return_val_if_fail(payload != NULL, NULL);

    // Response code
    GMatchInfo *pmatch;
    if (g_regex_match(attr->regex, payload, 0, &pmatch)) {
        ret = g_match_info_fetch_named(pmatch, "value");
    }
    g_match_info_free(pmatch);
    return ret;
}

static gchar *
attribute_getter_call_index(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    Call *call = msg_get_call(msg);
    return g_strdup_printf("%d", call->index);
}

static gchar *
attribute_getter_call_msgcnt(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    Call *call = msg_get_call(msg);
    return g_strdup_printf("%d", call_msg_count(call));
}

static gchar *
attribute_getter_call_state(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    Call *call = msg_get_call(msg);
    return g_strdup(call_state_to_str(call->state));
}

static gchar *
attribute_getter_call_convdur(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    gchar value[10];
    Call *call = msg_get_call(msg);
    return g_strdup(
        timeval_to_duration(
            msg_get_time(call->cstart_msg),
            msg_get_time(call->cend_msg),
            value)
    );
}

static gchar *
attribute_getter_call_totaldur(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    gchar value[10];
    Call *call = msg_get_call(msg);
    return g_strdup(
        timeval_to_duration(
            msg_get_time(g_ptr_array_first(call->msgs)),
            msg_get_time(g_ptr_array_last(call->msgs)),
            value)
    );
}

static gchar *
attribute_getter_msg_source(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    Address src = msg_src_address(msg);
    return g_strdup_printf("%s:%u", src.ip, src.port);
}

static gchar *
attribute_getter_msg_destination(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    Address dst = msg_dst_address(msg);
    return g_strdup_printf("%s:%u", dst.ip, dst.port);
}

static gchar *
attribute_getter_msg_date(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    gchar value[11];
    return g_strdup(timeval_to_date(msg_get_time(msg), value));
}

static gchar *
attribute_getter_msg_time(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    gchar value[15];
    return g_strdup(timeval_to_time(msg_get_time(msg), value));
}

static gchar *
attribute_getter_msg_transport(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    return g_strdup(packet_transport(msg->packet));
}


void
attr_set_regex_pattern(Attribute *attr, gchar *pattern)
{
    g_return_if_fail(attr != NULL);
    attr->regexp_pattern = pattern;

    attr->regex = g_regex_new(
        attr->regexp_pattern,
        G_REGEX_OPTIMIZE | G_REGEX_CASELESS | G_REGEX_NEWLINE_CRLF | G_REGEX_MULTILINE,
        G_REGEX_MATCH_NEWLINE_CRLF,
        NULL
    );

    attr->getterFunc = attr_regex_value_getter;
}

void
attribute_init()
{
    Attribute *attribute = NULL;
    attributes = g_ptr_array_new();

    //! Call Index
    attribute = attr_new("index", "Idx", "Call Index");
    attr_set_getter_func(attribute, attribute_getter_call_index);
    g_ptr_array_add(attributes, attribute);

    //! From SIP header
    attribute = attr_new("sipfrom", NULL, "SIP From");
    attr_set_regex_pattern(attribute, "^(From|f):[^:]+:(?P<value>([^@;>\r]+@)?[^;>\r]+)");
    g_ptr_array_add(attributes, attribute);

    //! From SIP header (URI user part)
    attribute = attr_new("sipfromuser", NULL, "SIP From User");
    attr_set_regex_pattern(attribute, "^(From|f):[^:]+:(?P<value>[^@;>\r]+)");
    g_ptr_array_add(attributes, attribute);

    //! To SIP header
    attribute = attr_new("sipto", NULL, "SIP To");
    attr_set_regex_pattern(attribute, "^(To|t):[^:]+:(?P<value>([^@;>\r]+@)?[^\r;>]+)");
    g_ptr_array_add(attributes, attribute);

    //! To SIP header (URI user part )
    attribute = attr_new("siptouser", NULL, "SIP To User");
    attr_set_regex_pattern(attribute, "^(To|t):[^:]+:(?P<value>[^@;>\r]+)");
    g_ptr_array_add(attributes, attribute);

    //! Source ip:port address
    attribute = attr_new("src", NULL, "Source");
    attr_set_getter_func(attribute, attribute_getter_msg_source);
    g_ptr_array_add(attributes, attribute);

    //! Destination ip:port address
    attribute = attr_new("dst", NULL, "Destination");
    attr_set_getter_func(attribute, attribute_getter_msg_destination);
    g_ptr_array_add(attributes, attribute);

    //! Call-Id SIP header
    attribute = attr_new("callid", NULL, "Call-ID");
    attr_set_regex_pattern(attribute, "^(Call-ID|i):\\s*(?P<value>.+)$");
    g_ptr_array_add(attributes, attribute);

    //! X-Call-Id SIP header
    attribute = attr_new("xcallid", NULL, "X-Call-ID");
    attr_set_regex_pattern(attribute, "^(X-Call-ID|X-CID):\\s*(?P<xcallid>.+)$");
    g_ptr_array_add(attributes, attribute);

    //! Packet captured date
    attribute = attr_new("date", NULL, "Date");
    attr_set_getter_func(attribute, attribute_getter_msg_date);
    g_ptr_array_add(attributes, attribute);

    //! Packet captured time
    attribute = attr_new("time", NULL, "Time");
    attr_set_getter_func(attribute, attribute_getter_msg_time);
    g_ptr_array_add(attributes, attribute);

    //! SIP Method
    attribute = attr_new("method", NULL, "Method");
    attr_set_regex_pattern(attribute, "(?P<value>\\w+) [^:]+:\\S* SIP/2.0");
    attr_set_color_func(attribute, attr_color_sip_method);
    g_ptr_array_add(attributes, attribute);

    //! SIP Transport (SIP over TCP, UDP, WS, ...)
    attribute = attr_new("transport", "Trans", "Transport");
    attr_set_getter_func(attribute, attribute_getter_msg_transport);
    g_ptr_array_add(attributes, attribute);

    //! Owner call message count
    attribute = attr_new("msgcnt", "Msgs", "Message Count");
    attr_set_getter_func(attribute, attribute_getter_call_msgcnt);
    attr_set_mutable(attribute, TRUE);
    g_ptr_array_add(attributes, attribute);

    //! Owner call state
    attribute = attr_new("state", NULL, "Call-State");
    attr_set_getter_func(attribute, attribute_getter_call_state);
    attr_set_color_func(attribute, attr_color_call_state);
    g_ptr_array_add(attributes, attribute);

    //! Conversation duration (from first 200 OK)
    attribute = attr_new("convdur", "ConvDur", "Conversation Duration");
    attr_set_getter_func(attribute, attribute_getter_call_convdur);
    attr_set_mutable(attribute, TRUE);
    g_ptr_array_add(attributes, attribute);

    //! Total duration (from first to last message in dialog)
    attribute = attr_new("totaldur", "TotalDur", "Total Duration");
    attr_set_getter_func(attribute, attribute_getter_call_totaldur);
    attr_set_mutable(attribute, TRUE);
    g_ptr_array_add(attributes, attribute);

    //! Reason SIP header
    attribute = attr_new("reason", "Reason", "Reason Text");
    attr_set_regex_pattern(attribute, "Reason:[ ]*[^\r]*;text=\"(?<value>[^\r]+)\"");
    g_ptr_array_add(attributes, attribute);

    //! Warning SIP header
    attribute = attr_new("warning", "Warning", "Warning Code");
    attr_set_regex_pattern(attribute, "^Warning:\\s*(?P<value>\\d+)");
    g_ptr_array_add(attributes, attribute);
}
