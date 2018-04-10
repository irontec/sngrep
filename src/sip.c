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
 * @file sip.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in sip.h
 */
#include "config.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include "glib-utils.h"
#include "sip.h"
#include "option.h"
#include "setting.h"
#include "filter.h"

/**
 * @brief Linked list of parsed calls
 *
 * All parsed calls will be added to this list, only accesible from
 * this awesome structure, so, keep it thread-safe.
 */
sip_call_list_t calls =
{ 0 };

/* @brief list of methods and responses */
sip_code_t sip_codes[] = {
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

void
sip_init(int limit, int only_calls, int no_incomplete)
{
    // Store capture limit
    calls.limit = limit;
    calls.only_calls = only_calls;
    calls.ignore_incomplete = no_incomplete;
    calls.last_index = 0;

    // Create a vector to store calls
    calls.list = g_sequence_new(call_destroy);
    calls.active = g_sequence_new(NULL);

    // Create hash table for callid search
    calls.callids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    // Set default sorting field
    if (sip_attr_from_name(setting_get_value(SETTING_CL_SORTFIELD)) >= 0) {
        calls.sort.by = sip_attr_from_name(setting_get_value(SETTING_CL_SORTFIELD));
        calls.sort.asc = (!strcmp(setting_get_value(SETTING_CL_SORTORDER), "asc"));
    } else {
        // Fallback to default sorting field
        calls.sort.by = SIP_ATTR_CALLINDEX;
        calls.sort.asc = true;
    }

    // Initialize payload parsing regexp
    GRegexMatchFlags mflags = G_REGEX_MATCH_NEWLINE_CRLF;
    GRegexCompileFlags cflags = G_REGEX_OPTIMIZE | G_REGEX_CASELESS | G_REGEX_NEWLINE_CRLF | G_REGEX_MULTILINE;

    calls.reg_method = g_regex_new("(?P<method>\\w+) [^:]+:\\S* SIP/2.0", cflags & ~G_REGEX_MULTILINE, mflags, NULL);
    calls.reg_callid = g_regex_new("^(Call-ID|i):\\s*(?P<callid>.+)$", cflags, mflags, NULL);
    calls.reg_xcallid = g_regex_new("^(X-Call-ID|X-CID):\\s*(?P<xcallid>.+)$", cflags, mflags, NULL);
    calls.reg_response = g_regex_new("SIP/2.0 (?P<text>(?P<code>\\d{3}) .*)", cflags & ~G_REGEX_MULTILINE, mflags, NULL);
    calls.reg_cseq = g_regex_new("^CSeq:\\s*(?P<cseq>\\d+)\\s+\\w+$", cflags, mflags, NULL);
    calls.reg_from = g_regex_new("^(From|f):[^:]+:(?P<from>((?P<fromuser>[^@;>\r]+)@)?[^;>\r]+)", cflags, mflags, NULL);
    calls.reg_to = g_regex_new("^(To|t):[^:]+:(?P<to>((?P<touser>[^@;>\r]+)@)?[^;>\r]+)", cflags, mflags, NULL);
    calls.reg_valid = g_regex_new("^(\\w+ \\w+:|SIP/2.0 \\d{3})", cflags & ~G_REGEX_MULTILINE, mflags, NULL);
    calls.reg_cl = g_regex_new( "^(Content-Length|l):\\s*(?P<clen>\\d+)$", cflags, mflags, NULL);
    calls.reg_body = g_regex_new("\r\n\r\n(.*)", cflags & ~G_REGEX_MULTILINE, mflags, NULL);
    calls.reg_reason = g_regex_new("Reason:[ ]*[^\r]*;text=\"([^\r]+)\"", cflags, mflags, NULL);
    calls.reg_warning = g_regex_new("^Warning:\\s*(?P<warning>\\d+)", cflags, mflags, NULL);
}

void
sip_deinit()
{
    // Remove all calls
    sip_calls_clear();
    // Remove Call-id hash table
    g_hash_table_destroy(calls.callids);
    // Remove calls vector
    g_sequence_free(calls.list);
    g_sequence_free(calls.active);
    // Deallocate regular expressions
    g_regex_unref(calls.reg_method);
    g_regex_unref(calls.reg_callid);
    g_regex_unref(calls.reg_xcallid);
    g_regex_unref(calls.reg_response);
    g_regex_unref(calls.reg_cseq);
    g_regex_unref(calls.reg_from);
    g_regex_unref(calls.reg_to);
    g_regex_unref(calls.reg_valid);
    g_regex_unref(calls.reg_cl);
    g_regex_unref(calls.reg_body);
    g_regex_unref(calls.reg_reason);
    g_regex_unref(calls.reg_warning);
}


char *
sip_get_callid(const char* payload, char *callid)
{
    GMatchInfo *pmatch;

    // Try to get Call-ID from payload
    g_regex_match(calls.reg_callid, payload, 0, &pmatch);
    if (g_match_info_matches(pmatch)) {
        // Copy the matching part of payload
        strcpy(callid, g_match_info_fetch_named(pmatch, "callid"));
    }
    g_match_info_free(pmatch);

    return callid;
}

char *
sip_get_xcallid(const char *payload, char *xcallid)
{
    GMatchInfo *pmatch;

    g_regex_match(calls.reg_xcallid, payload, 0, &pmatch);
    if (g_match_info_matches(pmatch)) {
        strcpy(xcallid, g_match_info_fetch_named(pmatch, "xcallid"));
    }
    g_match_info_free(pmatch);

    return xcallid;
}

int
sip_validate_packet(packet_t *packet)
{
    uint32_t plen = packet_payloadlen(packet);
    u_char payload[MAX_SIP_PAYLOAD];
    GMatchInfo *pmatch;
    char cl_header[10];
    int content_len;
    int bodylen;

    // Max SIP payload allowed
    if (plen == 0 || plen > MAX_SIP_PAYLOAD)
        return VALIDATE_NOT_SIP;

    // Get payload from packet(s)
    memset(payload, 0, MAX_SIP_PAYLOAD);
    memcpy(payload, packet_payload(packet), plen);

    // Initialize variables
    memset(cl_header, 0, sizeof(cl_header));

    // Check if payload is UTF8
    // @TODO what about binary content-body
    if (!g_utf8_validate((const char *) payload, plen, NULL)) {
        return VALIDATE_NOT_SIP;
    }

    // Check if the first line follows SIP request or response format
    if (g_regex_match(calls.reg_valid, (const char *) payload, 0, NULL)) {
        // Not a SIP message AT ALL
        return VALIDATE_NOT_SIP;
    }

    // Check if we have Content Length header
    g_regex_match(calls.reg_cl, (const char *) payload, 0, &pmatch);
    if (!g_match_info_matches(pmatch)) {
        g_match_info_free(pmatch);
        // Not a SIP message or not complete
        return VALIDATE_PARTIAL_SIP;
    }

    strncpy(cl_header, g_match_info_fetch_named(pmatch, "clen"), sizeof(cl_header));
    content_len = atoi(cl_header);
    g_match_info_free(pmatch);

    // Check if we have Body separator field
    g_regex_match(calls.reg_body, (const char *) payload, 0, &pmatch);
    if (!g_match_info_matches(pmatch)) {
        g_match_info_free(pmatch);
        // Not a SIP message or not complete
        return VALIDATE_PARTIAL_SIP;
    }

    // Get the SIP message body length
    bodylen = strlen(g_match_info_fetch(pmatch, 1));

    // The SDP body of the SIP message ends in another packet
    if (content_len > bodylen) {
        g_match_info_free(pmatch);
        return VALIDATE_PARTIAL_SIP;
    }

    if (content_len < bodylen) {
        // We got more than one SIP message in the same packet
        gint start, end;
        g_match_info_fetch_pos(pmatch, 1, &start, &end);
        packet_set_payload(packet, payload, start + content_len);
        g_match_info_free(pmatch);
        return VALIDATE_MULTIPLE_SIP;
    }

    g_match_info_free(pmatch);

    // We got all the SDP body of the SIP message
    return VALIDATE_COMPLETE_SIP;
}

sip_msg_t *
sip_check_packet(packet_t *packet)
{
    sip_msg_t *msg;
    sip_call_t *call;
    char callid[1024], xcallid[1024];
    u_char payload[MAX_SIP_PAYLOAD];
    bool newcall = false;

    // Max SIP payload allowed
    if (packet->payload_len > MAX_SIP_PAYLOAD)
        return NULL;

    // Initialize local variables
    memset(callid, 0, sizeof(callid));
    memset(xcallid, 0, sizeof(xcallid));

    // Get payload from packet(s)
    memset(payload, 0, MAX_SIP_PAYLOAD);
    memcpy(payload, packet_payload(packet), packet_payloadlen(packet));

    // Check if payload is UTF8
    // @TODO what about binary content-body
    if (!g_utf8_validate((const char *)payload, -1, NULL))
        return NULL;

    // Get the Call-ID of this message
    if (!sip_get_callid((const char*) payload, callid))
        return NULL;

    // Create a new message from this data
    if (!(msg = msg_create((const char*) payload)))
        return NULL;

    // Get Method and request for the following checks
    // There is no need to parse all payload at this point
    // If no response or request code is found, this is not a SIP message
    if (!sip_get_msg_reqresp(msg, payload)) {
        // Deallocate message memory
        msg_destroy(msg);
        return NULL;
    }

    // Find the call for this msg
    if (!(call = sip_find_by_callid(callid))) {

        // Check if payload matches expression
        if (!sip_check_match_expression((const char*) payload))
            goto skip_message;

        // User requested only INVITE starting dialogs
        if (calls.only_calls && msg->reqresp != SIP_METHOD_INVITE)
            goto skip_message;

        // Only create a new call if the first msg
        // is a request message in the following gorup
        if (calls.ignore_incomplete && msg->reqresp > SIP_METHOD_MESSAGE)
            goto skip_message;

        // Get the Call-ID of this message
        sip_get_xcallid((const char*) payload, xcallid);

        // Rotate call list if limit has been reached
        if (calls.limit == sip_calls_count())
            sip_calls_rotate();

        // Create the call if not found
        if (!(call = call_create(callid, xcallid)))
            goto skip_message;

        // Add this Call-Id to hash table
        g_hash_table_insert(calls.callids, call->callid, call);

        // Set call index
        call->index = ++calls.last_index;

        // Mark this as a new call
        newcall = true;
    }

    // At this point we know we're handling an interesting SIP Packet
    msg->packet = packet;

    // Always parse first call message
    if (call_msg_count(call) == 0) {
        // Parse SIP payload
        sip_parse_msg_payload(msg, payload);
        // If this call has X-Call-Id, append it to the parent call
        if (strlen(call->xcallid)) {
            call_add_xcall(sip_find_by_callid(call->xcallid), call);
        }
    }

    // Add the message to the call
    call_add_message(call, msg);

    // check if message is a retransmission
    call_msg_retrans_check(msg);

    if (call_is_invite(call)) {
        // Parse media data
        sip_parse_msg_media(msg, payload);
        // Update Call State
        call_update_state(call, msg);
        // Parse extra fields
        sip_parse_extra_headers(msg, payload);
        // Check if this call should be in active call list
        if (call_is_active(call)) {
            if (sip_call_is_active(call)) {
                g_sequence_append(calls.active, call);
            }
        } else {
            if (sip_call_is_active(call)) {
                g_sequence_remove_data(calls.active, call);
            }
        }
    }

    if (newcall) {
        // Append this call to the call list
        g_sequence_insert_sorted(calls.list, call, sip_list_sorter, NULL);
    }

    // Mark the list as changed
    calls.changed = true;

    // Return the loaded message
    return msg;

skip_message:
    // Deallocate message memory
    msg_destroy(msg);
    return NULL;

}

bool
sip_calls_has_changed()
{
    bool changed = calls.changed;
    calls.changed = false;
    return changed;
}

int
sip_calls_count()
{
    return g_sequence_get_length(calls.list);
}

GSequenceIter *
sip_calls_iterator()
{
    return g_sequence_get_begin_iter(calls.list);
}

GSequenceIter *
sip_active_calls_iterator()
{
    return g_sequence_get_begin_iter(calls.active);
}

bool
sip_call_is_active(sip_call_t *call)
{
    return g_sequence_index(calls.active, call) != -1;
}

GSequence *
sip_calls_vector()
{
    return calls.list;
}

GSequence *
sip_active_calls_vector()
{
    return calls.active;
}

sip_stats_t
sip_calls_stats()
{
    sip_stats_t stats = {};
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);

    // Total number of calls without filtering
    stats.total = g_sequence_iter_length(it);
    // Total number of calls after filtering
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        if (filter_check_call(g_sequence_get(it), NULL))
            stats.displayed++;
    }
    return stats;
}

