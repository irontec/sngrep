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
 * @file attribute.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in attribute.h
 *
 */
#include "config.h"
#include <glib.h>
#include "glib-extra/glib.h"
#include "attribute.h"
#include "packet/packet_sip.h"
#include "tui/tui.h"

static GPtrArray *attributes = NULL;

Attribute *
attribute_new(gchar *name, gchar *title, gchar *desc, gint length)
{
    Attribute *attr = g_malloc0(sizeof(Attribute));
    attr->name = name;
    attr->title = (title != NULL) ? title : desc;
    attr->desc = desc;
    attr->mutable = FALSE;
    attr->length = length;
    return attr;
}

void
attribute_set_color_func(Attribute *attr, AttributeColorFunc func)
{
    g_return_if_fail(attr != NULL);
    attr->colorFunc = func;
}

void
attribute_set_getter_func(Attribute *attr, AttributeGetterFunc func)
{
    g_return_if_fail(attr != NULL);
    attr->getterFunc = func;
}

void
attribute_set_mutable(Attribute *attr, gboolean mutable)
{
    g_return_if_fail(attr != NULL);
    attr->mutable = mutable;
}

const gchar *
attribute_get_description(Attribute *attribute)
{
    return attribute->desc;
}

void
attribute_set_description(Attribute *attr, const gchar *desc)
{
    attr->desc = (gchar *) desc;
}

const gchar *
attribute_get_title(Attribute *attr)
{
    return attr->title;
}

void
attribute_set_title(Attribute *attr, const gchar *title)
{
    attr->title = (gchar *) title;
}

const gchar *
attribute_get_name(Attribute *attr)
{
    return attr->name;
}

gint
attribute_get_length(Attribute *attr)
{
    return attr->length;
}

void
attribute_set_length(Attribute *attr, gint length)
{
    attr->length = length;
}

Attribute *
attribute_find_by_name(const gchar *name)
{
    for (guint i = 0; i < g_ptr_array_len(attributes); i++) {
        Attribute *attr = g_ptr_array_index(attributes, i);
        if (g_strcmp0(name, attr->name) == 0) {
            return attr;
        }
    }
    return NULL;
}

gint
attribute_get_color(Attribute *attr, const gchar *value)
{

    if (!setting_enabled(SETTING_TUI_CL_COLORATTR))
        return 0;

    if (attr->colorFunc) {
        return attr->colorFunc(value);
    }

    return 0;
}

gchar *
attribute_get_value(Attribute *attr, Message *msg)
{
    g_return_val_if_fail(msg != NULL, NULL);
    g_return_val_if_fail(attr != NULL, NULL);

    if (attr->getterFunc) {
        return attr->getterFunc(attr, msg);
    }

    return NULL;
}

