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
 * @file packet_sip.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_sip.h
 */

#include "config.h"
#include <stdlib.h>
#include "glib/glib-extra.h"
#include "storage/storage.h"
#include "capture/packet.h"
#include "packet_tcp.h"
#include "packet_sdp.h"
#include "packet_sip.h"

/* @brief list of methods and responses */
PacketSipCode sip_codes[] = {
    { SIP_METHOD_REGISTER,  "REGISTER" },
    { SIP_METHOD_INVITE,    "INVITE" },
    { SIP_METHOD_SUBSCRIBE, "SUBSCRIBE" },
    { SIP_METHOD_NOTIFY,    "NOTIFY" },
    { SIP_METHOD_OPTIONS,   "OPTIONS" },
    { SIP_METHOD_PUBLISH,   "PUBLISH" },
    { SIP_METHOD_MESSAGE,   "MESSAGE" },
    { SIP_METHOD_CANCEL,    "CANCEL" },
    { SIP_METHOD_BYE,       "BYE" },
    { SIP_METHOD_ACK,       "ACK" },
    { SIP_METHOD_PRACK,     "PRACK" },
    { SIP_METHOD_INFO,      "INFO" },
    { SIP_METHOD_REFER,     "REFER" },
    { SIP_METHOD_UPDATE,    "UPDATE" },
    { 100,                  "100 Trying" },
    { 180,                  "180 Ringing" },
    { 181,                  "181 Call is Being Forwarded" },
    { 182,                  "182 Queued" },
    { 183,                  "183 Session Progress" },
    { 199,                  "199 Early Dialog Terminated" },
    { 200,                  "200 OK" },
    { 202,                  "202 Accepted" },
    { 204,                  "204 No Notification" },
    { 300,                  "300 Multiple Choices" },
    { 301,                  "301 Moved Permanently" },
    { 302,                  "302 Moved Temporarily" },
    { 305,                  "305 Use Proxy" },
    { 380,                  "380 Alternative Service" },
    { 400,                  "400 Bad Request" },
    { 401,                  "401 Unauthorized" },
    { 402,                  "402 Payment Required" },
    { 403,                  "403 Forbidden" },
    { 404,                  "404 Not Found" },
    { 405,                  "405 Method Not Allowed" },
    { 406,                  "406 Not Acceptable" },
    { 407,                  "407 Proxy Authentication Required" },
    { 408,                  "408 Request Timeout" },
    { 409,                  "409 Conflict" },
    { 410,                  "410 Gone" },
    { 411,                  "411 Length Required" },
    { 412,                  "412 Conditional Request Failed" },
    { 413,                  "413 Request Entity Too Large" },
    { 414,                  "414 Request-URI Too Long" },
    { 415,                  "415 Unsupported Media Type" },
    { 416,                  "416 Unsupported URI Scheme" },
    { 417,                  "417 Unknown Resource-Priority" },
    { 420,                  "420 Bad Extension" },
    { 421,                  "421 Extension Required" },
    { 422,                  "422 Session Interval Too Small" },
    { 423,                  "423 Interval Too Brief" },
    { 424,                  "424 Bad Location Information" },
    { 428,                  "428 Use Identity Header" },
    { 429,                  "429 Provide Referrer Identity" },
    { 430,                  "430 Flow Failed" },
    { 433,                  "433 Anonymity Disallowed" },
    { 436,                  "436 Bad Identity-Info" },
    { 437,                  "437 Unsupported Certificate" },
    { 438,                  "438 Invalid Identity Header" },
    { 439,                  "439 First Hop Lacks Outbound Support" },
    { 470,                  "470 Consent Needed" },
    { 480,                  "480 Temporarily Unavailable" },
    { 481,                  "481 Call/Transaction Does Not Exist" },
    { 482,                  "482 Loop Detected." },
    { 483,                  "483 Too Many Hops" },
    { 484,                  "484 Address Incomplete" },
    { 485,                  "485 Ambiguous" },
    { 486,                  "486 Busy Here" },
    { 487,                  "487 Request Terminated" },
    { 488,                  "488 Not Acceptable Here" },
    { 489,                  "489 Bad Event" },
    { 491,                  "491 Request Pending" },
    { 493,                  "493 Undecipherable" },
    { 494,                  "494 Security Agreement Required" },
    { 500,                  "500 Server Internal Error" },
    { 501,                  "501 Not Implemented" },
    { 502,                  "502 Bad Gateway" },
    { 503,                  "503 Service Unavailable" },
    { 504,                  "504 Server Time-out" },
    { 505,                  "505 Version Not Supported" },
    { 513,                  "513 Message Too Large" },
    { 580,                  "580 Precondition Failure" },
    { 600,                  "600 Busy Everywhere" },
    { 603,                  "603 Decline" },
    { 604,                  "604 Does Not Exist Anywhere" },
    { 606,                  "606 Not Acceptable" },
    { 0, NULL },
};

