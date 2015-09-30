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
 * @file sip.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in sip.h
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <search.h>
#include <stdarg.h>
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
    int match_flags;

    // Store capture limit
    calls.limit = limit;
    calls.only_calls = only_calls;
    calls.ignore_incomplete = no_incomplete;

    // Create a vector to store calls
    calls.list = vector_create(200, 50);
    vector_set_destroyer(calls.list, call_destroyer);
    calls.active = vector_create(10, 10);

    // Create hash table for callid search
    hcreate(calls.limit);

    // Initialize payload parsing regexp
    match_flags = REG_EXTENDED | REG_ICASE | REG_NEWLINE;
    regcomp(&calls.reg_method, "^([a-zA-Z]+) sip:[^ ]+ SIP/2.0\r", match_flags & ~REG_NEWLINE);
    regcomp(&calls.reg_callid, "^(Call-ID|i):[ ]*([^ ]+)\r$", match_flags);
    regcomp(&calls.reg_xcallid, "^(X-Call-ID|X-CID):[ ]*([^ ]+)\r$", match_flags);
    regcomp(&calls.reg_response, "^SIP/2.0[ ]*(([0-9]{3}) [^\r]+)\r", match_flags & ~REG_NEWLINE);
    regcomp(&calls.reg_cseq, "^CSeq:[ ]*([0-9]+) .+\r$", match_flags);
    regcomp(&calls.reg_from, "^(From|f):[ ]*[^:]*:(([^@]+)@?[^\r>;]+)", match_flags);
    regcomp(&calls.reg_to, "^(To|t):[ ]*[^:]*:(([^@]+)@?[^\r>;]+)", match_flags);
}

void
sip_deinit()
{
    // Remove all calls
    sip_calls_clear();
    // Remove Call-id hash table
    hdestroy();
    // Remove calls vector
    vector_destroy(calls.list);
    vector_destroy(calls.active);
    // Deallocate regular expressions
    regfree(&calls.reg_method);
    regfree(&calls.reg_callid);
    regfree(&calls.reg_xcallid);
    regfree(&calls.reg_response);
    regfree(&calls.reg_cseq);
    regfree(&calls.reg_from);
    regfree(&calls.reg_to);
}


char *
sip_get_callid(const char* payload, char *callid)
{
    regmatch_t pmatch[3];

    // Try to get Call-ID from payload
    if (regexec(&calls.reg_callid, payload, 3, pmatch, 0) == 0) {
        // Copy the matching part of payload
        strncpy(callid, payload + pmatch[2].rm_so, (int) pmatch[2].rm_eo - pmatch[2].rm_so);
    }

    return callid;
}

char *
sip_get_xcallid(const char *payload, char *xcallid)
{
    regmatch_t pmatch[3];

    // Try to get X-Call-ID from payload
    if (regexec(&calls.reg_xcallid, (const char *)payload, 3, pmatch, 0) == 0) {
        strncpy(xcallid, (const char *)payload +  pmatch[2].rm_so, (int)pmatch[2].rm_eo - pmatch[2].rm_so);
    }

    return xcallid;
}

sip_msg_t *
sip_check_packet(capture_packet_t *packet)
{
    ENTRY entry;
    sip_msg_t *msg;
    sip_call_t *call;
    char callid[1024], xcallid[1024];
    const char *src, *dst;
    u_short sport, dport;
    char msg_src[ADDRESSLEN];
    char msg_dst[ADDRESSLEN];
    u_char payload[MAX_SIP_PAYLOAD];

    // Max SIP payload allowed
    if (packet->payload_len > MAX_SIP_PAYLOAD)
        return NULL;

    // Get Addresses from packet
    src = packet->ip_src;
    dst = packet->ip_dst;
    sport = packet->sport;
    dport = packet->dport;

    // Initialize local variables
    memset(callid, 0, sizeof(callid));
    memset(callid, 0, sizeof(xcallid));
    memset(msg_src, 0, sizeof(msg_src));
    memset(msg_dst, 0, sizeof(msg_dst));

    // Get payload from packet(s)
    memset(payload, 0, MAX_SIP_PAYLOAD);
    memcpy(payload, capture_packet_get_payload(packet), capture_packet_get_payload_len(packet));

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
        sip_get_callid((const char*) payload, xcallid);

        // Create the call if not found
        if (!(call = call_create(callid, xcallid)))
            goto skip_message;

        // Store this call in hash table
        entry.key = (char *) call->callid;
        entry.data = (void *) call;
        hsearch(entry, ENTER);

        // Append this call to the call list
        vector_append(calls.list, call);
        call->index = vector_count(calls.list);
    }

    // Store sorce address. Prefix too long IPv6 addresses with two dots
    if (strlen(src) > 15) {
        sprintf(msg_src, "..%s", src + strlen(src) - 13);
    } else {
        strcpy(msg_src, src);
    }

    // Store destination address. Prefix too long IPv6 addresses with two dots
    if (strlen(dst) > 15) {
        sprintf(msg_dst, "..%s", dst + strlen(dst) - 13);
    } else {
        strcpy(msg_dst, dst);
    }

    // At this point we know we're handling an interesting SIP Packet
    msg->packet = packet;

    // Always parse first call message
    if (call_msg_count(call) == 0) {
        // Parse SIP payload
        sip_parse_msg_payload(msg, payload);
    }

    // Add the message to the call
    call_add_message(call, msg);

    if (call_is_invite(call)) {
        // Parse media data
        sip_parse_msg_media(msg, payload);
        // Update Call State
        call_update_state(call, msg);
        // Check if this call should be in active call list
        if (call_is_active(call)) {
            if (vector_index(calls.active, call) == -1) {
                vector_append(calls.active, call);
            }
        } else {
            if (vector_index(calls.active, call) != -1) {
                vector_remove(calls.active, call);
            }
        }
    }

    // Return the loaded message
    return msg;