sip_call_t *
sip_find_by_index(int index)
{
    return g_sequence_nth(calls.list, index);
}

sip_call_t *
sip_find_by_callid(const char *callid)
{
    return g_hash_table_lookup(calls.callids, callid);
}

int
sip_get_msg_reqresp(sip_msg_t *msg, const u_char *payload)
{
    GMatchInfo *pmatch;
    char resp_str[SIP_ATTR_MAXLEN];
    char reqresp[SIP_ATTR_MAXLEN];
    char cseq[11];
    const char *resp_def;

    // Initialize variables
    memset(resp_str, 0, sizeof(resp_str));
    memset(reqresp, 0, sizeof(reqresp));

    // If not already parsed
    if (!msg->reqresp) {
        // Method & CSeq
        if (g_regex_match(calls.reg_method, (const char *) payload, 0, &pmatch)) {
            strncpy(reqresp, g_match_info_fetch_named(pmatch, "method"), sizeof(reqresp));
        }
        g_match_info_free(pmatch);

        // CSeq
        if (g_regex_match(calls.reg_cseq, (const char *) payload, 0, &pmatch)) {
            strncpy(cseq, g_match_info_fetch_named(pmatch, "cseq"), sizeof(cseq));
            msg->cseq = atoi(cseq);
        }
        g_match_info_free(pmatch);

        // Response code
        if (g_regex_match(calls.reg_response, (const char *) payload, 0, &pmatch)) {
            strncpy(resp_str, g_match_info_fetch_named(pmatch, "text"), sizeof(resp_str));
            strncpy(reqresp, g_match_info_fetch_named(pmatch, "code"), sizeof(reqresp));
        }
        g_match_info_free(pmatch);

        // Get Request/Response Code
        msg->reqresp = sip_method_from_str(reqresp);

        // For response codes, check if the text matches the default
        if (!msg_is_request(msg)) {
            resp_def = sip_method_str(msg->reqresp);
            if (!resp_def || strcmp(resp_def, resp_str)) {
                msg->resp_str = strdup(resp_str);
            }
        }
    }

    return msg->reqresp;
}

