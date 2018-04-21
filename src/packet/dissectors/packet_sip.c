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
 * @file packet_sip.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_sip.h
 */

#include "config.h"
#include <stdlib.h>
#include <packet/old_packet.h>
#include "packet_tcp.h"
#include "packet_sdp.h"
#include "storage.h"
#include "packet_sip.h"
#include "packet/old_packet.h"

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
    { 100, "100 Trying" },
    { 180, "180 Ringing" },
    { 181, "181 Call is Being Forwarded" },
    { 182, "182 Queued" },
    { 183, "183 Session Progress" },
    { 199, "199 Early Dialog Terminated" },
    { 200, "200 OK" },
    { 202, "202 Accepted" },
    { 204, "204 No Notification" },
    { 300, "300 Multiple Choices" },
    { 301, "301 Moved Permanently" },
    { 302, "302 Moved Temporarily" },
    { 305, "305 Use Proxy" },
    { 380, "380 Alternative Service" },
    { 400, "400 Bad Request" },
    { 401, "401 Unauthorized" },
    { 402, "402 Payment Required" },
    { 403, "403 Forbidden" },
    { 404, "404 Not Found" },
    { 405, "405 Method Not Allowed" },
    { 406, "406 Not Acceptable" },
    { 407, "407 Proxy Authentication Required" },
    { 408, "408 Request Timeout" },
    { 409, "409 Conflict" },
    { 410, "410 Gone" },
    { 411, "411 Length Required" },
    { 412, "412 Conditional Request Failed" },
    { 413, "413 Request Entity Too Large" },
    { 414, "414 Request-URI Too Long" },
    { 415, "415 Unsupported Media Type" },
    { 416, "416 Unsupported URI Scheme" },
    { 417, "417 Unknown Resource-Priority" },
    { 420, "420 Bad Extension" },
    { 421, "421 Extension Required" },
    { 422, "422 Session Interval Too Small" },
    { 423, "423 Interval Too Brief" },
    { 424, "424 Bad Location Information" },
    { 428, "428 Use Identity Header" },
    { 429, "429 Provide Referrer Identity" },
    { 430, "430 Flow Failed" },
    { 433, "433 Anonymity Disallowed" },
    { 436, "436 Bad Identity-Info" },
    { 437, "437 Unsupported Certificate" },
    { 438, "438 Invalid Identity Header" },
    { 439, "439 First Hop Lacks Outbound Support" },
    { 470, "470 Consent Needed" },
    { 480, "480 Temporarily Unavailable" },
    { 481, "481 Call/Transaction Does Not Exist" },
    { 482, "482 Loop Detected." },
    { 483, "483 Too Many Hops" },
    { 484, "484 Address Incomplete" },
    { 485, "485 Ambiguous" },
    { 486, "486 Busy Here" },
    { 487, "487 Request Terminated" },
    { 488, "488 Not Acceptable Here" },
    { 489, "489 Bad Event" },
    { 491, "491 Request Pending" },
    { 493, "493 Undecipherable" },
    { 494, "494 Security Agreement Required" },
    { 500, "500 Server Internal Error" },
    { 501, "501 Not Implemented" },
    { 502, "502 Bad Gateway" },
    { 503, "503 Service Unavailable" },
    { 504, "504 Server Time-out" },
    { 505, "505 Version Not Supported" },
    { 513, "513 Message Too Large" },
    { 580, "580 Precondition Failure" },
    { 600, "600 Busy Everywhere" },
    { 603, "603 Decline" },
    { 604, "604 Does Not Exist Anywhere" },
    { 606, "606 Not Acceptable" },
    { -1 , NULL },
};

const char *
sip_method_str(int method)
{
    int i;

    // Standard method
    for (i = 0; sip_codes[i].id > 0; i++) {
        if (method == sip_codes[i].id)
            return sip_codes[i].text;
    }
    return NULL;
}

int
sip_method_from_str(const char *method)
{
    int i;

    // Standard method
    for (i = 0; sip_codes[i].id > 0; i++) {
        if (!g_strcmp0(method, sip_codes[i].text))
            return sip_codes[i].id;
    }
    return atoi(method);
}

const gchar *
packet_sip_payload(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);

    // Get Packet sip data
    PacketSipData *sip = g_ptr_array_index(packet->proto, PACKET_SIP);
    g_return_val_if_fail(sip != NULL, NULL);
    g_return_val_if_fail(sip->payload != NULL, NULL);

    return sip->payload;
}