skip_message:
    // Deallocate message memory
    msg_destroy(msg);
    return NULL;

}

int
sip_calls_count()
{
    return vector_count(calls.list);
}

vector_iter_t
sip_calls_iterator()
{
    return vector_iterator(calls.list);
}

vector_iter_t
sip_active_calls_iterator()
{
    return vector_iterator(calls.active);
}

void
sip_calls_stats(int *total, int *displayed)
{
    vector_iter_t it = vector_iterator(calls.list);

    // Total number of calls without filtering
    *total = vector_iterator_count(&it);
    // Total number of calls after filtering
    vector_iterator_set_filter(&it, filter_check_call);
    *displayed = vector_iterator_count(&it);
}

sip_call_t *
sip_find_by_index(int index)
{
    return vector_item(calls.list, index);
}

sip_call_t *
sip_find_by_callid(const char *callid)
{
    ENTRY entry, *eptr;

    entry.key = (char *) callid;
    if ((eptr = hsearch(entry, FIND)))
        return eptr->data;

    return NULL;
}

sip_call_t *
sip_find_by_xcallid(const char *xcallid)
{
    sip_call_t *cur;
    vector_iter_t it;

    // Find the call with the given X-Call-Id
    while ((cur = vector_iterator_next(&it))) {
        if (cur->xcallid && !strcmp(cur->xcallid, xcallid)) {
            return cur;
        }
    }
    // None found
    return NULL;
}


sip_call_t *
call_get_xcall(sip_call_t *call)
{
    sip_call_t *xcall;
    if (call->xcallid) {
        xcall = sip_find_by_callid(call->xcallid);
    } else {
        xcall = sip_find_by_xcallid(call->callid);
    }
    return xcall;
}

int
sip_get_msg_reqresp(sip_msg_t *msg, const u_char *payload)
{
    regmatch_t pmatch[3];
    char reqresp[20];

    // If not already parsed
    if (!msg->reqresp) {
        // Initialize variables
        memset(reqresp, 0, sizeof(reqresp));

        // Method & CSeq
        if (regexec(&calls.reg_method, (const char *)payload, 2, pmatch, 0) == 0) {
            sprintf(reqresp, "%.*s", (int)(pmatch[1].rm_eo - pmatch[1].rm_so), payload + pmatch[1].rm_so);
        }

        // Response code
        if (regexec(&calls.reg_response, (const char *)payload, 3, pmatch, 0) == 0) {
            sprintf(reqresp, "%.*s", (int)(pmatch[2].rm_eo - pmatch[2].rm_so), payload + pmatch[2].rm_so);
        }

        // Get Request/Response Code
        msg->reqresp = sip_method_from_str(reqresp);
    }

    return msg->reqresp;
}

const char *
sip_get_response_str(sip_msg_t *msg, char *out)
{
    regmatch_t pmatch[3];
    const char *payload;

    // If not already parsed
    if (msg_is_request(msg))
        return NULL;

    // Get message payload
    payload = msg_get_payload(msg);

    // Response code (full text)
    if (regexec(&calls.reg_response, payload, 3, pmatch, 0) == 0) {
        sprintf(out, "%.*s", (int)(pmatch[1].rm_eo - pmatch[1].rm_so), payload + pmatch[1].rm_so);
    }

    return out;
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
    regmatch_t pmatch[4];
    char cseq[11];

    // CSeq
    if (regexec(&calls.reg_cseq, (char*)payload, 2, pmatch, 0) == 0) {
        sprintf(cseq, "%.*s", (int)(pmatch[1].rm_eo - pmatch[1].rm_so), payload + pmatch[1].rm_so);
        msg->cseq = atoi(cseq);
    }

    // From
    if (regexec(&calls.reg_from, (const char *)payload, 4, pmatch, 0) == 0) {
        msg->sip_from = sng_malloc((int)pmatch[2].rm_eo - pmatch[2].rm_so + 1);
        strncpy(msg->sip_from, (const char *)payload +  pmatch[2].rm_so, (int)pmatch[2].rm_eo - pmatch[2].rm_so);
    }

    // To
    if (regexec(&calls.reg_to, (const char *)payload, 4, pmatch, 0) == 0) {
        msg->sip_to = sng_malloc((int)pmatch[2].rm_eo - pmatch[2].rm_so + 1);
        strncpy(msg->sip_to, (const char *)payload +  pmatch[2].rm_so, (int)pmatch[2].rm_eo - pmatch[2].rm_so);
    }

    return 0;
}