const char *
sip_get_msg_reqresp_str(sip_msg_t *msg)
{
    // Check if code has non-standard text
    if (msg->resp_str) {
        return msg->resp_str;
    } else {
        return sip_method_str(msg->reqresp);
    }
}

sip_msg_t *
sip_parse_msg(sip_msg_t *msg)
{
    if (msg && !msg->cseq) {
        sip_parse_msg_payload(msg, (u_char*) msg_get_payload(msg));
    }
    return msg;
}

int
sip_parse_msg_payload(sip_msg_t *msg, const u_char *payload)
{
    GMatchInfo *pmatch;

    // From
    if (g_regex_match(calls.reg_from, (const char *) payload, 0, &pmatch)) {
        msg->sip_from = g_match_info_fetch_named(pmatch, "from");
    } else {
        msg->sip_from = strdup("<malformed>");
    }
    g_match_info_free(pmatch);


    // To
    if (g_regex_match(calls.reg_to, (const char *) payload, 0, &pmatch)) {
        msg->sip_to = g_match_info_fetch_named(pmatch, "to");
    } else {
        msg->sip_to = strdup("<malformed>");
    }
    g_match_info_free(pmatch);

    return 0;
}

void
sip_parse_msg_media(sip_msg_t *msg, const u_char *payload)
{

#define ADD_STREAM(stream) \
    if (stream) { \
        if (!rtp_find_call_stream(call, src, stream->dst)) { \
          call_add_stream(call, stream); \
      } else { \
          sng_free(stream); \
          stream = NULL; \
      } \
    }

    address_t dst, src = { };
    rtp_stream_t *rtp_stream = NULL, *rtcp_stream = NULL, *msg_rtp_stream = NULL;
    char media_type[MEDIATYPELEN] = { };
    char media_format[30] = { };
    uint32_t media_fmt_pref;
    uint32_t media_fmt_code;
    sdp_media_t *media = NULL;
    char *payload2, *tofree, *line;
    sip_call_t *call = msg_get_call(msg);

    // Parse each line of payload looking for sdp information
    tofree = payload2 = strdup((char*)payload);
    while ((line = strsep(&payload2, "\r\n")) != NULL) {
        // Check if we have a media string
        if (!strncmp(line, "m=", 2)) {
            if (sscanf(line, "m=%s %hu RTP/%*s %u", media_type, &dst.port, &media_fmt_pref) == 3) {

                // Add streams from previous 'm=' line to the call
                ADD_STREAM(msg_rtp_stream);
                ADD_STREAM(rtp_stream);
                ADD_STREAM(rtcp_stream);

                // Create a new media structure for this message
                if ((media = media_create(msg))) {
                    media_set_type(media, media_type);
                    media_set_address(media, dst);
                    media_set_prefered_format(media, media_fmt_pref);
                    msg_add_media(msg, media);

                    /**
                     * From SDP we can only guess destination address port. RTP Capture proccess
                     * will determine when the stream has been completed, getting source address
                     * and port of the stream.
                     */
                    // Create a new stream with this destination address:port

                    // Create RTP stream with source of message as destination address
                    msg_rtp_stream = stream_create(media, dst, PACKET_RTP);
                    msg_rtp_stream->dst = msg->packet->src;
                    msg_rtp_stream->dst.port = dst.port;

                    // Create RTP stream
                    rtp_stream = stream_create(media, dst, PACKET_RTP);

                    // Create RTCP stream
                    rtcp_stream = stream_create(media, dst, PACKET_RTCP);
                    rtcp_stream->dst.port++;
                }
            }
        }

        // Check if we have a connection string
        if (!strncmp(line, "c=", 2)) {
            if (sscanf(line, "c=IN IP4 %s", dst.ip) && media) {
                media_set_address(media, dst);
                strcpy(rtp_stream->dst.ip, dst.ip);
                strcpy(rtcp_stream->dst.ip, dst.ip);
            }
        }

        // Check if we have attribute format string
        if (!strncmp(line, "a=rtpmap:", 9)) {
            if (media && sscanf(line, "a=rtpmap:%u %[^ ]", &media_fmt_code, media_format)) {
                media_add_format(media, media_fmt_code, media_format);
            }
        }

        // Check if we have attribute format RTCP port
        if (!strncmp(line, "a=rtcp:", 7) && rtcp_stream) {
            sscanf(line, "a=rtcp:%hu", &rtcp_stream->dst.port);
        }


    }

    // Add streams from last 'm=' line to the call
    ADD_STREAM(msg_rtp_stream);
    ADD_STREAM(rtp_stream);
    ADD_STREAM(rtcp_stream);

    sng_free(tofree);

#undef ADD_STREAM
}