static GByteArray *
packet_sip_parse(PacketParser *parser, Packet *packet, GByteArray *data)
{
    GMatchInfo *pmatch;
    gint start, end;

    // Only handle UTF-8 SIP payloads
    if (!g_utf8_validate(data->data, data->len, NULL)) {
        return data;
    }

    DissectorSipData *sip = g_ptr_array_index(parser->dissectors, PACKET_SIP);

    // Convert payload to something we can parse with regular expressions
    GString *payload = g_string_new_len(data->data, data->len);

    // If this comes from a TCP stream, check we have a whole packet
    if (packet_has_type(packet, PACKET_TCP)) {
        // Content-Lenght is mandatory for SIP TCP
        g_regex_match(sip->reg_cl, payload->str, 0, &pmatch);
        if (!g_match_info_matches(pmatch)) {
            g_match_info_free(pmatch);
            // Not a SIP message or not complete
            return data;
        }
        gchar *cl_header = g_match_info_fetch_named(pmatch, "clen");
        gint content_len = atoi(cl_header);
        g_match_info_free(pmatch);

        // Check if we have Body separator field
        g_regex_match(sip->reg_body, payload->str, 0, &pmatch);
        if (!g_match_info_matches(pmatch)) {
            g_match_info_free(pmatch);
            // Not a SIP message or not complete
            return data;
        }
        g_match_info_fetch_pos(pmatch, 1, &start, &end);

        // The SDP body of the SIP message ends in another packet
        if (start + content_len > data->len) {
            g_match_info_free(pmatch);
            // Not a SIP message or not complete
            return data;
        }

        // We got more than one SIP message in the same packet
        if (start + content_len < data->len) {
            // Limit the size of the string to the end of the body
            g_string_set_size(payload, start + content_len);
        }

        g_match_info_free(pmatch);
    }

    // Allocate packet sip data
    PacketSipData *sip_data = g_malloc0(sizeof(PacketSipData));
    sip_data->payload = g_strdup(payload->str);

    // Try to get Call-ID from payload
    g_regex_match(sip->reg_callid, payload->str, 0, &pmatch);
    if (g_match_info_matches(pmatch)) {
        // Copy the matching part of payload
        sip_data->callid =  g_match_info_fetch_named(pmatch, "callid");
    }
    g_match_info_free(pmatch);

    // Check we have a valid SIP packet
    if (sip_data->callid == NULL)
        return data;

    // Method
    if (g_regex_match(sip->reg_method, payload->str, 0, &pmatch)) {
        sip_data->reqresp = sip_method_from_str(g_match_info_fetch_named(pmatch, "method"));
    }
    g_match_info_free(pmatch);

    g_regex_match(sip->reg_xcallid, payload->str, 0, &pmatch);
    if (g_match_info_matches(pmatch)) {
        sip_data->xcallid =  g_match_info_fetch_named(pmatch, "xcallid");
    }
    g_match_info_free(pmatch);

    // From
    if (g_regex_match(sip->reg_from, payload->str, 0, &pmatch)) {
        sip_data->from = g_match_info_fetch_named(pmatch, "from");
    } else {
        sip_data->from = strdup("<malformed>");
    }
    g_match_info_free(pmatch);

    // To
    if (g_regex_match(sip->reg_to, payload->str, 0, &pmatch)) {
        sip_data->to = g_match_info_fetch_named(pmatch, "to");
    } else {
        sip_data->to = strdup("<malformed>");
    }
    g_match_info_free(pmatch);

    // Reason text
    if (g_regex_match(sip->reg_reason, payload->str, 0, &pmatch)) {
        sip_data->reasontxt = strdup(g_match_info_fetch(pmatch, 1));
    }
    g_match_info_free(pmatch);

    // Warning code
    if (g_regex_match(sip->reg_warning, payload->str, 0, &pmatch)) {
        sip_data->warning = atoi(g_match_info_fetch_named(pmatch, "warning"));
    }
    g_match_info_free(pmatch);

    // CSeq
    if (g_regex_match(sip->reg_cseq, payload->str, 0, &pmatch)) {
        sip_data->cseq = atoi(g_match_info_fetch_named(pmatch, "cseq"));
    }
    g_match_info_free(pmatch);

    // Response code
    if (g_regex_match(sip->reg_response, payload->str, 0, &pmatch)) {
        sip_data->resp_str = g_strdup(g_match_info_fetch_named(pmatch, "text"));
        sip_data->reqresp = sip_method_from_str(g_match_info_fetch_named(pmatch, "code"));
    }
    g_match_info_free(pmatch);

    // Add SIP information to the packet
    g_ptr_array_insert(packet->proto, PACKET_SIP, sip_data);

    // Check if we have Body separator field
    g_regex_match(sip->reg_body, payload->str, 0, &pmatch);
    g_match_info_fetch_pos(pmatch, 0, &start, &end);

    // Remove SIP headers from data
    data = g_byte_array_remove_range(data, 0, end);

    // Pass data to subdissectors
    packet_parser_next_dissector(parser, packet, data);

    // Add data to storage
    storage_check_sip_packet(packet);

    return NULL;
}