const gchar *
sip_method_str(guint method)
{
    int i;

    // Standard method
    for (i = 0; sip_codes[i].id > 0; i++) {
        if (method == sip_codes[i].id)
            return sip_codes[i].text;
    }
    return NULL;
}

guint
packet_sip_method_from_str(const gchar *method)
{
    // Standard method
    for (guint i = 0; sip_codes[i].id > 0; i++) {
        if (g_strcmp0(method, sip_codes[i].text) == 0)
            return sip_codes[i].id;
    }
    return (guint) g_ascii_strtoull(method, NULL, 10);
}

PacketSipData *
packet_sip_data(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);

    // Get Packet sip data
    PacketSipData *sip = g_ptr_array_index(packet->proto, PACKET_SIP);
    g_return_val_if_fail(sip != NULL, NULL);

    return sip;
}

const gchar *
packet_sip_payload(const Packet *packet)
{
    // Get Packet sip data
    PacketSipData *sip = packet_sip_data(packet);
    return sip->payload;
}

const gchar *
packet_sip_method_str(const Packet *packet)
{
    PacketSipData *sip = packet_sip_data(packet);

    // Check if code has non-standard text
    if (sip->code.text != NULL) {
        return sip->code.text;
    } else {
        return sip_method_str(sip->code.id);
    }
}

guint
packet_sip_method(const Packet *packet)
{
    return packet_sip_data(packet)->code.id;
}

guint64
packet_sip_cseq(const Packet *packet)
{
    return packet_sip_data(packet)->cseq;
}

gboolean
packet_sip_initial_transaction(const Packet *packet)
{
    return packet_sip_data(packet)->initial;
}

const gchar *
packet_sip_auth_data(const Packet *packet)
{
    return packet_sip_data(packet)->auth;
}