void
sip_parse_msg_media(sip_msg_t *msg, const u_char *payload)
{
    char address[ADDRESSLEN];
    char media_address[ADDRESSLEN] = { };
    char media_type[15] = { };
    char media_format[30] = { };
    int media_port;
    u_int media_fmt_pref;
    u_int media_fmt_code;
    sdp_media_t *media = NULL;
    char *payload2, *tofree, *line;

    // Initialize variables
    memset(address, 0, sizeof(address));

    // Parse each line of payload looking for sdp information
    tofree = payload2 = strdup((char*)payload);
    while ((line = strsep(&payload2, "\r\n")) != NULL) {
        // Check if we have a media string
        if (!strncmp(line, "m=", 2)) {
            if (sscanf(line, "m=%s %d RTP/%*s %u", media_type, &media_port, &media_fmt_pref) == 3) {
                // Create a new media structure for this message
                if ((media = media_create(msg))) {
                    media_set_type(media, media_type);
                    media_set_port(media, media_port);
                    media_set_address(media, media_address);
                    media_set_prefered_format(media, media_fmt_pref);
                    msg_add_media(msg, media);

                    /**
                     * From SDP we can only guess destination address port. RTP Capture process
                     * will determine when the stream has been completed, getting source address
                     * and port of the stream.
                     */
                    // Create a new stream with this destination address:port
                    if (!call_msg_is_retrans(msg)) {
                        call_add_stream(msg_get_call(msg), stream_create(media, media_address, media_port));
                    }
                }
            }
        }

        // Check if we have a connection string
        if (!strncmp(line, "c=", 2)) {
            if (sscanf(line, "c=IN IP4 %s", media_address) && media) {
                media_set_address(media, media_address);
            }
        }

        // Check if we have attribute format string
        if (!strncmp(line, "a=rtpmap:", 9)) {
            if (media && sscanf(line, "a=rtpmap:%u %[^ ]", &media_fmt_code, media_format)) {
                media_add_format(media, media_fmt_code, media_format);
            }
        }

    }
    sng_free(tofree);
}


void
sip_calls_clear()
{
    // Create again the callid hash table
    hdestroy();
    hcreate(calls.limit);
    // Remove all items from vector
    vector_clear(calls.list);
    vector_clear(calls.active);
}

int
sip_set_match_expression(const char *expr, int insensitive, int invert)
{
    // Store expression text
    calls.match_expr = expr;
    // Set invert flag
    calls.match_invert = invert;

#ifdef WITH_PCRE
    const char *re_err = NULL;
    int32_t err_offset;
    int32_t pflags = PCRE_UNGREEDY;

    if (insensitive)
        pflags |= PCRE_CASELESS;

    // Check if we have a valid expression
    calls.match_regex = pcre_compile(expr, pflags, &re_err, &err_offset, 0);
    return calls.match_regex == NULL;
#else
    int cflags = REG_EXTENDED;

    // Case insensitive requested
    if (insensitive)
        cflags |= REG_ICASE;

    // Check the expresion is a compilable regexp
    return regcomp(&calls.match_regex, expr, cflags) != 0;
#endif
}

int
sip_check_match_expression(const char *payload)
{
    // Everything matches when there is no match
    if (!calls.match_expr)
        return 1;

#ifdef WITH_PCRE
    switch (pcre_exec(calls.match_regex, 0, payload, strlen(payload), 0, 0, 0, 0)) {
        case PCRE_ERROR_NOMATCH:
            return 1 == calls.match_invert;
    }

    return 0 == calls.match_invert;
#else
    // Check if payload matches the given expresion
    return (regexec(&calls.match_regex, payload, 0, NULL, 0) == calls.match_invert);
#endif
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
        case CAPTURE_PACKET_SIP_UDP:
            return "UDP";
        case CAPTURE_PACKET_SIP_TCP:
            return "TCP";
        case CAPTURE_PACKET_SIP_TLS:
            return "TLS";
        case CAPTURE_PACKET_SIP_WS:
            return "WS";
        case CAPTURE_PACKET_SIP_WSS:
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

const char *
sip_address_format(const char *address)
{
    // Return address formatted depending on active settings
    if (setting_enabled(SETTING_DISPLAY_ALIAS)) {
        return get_alias_value(address);
    } else if (setting_enabled(SETTING_DISPLAY_ALIAS)) {
        return lookup_hostname(address);
    } else {
        return address;
    }
}

const char *
sip_address_port_format(const char *addrport)
{
    static char aport[ADDRESSLEN + 6];
    char address[ADDRESSLEN + 6];
    int port;

    strncpy(aport, addrport, sizeof(aport));
    if (sscanf(aport, "%[^:]:%d", address, &port) == 2) {
        sprintf(aport, "%s:%d", sip_address_format(address), port);
    }

    return aport;
}
