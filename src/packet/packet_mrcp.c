/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2021 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2021 Irontec SL. All rights reserved.
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
 * @file packet_mrcp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 * @author Mayama Takeshi <mayamatakeshi@gmail.com>
 *
 * @brief functions to manage MRCPv2 protocol
 *
 */

#include "config.h"
#include "glib-extra/glib.h"
#include "packet.h"
#include "packet_mrcp.h"
#include "storage/storage.h"

G_DEFINE_TYPE(PacketDissectorMrcp, packet_dissector_mrcp, PACKET_TYPE_DISSECTOR)

/* @brief list of methods and responses */
PacketMrcpCode mrcp_codes[] = {
    { MRCP_METHOD_SET_PARAMS,                "SET-PARAMS" },
    { MRCP_METHOD_GET_PARAMS,                "GET-PARAMS" },
    { MRCP_METHOD_SPEAK,                     "SPEAK" },
    { MRCP_METHOD_STOP,                      "STOP" },
    { MRCP_METHOD_PAUSE,                     "PAUSE" },
    { MRCP_METHOD_RESUME,                    "RESUME" },
    { MRCP_METHOD_BARGE_IN_OCCURRED,         "BARGE-IN-OCCURRED" },
    { MRCP_METHOD_CONTROL,                   "CONTROL" },
    { MRCP_METHOD_DEFINE_LEXICON,            "DEFINE-LEXICON" },
    { MRCP_METHOD_DEFINE_GRAMMAR,            "DEFINE-GRAMMAR" },
    { MRCP_METHOD_RECOGNIZE,                 "RECOGNIZE" },
    { MRCP_METHOD_INTERPRET,                 "INTERPRET" },
    { MRCP_METHOD_GET_RESULT,                "GET-RESULT" },
    { MRCP_METHOD_START_INPUT_TIMERS,        "START-INPUT-TIMERS" },
    { MRCP_METHOD_START_PHRASE_ENROLLMENT,   "START-PHRASE-ENROLLMENT" },
    { MRCP_METHOD_ENROLLMENT_ROLLBACK,       "ENROLLMENT-ROLLBACK" },
    { MRCP_METHOD_END_PHRASE_ENROLLMENT,     "END_PHRASE-ENROLLMENT" },
    { MRCP_METHOD_MODIFY_PHRASE,             "MODIFY-PHRASE" },
    { MRCP_METHOD_DELETE_PHRASE,             "DELETE-PHRASE" },
    { MRCP_METHOD_RECORD,                    "RECORD" },
    { MRCP_METHOD_START_SESSION,             "START-SESSION" },
    { MRCP_METHOD_END_SESSION,               "END-SESSION" },
    { MRCP_METHOD_QUERY_VOICEPRINT,          "QUERY-VOICEPRINT" },
    { MRCP_METHOD_DELETE_VOICEPRINT,         "DELETE-VOICEPRINT" },
    { MRCP_METHOD_VERIFY,                    "VERIFY" },
    { MRCP_METHOD_VERIFY_FROM_BUFFER,        "VERIFY-FROM-BUFFER" },
    { MRCP_METHOD_VERIFY_ROLLBACK,           "VERIFY-ROLLBACK" },
    { MRCP_METHOD_CLEAR_BUFFER,              "CLEAR-BUFFER" },
    { MRCP_METHOD_GET_INTERMEDIATE_RESULT,   "INTERMEDIATE-RESULT" },
    { 0, NULL },
};

guint
packet_mrcp_method_from_str(const gchar *method)
{
    // Standard method
    for (guint i = 0; mrcp_codes[i].id > 0; i++) {
        if (g_strcmp0(method, mrcp_codes[i].text) == 0)
            return mrcp_codes[i].id;
    }
    return (guint) g_ascii_strtoull(method, NULL, 10);
}

PacketMrcpData *
packet_mrcp_data(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);

    // Get Packet mrcp data
    PacketMrcpData *mrcp = packet_get_protocol_data(packet, PACKET_PROTO_MRCP);
    g_return_val_if_fail(mrcp != NULL, NULL);

    return mrcp;
}

gchar *
packet_mrcp_payload_str(const Packet *packet)
{
    // Get Packet mrcp data
    PacketMrcpData *mrcp = packet_mrcp_data(packet);
    return g_strndup(
        g_bytes_get_data(mrcp->payload, NULL),
        g_bytes_get_size(mrcp->payload)
    );
}

const gchar *
packet_mrcp_method_str(const Packet *packet)
{
    PacketMrcpData *mrcp = packet_mrcp_data(packet);
    g_return_val_if_fail(mrcp != NULL, NULL);
    return mrcp->method;
}

guint
packet_mrcp_method(const Packet *packet)
{
    PacketMrcpData *mrcp = packet_mrcp_data(packet);
    g_return_val_if_fail(mrcp != NULL, 0);
    return mrcp->code;
}

guint64
packet_mrcp_request_id(const Packet *packet)
{
    // Get Packet mrcp data
    PacketMrcpData *mrcp = packet_mrcp_data(packet);
    g_return_val_if_fail(mrcp != NULL, 0);
    return mrcp->request_id;
}

gboolean
packet_mrcp_is_request(const Packet *packet)
{
    // Get Packet mrcp data
    PacketMrcpData *mrcp = packet_mrcp_data(packet);
    g_return_val_if_fail(mrcp != NULL, 0);
    return mrcp->type == MRCP_MESSAGE_REQUEST;
}