void
sip_parse_extra_headers(sip_msg_t *msg, const u_char *payload)
{
    GMatchInfo *pmatch;

    // Reason text
    if (g_regex_match(calls.reg_reason, (const char *) payload, 0, &pmatch)) {
        msg->call->reasontxt = strdup(g_match_info_fetch(pmatch, 1));
    }
    g_match_info_free(pmatch);

    // Warning code
    if (g_regex_match(calls.reg_warning, (const char *) payload, 0, &pmatch)) {
        msg->call->warning = atoi(g_match_info_fetch_named(pmatch, "warning"));
    }
    g_match_info_free(pmatch);

}

void
sip_calls_clear()
{
    // Create again the callid hash table
    g_hash_table_remove_all(calls.callids);

    // Remove all items from vector
    g_sequence_remove_all(calls.list);
    g_sequence_remove_all(calls.active);
}

void
sip_calls_clear_soft()
{
    // Create again the callid hash table
    g_hash_table_remove_all(calls.callids);

    // Repopulate list applying current filter
    calls.list = g_sequence_copy(sip_calls_vector(), filter_check_call, NULL);
    calls.active = g_sequence_copy(sip_active_calls_vector(), filter_check_call, NULL);

    // Repopulate callids based on filtered list
    sip_call_t *call;
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);

    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        call = g_sequence_get(it);
        g_hash_table_insert(calls.callids, call->callid, call);
    }
}

