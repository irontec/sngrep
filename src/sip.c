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
 *
 * @todo Replace structures for their typedef shorter names
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

    // Create hash table for callid search
    hcreate(calls.limit);

    // Initialize calls lock
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__) || defined(BSD) || defined (__OpenBSD__)
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
    pthread_mutex_init(&calls.lock, &attr);

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
    // Remove calls vector
    vector_destroy(calls.list);
    // Remove Call-id hash table
    hdestroy();
    // Deallocate regular expressions
    regfree(&calls.reg_method);
    regfree(&calls.reg_callid);
    regfree(&calls.reg_xcallid);
    regfree(&calls.reg_response);
    regfree(&calls.reg_cseq);
    regfree(&calls.reg_from);
    regfree(&calls.reg_to);
    // Remove calls mutex
    pthread_mutex_destroy(&calls.lock);
}


char *
sip_get_callid(const char* payload)
{
    char *callid = NULL;
    regmatch_t pmatch[3];

    // Try to get Call-ID from payload
    if (regexec(&calls.reg_callid, payload, 3, pmatch, 0) == 0) {
        // Call-ID is the firts regexp match
        callid = malloc(pmatch[2].rm_eo - pmatch[2].rm_so + 1);
        // Allocate memory for Call-Id (caller MUST free it)
        memset(callid, 0, pmatch[2].rm_eo - pmatch[2].rm_so + 1);
        // Copy the matching part of payload
        strncpy(callid, payload + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
    }

    return callid;
}

sip_msg_t *
sip_load_message(capture_packet_t *packet, const char *src, u_short sport, const char* dst, u_short dport, u_char *payload)
{
    ENTRY entry;
    sip_msg_t *msg;
    sip_call_t *call;
    int call_idx;
    char *callid;
    char msg_src[ADDRESSLEN];
    char msg_dst[ADDRESSLEN];

    // Get the Call-ID of this message
    if (!(callid = sip_get_callid((const char*) payload))) {
        return NULL;
    }

    // Create a new message from this data
    if (!(msg = msg_create((const char*) payload))) {
        return NULL;
    }

    // Get Method and request for the following checks
    // There is no need to parse all payload at this point
    // If no response or request code is found, this is not a SIP message
    if (!sip_get_msg_reqresp(msg, payload)) {
        // Deallocate message memory
        msg_destroy(msg);
        free(callid);
        return NULL;
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

    // Set Source and Destination attributes
    msg_set_attribute(msg, SIP_ATTR_SRC, "%s:%u", msg_src, sport);
    msg_set_attribute(msg, SIP_ATTR_DST, "%s:%u", msg_dst, dport);

    // If payload is encrypted, dup payload
    if (packet->type == CAPTURE_PACKET_SIP_TLS)
        msg->payload = (u_char *)strdup((const char *)payload);

    pthread_mutex_lock(&calls.lock);
    // Find the call for this msg
    if (!(call = sip_find_by_callid(callid))) {

        // Check if payload matches expression
        if (!sip_check_match_expression((const char*) payload)) {
            // Deallocate message memory
            msg_destroy(msg);
            free(callid);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }

        // User requested only INVITE starting dialogs
        if (calls.only_calls) {
            if (msg->reqresp != SIP_METHOD_INVITE) {
                // Deallocate message memory
                msg_destroy(msg);
                free(callid);
                pthread_mutex_unlock(&calls.lock);
                return NULL;
            }
        }

        // Only create a new call if the first msg
        // is a request message in the following gorup
        if (calls.ignore_incomplete) {
            if (msg->reqresp > SIP_METHOD_MESSAGE) {
                // Deallocate message memory
                msg_destroy(msg);
                free(callid);
                pthread_mutex_unlock(&calls.lock);
                return NULL;
            }
        }

        // Check if this message is ignored by configuration directive
        if (sip_check_msg_ignore(msg)) {
            // Deallocate message memory
            msg_destroy(msg);
            free(callid);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }

        // Create the call if not found
        if (!(call = call_create(callid))) {
            // Deallocate message memory
            msg_destroy(msg);
            free(callid);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }

        // Store this call in hash table
        entry.key = call->callid;
        entry.data = (void *) call;
        hsearch(entry, ENTER);

        // Append this call to the call list
        pthread_mutex_lock(&calls.lock);
        call_idx = vector_append(calls.list, call);
        pthread_mutex_unlock(&calls.lock);

        // Store current call Index
        call_set_attribute(call, SIP_ATTR_CALLINDEX, "%d", call_idx);
    }

    // Set message callid
    msg_set_attribute(msg, SIP_ATTR_CALLID, callid);
    // Store Transport attribute
    if (packet->type == CAPTURE_PACKET_SIP_UDP) {
        msg_set_attribute(msg, SIP_ATTR_TRANSPORT, "UDP");
    } else if (packet->type == CAPTURE_PACKET_SIP_TCP) {
        msg_set_attribute(msg, SIP_ATTR_TRANSPORT, "TCP");
    } else if (packet->type == CAPTURE_PACKET_SIP_TLS) {
        msg_set_attribute(msg, SIP_ATTR_TRANSPORT, "TLS");
    } else if (packet->type == CAPTURE_PACKET_SIP_WS) {
        msg_set_attribute(msg, SIP_ATTR_TRANSPORT, "WS");
    }

    // Dellocate callid memory
    free(callid);

    // Add this SIP packet to the message
    msg_add_packet(msg, packet);
    // Add the message to the call
    call_add_message(call, msg);
    // Parse SIP payload
    sip_parse_msg_payload(msg, payload);
    // Parse media data
    sip_parse_msg_media(msg, payload);
    // Update Call State
    call_update_state(call, msg);
    pthread_mutex_unlock(&calls.lock);

    // Return the loaded message
    return msg;
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

void
sip_calls_stats(int *total, int *displayed)
{
    vector_iter_t it = vector_iterator(calls.list);

    pthread_mutex_lock(&calls.lock);
    // Total number of calls without filtering
    *total = vector_iterator_count(&it);
    // Total number of calls after filtering
    vector_iterator_set_filter(&it, filter_check_call);
    *displayed = vector_iterator_count(&it);
    pthread_mutex_unlock(&calls.lock);
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
    const char *cur_xcallid;
    sip_call_t *cur;
    int i;

    for (i=0; i < vector_count(calls.list); i++) {
        cur = vector_item(calls.list, i);
        cur_xcallid = call_get_attribute(cur, SIP_ATTR_XCALLID);
        if (cur_xcallid && !strcmp(cur_xcallid, xcallid)) {
            return cur;
        }
    }
    return NULL;
}


sip_call_t *
call_get_xcall(sip_call_t *call)
{
    sip_call_t *xcall;
    pthread_mutex_lock(&calls.lock);
    if (call_get_attribute(call, SIP_ATTR_XCALLID)) {
        xcall = sip_find_by_callid(call_get_attribute(call, SIP_ATTR_XCALLID));
    } else {
        xcall = sip_find_by_xcallid(call_get_attribute(call, SIP_ATTR_CALLID));
    }
    pthread_mutex_unlock(&calls.lock);
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
            sprintf(reqresp, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, payload + pmatch[1].rm_so);
            msg_set_attribute(msg, SIP_ATTR_METHOD, reqresp);
        }

        // Response code
        if (regexec(&calls.reg_response, (const char *)payload, 3, pmatch, 0) == 0) {
            msg_set_attribute(msg, SIP_ATTR_METHOD, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so,
                              payload + pmatch[1].rm_so);
            sprintf(reqresp, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so, payload + pmatch[2].rm_so);
        }

        // Get Request/Response Code
        msg->reqresp = sip_method_from_str(reqresp);
    }

    return msg->reqresp;
}

int
sip_parse_msg_payload(sip_msg_t *msg, const u_char *payload)
{
    regmatch_t pmatch[4];
    char date[12], time[20], cseq[11];

    // CSeq
    if (regexec(&calls.reg_cseq, (char*)payload, 2, pmatch, 0) == 0) {
        sprintf(cseq, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, payload + pmatch[1].rm_so);
        msg->cseq = atoi(cseq);
    }

    // X-Call-Id
    if (regexec(&calls.reg_xcallid, (const char *)payload, 3, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_XCALLID, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
    }

    // From
    if (regexec(&calls.reg_from, (const char *)payload, 4, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_SIPFROM, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
        msg_set_attribute(msg, SIP_ATTR_SIPFROMUSER, "%.*s", pmatch[3].rm_eo - pmatch[3].rm_so,
                          payload + pmatch[3].rm_so);
    }

    // To
    if (regexec(&calls.reg_to, (const char *)payload, 4, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_SIPTO, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
        msg_set_attribute(msg, SIP_ATTR_SIPTOUSER, "%.*s", pmatch[3].rm_eo - pmatch[3].rm_so,
                          payload + pmatch[3].rm_so);
    }

    // Set message Date and Time attribute
    msg_set_attribute(msg, SIP_ATTR_DATE, timeval_to_date(msg_get_time(msg), date));
    msg_set_attribute(msg, SIP_ATTR_TIME, timeval_to_time(msg_get_time(msg), time));

    return 0;
}

void
sip_parse_msg_media(sip_msg_t *msg, const u_char *payload)
{
    regmatch_t pmatch[4];
    char address[ADDRESSLEN];
    char media_address[ADDRESSLEN] = { };
    char media_type[15] = { };
    char media_format[30] = { };
    int media_port;
    int media_fmt_pref;
    int media_fmt_code;
    sdp_media_t *media = NULL;
    int port = 0;
    char *payload2, *tofree, *line;

    // Initialize variables
    memset(address, 0, sizeof(address));

    // Parse each line of payload looking for sdp information
    tofree = payload2 = strdup((char*)payload);
    while ((line = strsep(&payload2, "\r\n")) != NULL) {
        // Check if we have a media string
        if (!strncmp(line, "m=", 2)) {
            if (sscanf(line, "m=%s %d RTP/AVP %d", media_type, &media_port, &media_fmt_pref) == 3) {
                // Create a new media structure for this message
                if ((media = media_create(msg))) {
                    media_set_type(media, media_type);
                    media_set_port(media, media_port);
                    media_set_address(media, media_address);
                    media_set_format_code(media, media_fmt_pref);
                    vector_append(msg->medias, media);

                    /**
                     * From SDP we can only guess destination address port. RTP Capture process
                     * will determine when the stream has been completed, getting source address
                     * and port of the stream.
                     */
                    // Create a new stream with this destination address:port
                    if (!msg_is_retrans(msg)) {
                        vector_append(msg->call->streams, stream_create(media, media_address, media_port));
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
            if (sscanf(line, "a=rtpmap:%d %[^ ]", &media_fmt_code, media_format)) {
                if (media && media_fmt_pref == media_fmt_code) {
                    media_set_format(media, media_format);
                }
            }
        }
    }
    free(tofree);

    // If message has media
    if ((media = vector_first(msg->medias))) {
        msg_set_attribute(msg, SIP_ATTR_SDP_ADDRESS, media_get_address(media));
        msg_set_attribute(msg, SIP_ATTR_SDP_PORT, "%d", media_get_port(media));
    }

}


void
sip_calls_clear()
{
    pthread_mutex_lock(&calls.lock);
    // Remove all items from vector
    vector_clear(calls.list);
    // Create again the callid hash table
    hdestroy();
    hcreate(calls.limit);
    pthread_mutex_unlock(&calls.lock);
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


int
sip_check_msg_ignore(sip_msg_t *msg)
{
    int i;
    sip_attr_hdr_t *header;

    // Check if an ignore option exists
    for (i = 0; i < SIP_ATTR_COUNT; i++) {
        header = sip_attr_get_header(i);
        if (is_ignored_value(header->name, msg_get_attribute(msg, header->id))) {
            return 1;
        }
    }
    return 0;
}

const char *
sip_method_str(enum sip_methods method)
{
    switch (method) {
        case SIP_METHOD_REGISTER:
            return "REGISTER";
        case SIP_METHOD_INVITE:
            return "INVITE";
        case SIP_METHOD_SUBSCRIBE:
            return "SUBSCRIBE";
        case SIP_METHOD_NOTIFY:
            return "NOTIFY";
        case SIP_METHOD_OPTIONS:
            return "OPTIONS";
        case SIP_METHOD_PUBLISH:
            return "PUBLISH";
        case SIP_METHOD_MESSAGE:
            return "MESSAGE";
        case SIP_METHOD_CANCEL:
            return "CANCEL";
        case SIP_METHOD_BYE:
            return "BYE";
        case SIP_METHOD_ACK:
            return "ACK";
        case SIP_METHOD_PRACK:
            return "PRACK";
        case SIP_METHOD_INFO:
            return "INFO";
        case SIP_METHOD_REFER:
            return "REFER";
        case SIP_METHOD_UPDATE:
            return "UPDATE";
        case SIP_METHOD_SENTINEL:
            return "";
    }
    return NULL;
}

int
sip_method_from_str(const char *method)
{
    int i;
    for (i = 1; i < SIP_METHOD_SENTINEL; i++)
        if (!strcmp(method, sip_method_str(i)))
            return i;
    return atoi(method);
}

char *
sip_get_msg_header(sip_msg_t *msg, char *out)
{
    // Source and Destination address
    char from_addr[80], to_addr[80];

    // We dont use Message attributes here because it contains truncated data
    // This should not overload too much as all results should be already cached
    sprintf(from_addr, "%s", sip_address_port_format(SRC(msg)));
    sprintf(to_addr, "%s", sip_address_port_format(DST(msg)));

    // Get msg header
    sprintf(out, "%s %s %s -> %s", DATE(msg), TIME(msg), from_addr, to_addr);
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