static GBytes *
packet_dissector_mrcp_dissect(PacketDissector *self, Packet *packet, GBytes *data)
{
    gchar *method = NULL;
    gchar *request_state = NULL;
    guint status_code = 0;
    guint64 request_id = 0;
    enum PacketMrcpMessageTypes type;

    // Ignore too small packets
    if (g_bytes_get_size(data) < MRCP_VERSION_LEN + 1)
        return data;

    // Convert payload to something we can parse with regular expressions
    g_autoptr(GString) payload = g_string_new_len(
        (const gchar *) g_bytes_get_data(data, NULL),
        g_bytes_get_size(data)
    );

    // All MRCP messages start with version string
    if (g_ascii_strncasecmp(payload->str, MRCP_VERSION, MRCP_VERSION_LEN) != 0)
        return data;

    // Split SIP payload in lines separated by CRLF
    g_auto(GStrv) payload_data = g_strsplit(payload->str, MRCP_CRLF, 0);

    if (g_strv_length(payload_data) == 0) {
        return data;
    }

    g_auto(GStrv) first_line_data = g_strsplit(payload_data[0], " ", -1);
    if (g_strv_length(first_line_data) < 4) {
        return data;
    }

    // request-line  = mrcp-version message-length method-name request-id
    // response-line = mrcp-version message-length request-id status-code request-state
    // event-line    = mrcp-version message-length event-name request-id request-state
    if (g_strv_length(first_line_data) == 4) {
        // This is a request line
        method = g_strdup(first_line_data[2]);
        request_id = g_ascii_strtoull(first_line_data[3], NULL, 10);
        type = MRCP_MESSAGE_REQUEST;
    } else {
        request_id = g_ascii_strtoull(first_line_data[2], NULL, 10);
        if (request_id != 0) {
            // This is a response line
            status_code = (guint) g_atoi(first_line_data[3]);
            request_state = g_strdup(first_line_data[4]);
            type = MRCP_MESSAGE_RESPONSE;
        } else {
            // This is a event line
            method = g_strdup(first_line_data[2]);
            request_id = g_ascii_strtoull(first_line_data[3], NULL, 10);
            type = MRCP_MESSAGE_EVENT;
        }
    }

    // No MRCP information in first line. Skip this packet.
    if (method == NULL && request_state == NULL) {
        return data;
    }

    // Allocate packet mrcp data
    PacketMrcpData *mrcp_data = g_malloc0(sizeof(PacketMrcpData));
    mrcp_data->proto.id = PACKET_PROTO_MRCP;
    if (method != NULL) {
        mrcp_data->code = packet_mrcp_method_from_str(method);
        mrcp_data->method = method;
    } else {
        mrcp_data->code = status_code;
        mrcp_data->method = g_strdup_printf("%d %s", status_code, request_state);
    }

    mrcp_data->payload = g_bytes_ref(data);
    mrcp_data->type = type;
    mrcp_data->request_id = request_id;

    // Add SIP information to the packet
    packet_set_protocol_data(packet, PACKET_PROTO_MRCP, mrcp_data);

    gsize mrcp_size = strlen(payload_data[0]) + 2 /* CRLF */;
    g_auto(GStrv) headers = g_strsplit(payload->str, MRCP_CRLF, 0);
    for (guint i = 1; i < g_strv_length(headers); i++) {
        const gchar *line = headers[i];

        // End of MRCP payload
        if (g_strcmp0(line, "") == 0) {
            mrcp_size += 2 /* Final CRLF */;
            break;
        }

        // MRCP Headers Size
        mrcp_size += strlen(line) + 2 /* CRLF */;

        g_auto(GStrv) hdr_data = g_strsplit(line, ":", 2);
        if (g_strv_length(hdr_data) != 2) {
            break;
        }

        gchar *hdr_name = g_strstrip(hdr_data[0]);
        gchar *hdr_value = g_strstrip(hdr_data[1]);


        if (strcasecmp(hdr_name, "channel-identifier") == 0) {
            mrcp_data->channel = g_strdup(hdr_value);
        } else if (strcasecmp(hdr_name, "content-length") == 0) {
            mrcp_data->content_len = g_ascii_strtoull(hdr_value, NULL, 10);
        }
    }

    // Check we have a valid MRCP packet
    if (mrcp_data->channel == NULL) {
        return data;
    }

    // Check we have a whole packet
    if (mrcp_data->content_len != g_bytes_get_size(data) - mrcp_size) {
        return data;
    }

    // Handle bad terminated SIP messages
    if (mrcp_size > g_bytes_get_size(data))
        mrcp_size = g_bytes_get_size(data);

    // Remove SIP headers from data
    data = g_bytes_offset(data, mrcp_size);

    // Pass data to sub-dissectors
    packet_dissector_next(self, packet, data);

    // Add data to storage
    storage_add_packet(packet);

    return data;
}

static void
packet_dissector_mrcp_free(Packet *packet)
{
    PacketMrcpData *mrcp_data = packet_mrcp_data(packet);
    g_return_if_fail(mrcp_data != NULL);

    g_bytes_unref(mrcp_data->payload);
    g_free(mrcp_data->channel);
    g_free(mrcp_data->method);
    g_free(mrcp_data);
}

static void
packet_dissector_mrcp_class_init(PacketDissectorMrcpClass *klass)
{
    PacketDissectorClass *dissector_class = PACKET_DISSECTOR_CLASS(klass);
    dissector_class->dissect = packet_dissector_mrcp_dissect;
    dissector_class->free_data = packet_dissector_mrcp_free;
}

static void
packet_dissector_mrcp_init(G_GNUC_UNUSED PacketDissectorMrcp *self)
{
}

PacketDissector *
packet_dissector_mrcp_new()
{
    return g_object_new(
        PACKET_DISSECTOR_TYPE_MRCP,
        "id", PACKET_PROTO_MRCP,
        "name", "MRCP",
        NULL
    );
}