void
sip_calls_rotate()
{
    sip_call_t *call;
    GSequenceIter *it = g_sequence_get_begin_iter(calls.list);
    for (;!g_sequence_iter_is_end(it); it = g_sequence_iter_next(it)) {
        call = g_sequence_get(it);
        if (!call->locked) {
            // Remove from callids hash
            g_hash_table_remove(calls.callids, call->callid);
            // Remove first call from active and call lists
            g_sequence_remove_data(calls.active, call);
            g_sequence_remove_data(calls.list, call);
            return;
        }
    }
}

int
sip_set_match_expression(const char *expr, int insensitive, int invert)
{
    // Store expression text
    calls.match_expr = expr;
    // Set invert flag
    calls.match_invert = invert;

    GRegexCompileFlags cflags = 0;

    // Case insensitive requested
    if (insensitive)
        cflags |= G_REGEX_CASELESS;

    // Check the expresion is a compilable regexp
    calls.match_regex = g_regex_new(expr, cflags, 0, NULL);
    return (calls.match_regex != NULL) ? 0 : 1;
}

const char *
sip_get_match_expression()
{
    return calls.match_expr;
}

int
sip_check_match_expression(const char *payload)
{
    // Everything matches when there is no match
    if (!calls.match_expr)
        return 1;

    // Check if payload matches the given expresion
    if (g_regex_match(calls.match_regex, payload, 0, NULL)) {
        return 0 == calls.match_invert;
    } else {
        return 1 == calls.match_invert;
    }

}

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
        if (!strcmp(method, sip_codes[i].text))
            return sip_codes[i].id;
    }
    return atoi(method);
}