static void
packet_sip_init(PacketParser *parser)
{

    DissectorSipData *sip = g_malloc0(sizeof(DissectorSipData));

    // Initialize payload parsing regexp
    GRegexMatchFlags mflags = G_REGEX_MATCH_NEWLINE_CRLF;
    GRegexCompileFlags cflags = G_REGEX_OPTIMIZE | G_REGEX_CASELESS | G_REGEX_NEWLINE_CRLF | G_REGEX_MULTILINE;

    sip->reg_method = g_regex_new(
            "(?P<method>\\w+) [^:]+:\\S* SIP/2.0",
            cflags & ~G_REGEX_MULTILINE, mflags, NULL);

    sip->reg_callid = g_regex_new(
            "^(Call-ID|i):\\s*(?P<callid>.+)$",
            cflags, mflags, NULL);

    sip->reg_xcallid = g_regex_new(
            "^(X-Call-ID|X-CID):\\s*(?P<xcallid>.+)$",
            cflags, mflags, NULL);

    sip->reg_response = g_regex_new(
            "SIP/2.0 (?P<text>(?P<code>\\d{3}) .*)",
            cflags & ~G_REGEX_MULTILINE, mflags, NULL);

    sip->reg_cseq = g_regex_new(
            "^CSeq:\\s*(?P<cseq>\\d+)\\s+\\w+$",
            cflags, mflags, NULL);

    sip->reg_from = g_regex_new(
            "^(From|f):[^:]+:(?P<from>((?P<fromuser>[^@;>\r]+)@)?[^;>\r]+)",
            cflags, mflags, NULL);

    sip->reg_to = g_regex_new(
            "^(To|t):[^:]+:(?P<to>((?P<touser>[^@;>\r]+)@)?[^;>\r]+)",
            cflags, mflags, NULL);

    sip->reg_valid = g_regex_new(
            "^(\\w+ \\w+:|SIP/2.0 \\d{3})",
            cflags & ~G_REGEX_MULTILINE, mflags, NULL);

    sip->reg_cl = g_regex_new(
            "^(Content-Length|l):\\s*(?P<clen>\\d+)$",
            cflags, mflags, NULL);

    sip->reg_body = g_regex_new(
            "\r\n\r\n",
            cflags & ~G_REGEX_MULTILINE, mflags, NULL);

    sip->reg_reason = g_regex_new(
            "Reason:[ ]*[^\r]*;text=\"([^\r]+)\"",
            cflags, mflags, NULL);

    sip->reg_warning = g_regex_new(
            "^Warning:\\s*(?P<warning>\\d+)",
            cflags, mflags, NULL);

    g_ptr_array_insert(parser->dissectors, PACKET_SIP, sip);

}

static void
packet_sip_deinit(PacketParser *parser)
{
    DissectorSipData *sip = g_ptr_array_index(parser->dissectors, PACKET_SIP);
    g_return_if_fail(sip != NULL);

    // Deallocate regular expressions
    g_regex_unref(sip->reg_method);
    g_regex_unref(sip->reg_callid);
    g_regex_unref(sip->reg_xcallid);
    g_regex_unref(sip->reg_response);
    g_regex_unref(sip->reg_cseq);
    g_regex_unref(sip->reg_from);
    g_regex_unref(sip->reg_to);
    g_regex_unref(sip->reg_valid);
    g_regex_unref(sip->reg_cl);
    g_regex_unref(sip->reg_body);
    g_regex_unref(sip->reg_reason);
    g_regex_unref(sip->reg_warning);

    // Free dissector data
    g_free(sip);
}

PacketDissector *
packet_sip_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_SIP;
    proto->init = packet_sip_init;
    proto->dissect = packet_sip_parse;
    proto->deinit = packet_sip_deinit;
    proto->subdissectors = g_slist_append(proto->subdissectors, GUINT_TO_POINTER(PACKET_SDP));
    return proto;
}