gint
attribute_color_sip_method(const gchar *value)
{
    switch (packet_sip_method_from_str(value)) {
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
attribute_color_call_state(const gchar *value)
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
attribute_regex_value_getter(Attribute *attr, Message *msg)
{
    gchar *ret = NULL;

    g_return_val_if_fail(msg != NULL, NULL);

    g_autofree gchar *payload = msg_get_payload(msg);
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
        date_time_to_duration(
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
        date_time_to_duration(
            msg_get_time(g_ptr_array_first(call->msgs)),
            msg_get_time(g_ptr_array_last(call->msgs)),
            value)
    );
}

static gchar *
attribute_getter_msg_source(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    const Address src = msg_src_address(msg);
    return g_strdup_printf(
        "%s:%u",
        address_get_ip(src),
        address_get_port(src)
    );
}

static gchar *
attribute_getter_msg_destination(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    const Address dst = msg_dst_address(msg);
    return g_strdup_printf(
        "%s:%u",
        address_get_ip(dst),
        address_get_port(dst)
    );
}

static gchar *
attribute_getter_msg_date(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    gchar value[11];
    return g_strdup(date_time_date_to_str(msg_get_time(msg), value));
}

static gchar *
attribute_getter_msg_time(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    gchar value[16];
    return g_strdup(date_time_time_to_str(msg_get_time(msg), value));
}

static gchar *
attribute_getter_msg_method(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    return g_strdup(msg_get_method_str(msg));
}

static gchar *
attribute_getter_msg_transport(G_GNUC_UNUSED Attribute *attr, Message *msg)
{
    return g_strdup(packet_transport(msg->packet));
}


void
attribute_set_regex_pattern(Attribute *attr, const gchar *pattern)
{
    g_return_if_fail(attr != NULL);
    attr->regexp_pattern = pattern;

    attr->regex = g_regex_new(
        attr->regexp_pattern,
        G_REGEX_OPTIMIZE | G_REGEX_CASELESS | G_REGEX_NEWLINE_CRLF | G_REGEX_MULTILINE,
        G_REGEX_MATCH_NEWLINE_CRLF,
        NULL
    );

    attr->getterFunc = attribute_regex_value_getter;
}

AttributeValue *
attribute_value_new(Attribute *attr, gchar *value)
{
    AttributeValue *attr_value = g_malloc0(sizeof(AttributeValue));
    attr_value->attr = attr;
    attr_value->value = value;
    return attr_value;
}

void
attribute_value_free(AttributeValue *attr_value)
{
    g_free(attr_value->value);
    g_free(attr_value);
}

GPtrArray *
attribute_get_internal_array()
{
    return attributes;
}

void
attribute_from_setting(const gchar *setting, const gchar *value)
{
    GStrv attr_data = g_strsplit(setting, ".", 2);
    if (g_strv_length(attr_data) < 2) {
        g_printerr("Invalid attribute setting %s\n", setting);
        return;
    }

    Attribute *attribute = attribute_find_by_name(attr_data[0]);
    if (attribute == NULL) {
        attribute = attribute_new(attr_data[0], attr_data[0], attr_data[0], 20);
        g_ptr_array_add(attributes, attribute);
    }

    if (g_strcmp0(attr_data[1], "title") == 0) {
        attribute_set_title(attribute, value);
    }

    if (g_strcmp0(attr_data[1], "desc") == 0) {
        attribute_set_description(attribute, value);
    }

    if (g_strcmp0(attr_data[1], "regexp") == 0) {
        attribute_set_regex_pattern(attribute, value);
    }

    if (g_strcmp0(attr_data[1], "length") == 0) {
        attribute_set_length(attribute, g_atoi(value));
    }
}

void
attribute_init()
{
    Attribute *attribute = NULL;
    attributes = g_ptr_array_new();

    //! Call Index
    attribute = attribute_new("index", "Idx", "Call Index", 4);
    attribute_set_getter_func(attribute, attribute_getter_call_index);
    g_ptr_array_add(attributes, attribute);

    //! From SIP header
    attribute = attribute_new("sipfrom", NULL, "SIP From", 25);
    attribute_set_regex_pattern(attribute, "^(From|f):[^:]+:(?P<value>([^@;>\r]+@)?[^;>\r]+)");
    g_ptr_array_add(attributes, attribute);

    //! From SIP header (URI user part)
    attribute = attribute_new("sipfromuser", NULL, "SIP From User", 20);
    attribute_set_regex_pattern(attribute, "^(From|f):[^:]+:(?P<value>[^@;>\r]+)");
    g_ptr_array_add(attributes, attribute);

    //! To SIP header
    attribute = attribute_new("sipto", NULL, "SIP To", 25);
    attribute_set_regex_pattern(attribute, "^(To|t):[^:]+:(?P<value>([^@;>\r]+@)?[^\r;>]+)");
    g_ptr_array_add(attributes, attribute);

    //! To SIP header (URI user part )
    attribute = attribute_new("siptouser", NULL, "SIP To User", 20);
    attribute_set_regex_pattern(attribute, "^(To|t):[^:]+:(?P<value>[^@;>\r]+)");
    g_ptr_array_add(attributes, attribute);

    //! Source ip:port address
    attribute = attribute_new("src", NULL, "Source", 22);
    attribute_set_getter_func(attribute, attribute_getter_msg_source);
    g_ptr_array_add(attributes, attribute);

    //! Destination ip:port address
    attribute = attribute_new("dst", NULL, "Destination", 22);
    attribute_set_getter_func(attribute, attribute_getter_msg_destination);
    g_ptr_array_add(attributes, attribute);

    //! Call-Id SIP header
    attribute = attribute_new("callid", NULL, "Call-ID", 50);
    attribute_set_regex_pattern(attribute, "^(Call-ID|i):\\s*(?P<value>.+)$");
    g_ptr_array_add(attributes, attribute);

    //! X-Call-Id SIP header
    attribute = attribute_new("xcallid", NULL, "X-Call-ID", 50);
    attribute_set_regex_pattern(attribute, "^(X-Call-ID|X-CID):\\s*(?P<value>.+)$");
    g_ptr_array_add(attributes, attribute);

    //! Packet captured date
    attribute = attribute_new("date", NULL, "Date", 10);
    attribute_set_getter_func(attribute, attribute_getter_msg_date);
    g_ptr_array_add(attributes, attribute);

    //! Packet captured time
    attribute = attribute_new("time", NULL, "Time", 8);
    attribute_set_getter_func(attribute, attribute_getter_msg_time);
    g_ptr_array_add(attributes, attribute);

    //! SIP Method
    attribute = attribute_new("method", NULL, "Method", 8);
    attribute_set_getter_func(attribute, attribute_getter_msg_method);
    attribute_set_color_func(attribute, attribute_color_sip_method);
    g_ptr_array_add(attributes, attribute);

    //! SIP Transport (SIP over TCP, UDP, WS, ...)
    attribute = attribute_new("transport", "Trans", "Transport", 3);
    attribute_set_getter_func(attribute, attribute_getter_msg_transport);
    g_ptr_array_add(attributes, attribute);

    //! Owner call message count
    attribute = attribute_new("msgcnt", "Msgs", "Message Count", 4);
    attribute_set_getter_func(attribute, attribute_getter_call_msgcnt);
    attribute_set_mutable(attribute, TRUE);
    g_ptr_array_add(attributes, attribute);

    //! Owner call state
    attribute = attribute_new("state", NULL, "Call-State", 12);
    attribute_set_getter_func(attribute, attribute_getter_call_state);
    attribute_set_color_func(attribute, attribute_color_call_state);
    attribute_set_mutable(attribute, TRUE);
    g_ptr_array_add(attributes, attribute);

    //! Conversation duration (from first 200 OK)
    attribute = attribute_new("convdur", "ConvDur", "Conversation Duration", 7);
    attribute_set_getter_func(attribute, attribute_getter_call_convdur);
    attribute_set_mutable(attribute, TRUE);
    g_ptr_array_add(attributes, attribute);

    //! Total duration (from first to last message in dialog)
    attribute = attribute_new("totaldur", "TotalDur", "Total Duration", 8);
    attribute_set_getter_func(attribute, attribute_getter_call_totaldur);
    attribute_set_mutable(attribute, TRUE);
    g_ptr_array_add(attributes, attribute);

    //! Reason SIP header
    attribute = attribute_new("reason", "Reason", "Reason Text", 25);
    attribute_set_regex_pattern(attribute, "Reason:[ ]*[^\r]*;text=\"(?<value>[^\r]+)\"");
    g_ptr_array_add(attributes, attribute);

    //! Warning SIP header
    attribute = attribute_new("warning", "Warning", "Warning Code", 4);
    attribute_set_regex_pattern(attribute, "^Warning:\\s*(?P<value>\\d+)");
    g_ptr_array_add(attributes, attribute);
}

void
attribute_deinit()
{
    for (guint i = 0; i < g_ptr_array_len(attributes); i++) {
        Attribute *attribute = g_ptr_array_index(attributes, i);
        if (attribute->regex != NULL) {
            g_regex_unref(attribute->regex);
        }
    }
}

void
attribute_dump()
{
    g_print("\nAttribute List\n===============\n");
    for (guint i = 0; i < g_ptr_array_len(attributes); i++) {
        Attribute *attr = g_ptr_array_index(attributes, i);
        g_print("Attribute: %-15s Description: %-25s Getter: %s\n",
                attr->name,
                attr->desc,
                (attr->regex) ? g_strescape(attr->regexp_pattern, "\\") : "internal"
        );
    }
}