const char *
sip_transport_str(int transport)
{
    switch(transport)
    {
        case PACKET_SIP_UDP:
            return "UDP";
        case PACKET_SIP_TCP:
            return "TCP";
        case PACKET_SIP_TLS:
            return "TLS";
        case PACKET_SIP_WS:
            return "WS";
        case PACKET_SIP_WSS:
            return "WSS";
    }
    return "";
}

char *
sip_get_msg_header(sip_msg_t *msg, char *out)
{
    char from_addr[80], to_addr[80], time[80], date[80];

    // Source and Destination address
    msg_get_attribute(msg, SIP_ATTR_DATE, date);
    msg_get_attribute(msg, SIP_ATTR_TIME, time);
    msg_get_attribute(msg, SIP_ATTR_SRC, from_addr);
    msg_get_attribute(msg, SIP_ATTR_DST, to_addr);

    // Get msg header
    sprintf(out, "%s %s %s -> %s", date, time, from_addr, to_addr);
    return out;
}

void
sip_set_sort_options(sip_sort_t sort)
{
    calls.sort = sort;
    sip_sort_list();
}

sip_sort_t
sip_sort_options()
{
    return calls.sort;
}

void
sip_sort_list()
{
    g_sequence_sort(calls.list, sip_list_sorter, NULL);
}

gint
sip_list_sorter(gconstpointer a, gconstpointer b, gpointer user_data)
{
    const sip_call_t *calla = a, *callb = b;
    int cmp = call_attr_compare(calla, callb, calls.sort.by);
    return (calls.sort.asc) ? cmp : cmp * -1;
}