static GByteArray *
packet_sip_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    gchar *method = NULL;
    gchar *resp_code = NULL;

    // Ignore too small packets
    if (data->len < SIP_VERSION_LEN + 1)
        return data;

    // Convert payload to something we can parse with regular expressions
    g_autoptr(GString) payload = g_string_new_len((const gchar *) data->data, data->len);

    // Split SIP payload in lines separated by CRLF
    g_auto(GStrv) payload_data = g_strsplit(payload->str, SIP_CRLF, 0);

    if (g_strv_length(payload_data) == 0) {
        return data;
    }

    g_auto(GStrv) first_line_data = g_strsplit(payload_data[0], " ", 2);
    if (g_strv_length(first_line_data) != 2) {
        return data;
    }

    if (g_strcmp0(first_line_data[0], SIP_VERSION) == 0) {
        resp_code = g_strdup(first_line_data[1]);
    }

    if (resp_code == NULL) {
        for (guint i = 0; sip_codes[i].id < 100; i++) {
            if (g_strcmp0(first_line_data[0], sip_codes[i].text) == 0) {
                method = g_strdup(first_line_data[0]);
                break;
            }
        }
    }

    // No SIP information in first line. Skip this packet.
    if (method == NULL && resp_code == NULL) {
        return data;
    }

    // Allocate packet sip data
    PacketSipData *sip_data = g_malloc0(sizeof(PacketSipData));
    if (method != NULL) {
        sip_data->code.id = packet_sip_method_from_str(method);
        sip_data->code.text = method;
    } else {
        sip_data->code.id = packet_sip_method_from_str(resp_code);
        sip_data->code.text = resp_code;
    }
    sip_data->payload = g_strdup(payload->str);

    // Add SIP information to the packet
    packet_add_type(packet, PACKET_SIP, sip_data);

    guint sip_size = (guint) strlen(payload_data[0]) + 2 /* CRLF */;
    g_auto(GStrv) headers = g_strsplit(payload->str, SIP_CRLF, 0);
    for (guint i = 1; i < g_strv_length(headers); i++) {
        const gchar *line = headers[i];

        // End of SIP payload
        if (g_strcmp0(line, "") == 0) {
            sip_size += 2 /* Final CRLF */;
            break;
        }

        // Sip Headers Size
        sip_size += strlen(line) + 2 /* CRLF */;

        g_auto(GStrv) hdr_data = g_strsplit(line, ":", 2);
        if (g_strv_length(hdr_data) != 2) {
            break;
        }

        gchar *hdr_name = g_strstrip(hdr_data[0]);
        gchar *hdr_value = g_strstrip(hdr_data[1]);


        if (strcasecmp(hdr_name, "Call-ID") == 0 || strcasecmp(hdr_name, "i") == 0) {
            sip_data->callid = g_strdup(hdr_value);
        } else if (strcasecmp(hdr_name, "X-Call-ID") == 0 || strcasecmp(hdr_name, "X-CID") == 0) {
            g_free(sip_data->xcallid);      // In case X-Call-Id is multiple times in the payload
            sip_data->xcallid = g_strdup(hdr_value);
        } else if (strcasecmp(hdr_name, "To") == 0 || strcasecmp(hdr_name, "t") == 0) {
            sip_data->initial = g_strstr_len(hdr_value, strlen(hdr_value), ";tag=") == NULL;
        } else if (strcasecmp(hdr_name, "Content-Length") == 0 || strcasecmp(hdr_name, "l") == 0) {
            sip_data->content_len = g_ascii_strtoull(hdr_value, NULL, 10);
        } else if (strcasecmp(hdr_name, "CSeq") == 0) {
            g_auto(GStrv) cseq_data = g_strsplit(hdr_value, " ", 2);
            if (g_strv_length(cseq_data) != 2)
                break;
            sip_data->cseq = g_ascii_strtoull(cseq_data[0], NULL, 10);
        } else if (strcasecmp(hdr_name, "Authorization") == 0 || strcasecmp(hdr_name, "Proxy-Authorization") == 0) {
            sip_data->auth = g_strdup(hdr_value);
        }
    }

    // Check we have a valid SIP packet
    if (sip_data->callid == NULL) {
        return data;
    }

    // If this comes from a TCP stream, check we have a whole packet
    if (packet_has_type(packet, PACKET_TCP)) {
        if (sip_data->content_len != data->len - sip_size) {
            return data;
        }
    }

    // Handle bad terminated SIP messages
    if (sip_size > data->len)
        sip_size = data->len;

    // Remove SIP headers from data
    data = g_byte_array_remove_range(data, 0, sip_size);

    // Pass data to subdissectors
    packet_parser_next_dissector(parser, packet, data);

    // Add data to storage
    storage_add_packet(packet);

    return data;
}

static void
packet_sip_free(G_GNUC_UNUSED PacketParser *parser, Packet *packet)
{
    PacketSipData *sip_data = packet_sip_data(packet);
    g_return_if_fail(sip_data != NULL);

    g_free(sip_data->callid);
    g_free(sip_data->xcallid);
    g_free(sip_data->auth);
    g_free(sip_data->payload);
    g_free(sip_data->code.text);
    g_free(sip_data);
}

PacketDissector *
packet_sip_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_SIP;
    proto->dissect = packet_sip_parse;
    proto->free = packet_sip_free;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SDP));
    return proto;
}
