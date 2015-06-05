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
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <search.h>
#include <stdarg.h>
#include "sip.h"
#include "option.h"
#include "capture.h"
#include "filter.h"

/**
 * @brief Linked list of parsed calls
 *
 * All parsed calls will be added to this list, only accesible from
 * this awesome structure, so, keep it thread-safe.
 */
static sip_call_list_t calls =
    { 0 };

void
sip_init(int limit, int only_calls, int no_incomplete)
{
    int match_flags;

    // Store capture limit
    calls.limit = limit;
    calls.only_calls = only_calls;
    calls.ignore_incomplete = no_incomplete;

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
    regcomp(&calls.reg_sdp, "^Content-Type:[ ]* application/sdp\r$", match_flags);
    regcomp(&calls.reg_sdp_addr, "^c=[^ ]+ [^ ]+ (.+)\r$", match_flags);
    regcomp(&calls.reg_sdp_port, "^m=[^ ]+ ([0-9]+)", match_flags);

}

sip_msg_t *
sip_msg_create(const char *payload)
{
    sip_msg_t *msg;

    if (!(msg = malloc(sizeof(sip_msg_t))))
        return NULL;
    memset(msg, 0, sizeof(sip_msg_t));
    msg->payload = strdup(payload);
    msg->color = 0;
    return msg;
}

void
sip_msg_destroy(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;

    if (!msg)
        return;

    // If the message belongs to a call, remove it from
    // its message list
    if (msg->call) {
        if ((prev = call_get_prev_msg(msg->call, msg))) {
            prev->next = msg->next;
        } else {
            msg->call->msgs = msg->next;
        }
        if (msg->next) {
            msg->next->prev = msg->prev;
        }
    }

    // Free message attribute list
    sip_attr_list_destroy(msg->attrs);

    // Free message payload pointer if not parsed
    if (msg->payload)
        free(msg->payload);

    // Free packet data
    if (msg->pcap_header)
        free(msg->pcap_header);
    if (msg->pcap_packet)
        free(msg->pcap_packet);

    // Free all memory
    free(msg);
}

sip_call_t *
sip_call_create(char *callid)
{
    ENTRY entry;
    // Initialize a new call structure
    sip_call_t *call = malloc(sizeof(sip_call_t));
    memset(call, 0, sizeof(sip_call_t));

    if (!calls.count) {
        calls.first = call;
    } else {
        call->prev = calls.last;
        calls.last->next = call;
    }
    calls.last = call;
    calls.count++;

    // Store this call in hash table
    entry.key = strdup(callid);
    entry.data = (void *) call;
    hsearch(entry, ENTER);

    // Initialize call filter status
    call->filtered = -1;

    // Store current call Index
    call_set_attribute(call, SIP_ATTR_CALLINDEX, "%d", calls.count);

    return call;
}

void
sip_call_destroy(sip_call_t *call)
{
    // No call to destroy
    if (!call)
        return;

    // If removing the first call, update list head
    if (call == calls.first)
        calls.first = call->next;
    // If removing the last call, update the list tail
    if (call == calls.last)
        calls.last = call->prev;
    // Update previous call
    if (call->prev)
        call->prev->next = call->next;
    // Update next call
    if (call->next)
        call->next->prev = call->prev;
    // Update call counter
    calls.count--;

    // Remove all messages
    while (call->msgs)
        sip_msg_destroy(call->msgs);

    // Remove all call attributes
    sip_attr_list_destroy(call->attrs);

    // Free it!
    free(call);
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
sip_load_message(const struct pcap_pkthdr *header, const char *src, u_short sport, const char* dst,
                 u_short dport, u_char *payload)
{
    sip_msg_t *msg;
    sip_call_t *call;
    char *callid;

    // Get the Call-ID of this message
    if (!(callid = sip_get_callid((const char*) payload))) {
        return NULL;
    }

    // Create a new message from this data
    if (!(msg = sip_msg_create((const char*) payload))) {
        return NULL;
    }

    // Get Method and request for the following checks
    // There is no need to parse all payload at this point
    // If no response or request code is found, this is not a SIP message
    if (!msg_get_reqresp(msg)) {
        // Deallocate message memory
        sip_msg_destroy(msg);
        return NULL;
    }

    // Store sorce address. Prefix too long IPv6 addresses with two dots
    if (strlen(src) > 15) {
        sprintf(msg->src, "..%s", src + strlen(src) - 13);
    } else {
        strcpy(msg->src, src);
    }

    // Store destination address. Prefix too long IPv6 addresses with two dots
    if (strlen(dst) > 15) {
        sprintf(msg->dst, "..%s", dst + strlen(dst) - 13);
    } else {
        strcpy(msg->dst, dst);
    }

    // Fill message data
    msg->sport = sport;
    msg->dport = dport;

    // Set message PCAP header
    msg->pcap_header = malloc(sizeof(struct pcap_pkthdr));
    memcpy(msg->pcap_header, header, sizeof(struct pcap_pkthdr));

    pthread_mutex_lock(&calls.lock);
    // Find the call for this msg
    if (!(call = call_find_by_callid(callid))) {

        // Check if payload matches expression
        if (!sip_check_match_expression((const char*) payload)) {
            // Deallocate message memory
            sip_msg_destroy(msg);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }

        // User requested only INVITE starting dialogs
        if (calls.only_calls) {
            if (msg->reqresp != SIP_METHOD_INVITE) {
                // Deallocate message memory
                sip_msg_destroy(msg);
                pthread_mutex_unlock(&calls.lock);
                return NULL;
            }
        }

        // Only create a new call if the first msg
        // is a request message in the following gorup
        if (calls.ignore_incomplete) {
            if (msg->reqresp > SIP_METHOD_MESSAGE) {
                // Deallocate message memory
                sip_msg_destroy(msg);
                pthread_mutex_unlock(&calls.lock);
                return NULL;
            }
        }

        // Check if this message is ignored by configuration directive
        if (sip_check_msg_ignore(msg)) {
            // Deallocate message memory
            sip_msg_destroy(msg);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }

        // Create the call if not found
        if (!(call = sip_call_create(callid))) {
            // Deallocate message memory
            sip_msg_destroy(msg);
            pthread_mutex_unlock(&calls.lock);
            return NULL;
        }
    }

    // Set message callid
    msg_set_attribute(msg, SIP_ATTR_CALLID, callid);
    // Dellocate callid memory
    free(callid);

    // Add the message to the found/created call
    call_add_message(call, msg);

    // Parse media data
    msg_parse_media(msg);

    // Update Call State
    call_update_state(call, msg);

    pthread_mutex_unlock(&calls.lock);

    // Return the loaded message
    return msg;
}

int
sip_calls_count()
{
    return calls.count;
}

void
call_add_message(sip_call_t *call, sip_msg_t *msg)
{
    // Set the message owner
    msg->call = call;

    // Put this msg at the end of the msg list
    if (!call->msgs) {
        call->msgs = msg;
        msg->prev = NULL;
    } else {
        call->last_msg->next = msg;
        msg->prev = call->last_msg;
    }
    call->last_msg = msg;

    // Increase message count
    call->msgcnt++;

    // Store message count
    call_set_attribute(call, SIP_ATTR_MSGCNT, "%d", call->msgcnt);
}

sip_call_t *
call_find_by_callid(const char *callid)
{
    ENTRY entry, *eptr;

    entry.key = (char *) callid;
    if ((eptr = hsearch(entry, FIND)))
        return eptr->data;

    return NULL;
}

sip_call_t *
call_find_by_xcallid(const char *xcallid)
{
    const char *cur_xcallid;

    sip_call_t *cur = calls.first;
    while (cur) {
        cur_xcallid = call_get_attribute(cur, SIP_ATTR_XCALLID);
        if (cur_xcallid && !strcmp(cur_xcallid, xcallid)) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

int
call_msg_count(sip_call_t *call)
{
    return call->msgcnt;
}

sip_call_t *
call_get_xcall(sip_call_t *call)
{
    sip_call_t *xcall;
    pthread_mutex_lock(&calls.lock);
    if (call_get_attribute(call, SIP_ATTR_XCALLID)) {
        xcall = call_find_by_callid(call_get_attribute(call, SIP_ATTR_XCALLID));
    } else {
        xcall = call_find_by_xcallid(call_get_attribute(call, SIP_ATTR_CALLID));
    }
    pthread_mutex_unlock(&calls.lock);
    return xcall;
}

sip_msg_t *
call_get_next_msg(sip_call_t *call, sip_msg_t *msg)
{
    sip_msg_t *ret;
    pthread_mutex_lock(&calls.lock);
    if (msg == NULL) {
        ret = call->msgs;
    } else {
        ret = msg->next;
    }

    // Parse message if not parsed
    if (ret && !ret->parsed)
        msg_parse_payload(ret, ret->payload);

    pthread_mutex_unlock(&calls.lock);
    return ret;
}

sip_msg_t *
call_get_prev_msg(sip_call_t *call, sip_msg_t *msg)
{
    sip_msg_t *ret = NULL;
    pthread_mutex_lock(&calls.lock);
    if (msg == NULL) {
        // No message, no previous
        ret = NULL;
    } else {
        // Get previous message
        ret = msg->prev;
    }

    // Parse message if not parsed
    if (ret && !ret->parsed)
        msg_parse_payload(ret, ret->payload);

    pthread_mutex_unlock(&calls.lock);
    return ret;
}

sip_call_t *
call_get_next(sip_call_t *cur)
{

    sip_call_t * next;
    pthread_mutex_lock(&calls.lock);
    if (!cur) {
        next = calls.first;
    } else {
        next = cur->next;
    }
    pthread_mutex_unlock(&calls.lock);
    return next;
}

sip_call_t *
call_get_prev(sip_call_t *cur)
{

    sip_call_t *prev;
    pthread_mutex_lock(&calls.lock);
    if (!cur) {
        prev = calls.first;
    } else {
        prev = cur->prev;
    }
    pthread_mutex_unlock(&calls.lock);
    return prev;
}

sip_call_t *
call_get_next_filtered(sip_call_t *cur)
{
    sip_call_t *next = call_get_next(cur);

    pthread_mutex_lock(&calls.lock);
    // Return next not filtered call
    if (next && filter_check_call(next))
        next = call_get_next_filtered(next);
    pthread_mutex_unlock(&calls.lock);

    return next;
}

sip_call_t *
call_get_prev_filtered(sip_call_t *cur)
{
    sip_call_t *prev = call_get_prev(cur);

    pthread_mutex_lock(&calls.lock);
    // Return previous call if this one is filtered
    if (prev && filter_check_call(prev))
        prev = call_get_prev_filtered(prev);
    pthread_mutex_unlock(&calls.lock);
    return prev;
}

void
call_update_state(sip_call_t *call, sip_msg_t *msg)
{
    const char *callstate;
    char dur[20];
    int reqresp;

    // Sanity check
    if (!call || !call->msgs || !msg)
        return;

    // Check First message of Call has INVITE method
    if (call->msgs->reqresp != SIP_METHOD_INVITE) {
        return;
    }

    // Get current message Method / Response Code
    reqresp = msg_get_reqresp(msg);

    // If this message is actually a call, get its current state
    if ((callstate = call_get_attribute(call, SIP_ATTR_CALLSTATE))) {
        if (!strcmp(callstate, SIP_CALLSTATE_CALLSETUP)) {
            if (reqresp == 200) {
                // Alice and Bob are talking
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_INCALL);
                // Store the timestap where call has started
                call->cstart_msg = msg;
            } else if (reqresp == SIP_METHOD_CANCEL) {
                // Alice is not in the mood
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_CANCELLED);
                // Store total call duration
                call_set_attribute(call, SIP_ATTR_TOTALDUR,
                                   sip_calculate_duration(call->msgs, msg, dur));
            } else if (reqresp > 400) {
                // Bob is not in the mood
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_REJECTED);
                // Store total call duration
                call_set_attribute(call, SIP_ATTR_TOTALDUR,
                                   sip_calculate_duration(call->msgs, msg, dur));
            }
        } else if (!strcmp(callstate, SIP_CALLSTATE_INCALL)) {
            if (reqresp == SIP_METHOD_BYE) {
                // Thanks for all the fish!
                call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_COMPLETED);
                // Store Conversation duration
                call_set_attribute(call, SIP_ATTR_CONVDUR,
                                   sip_calculate_duration(call->cstart_msg, msg, dur));
            }
        } else if (reqresp == SIP_METHOD_INVITE && strcmp(callstate, SIP_CALLSTATE_INCALL)) {
            // Call is being setup (after proper authentication)
            call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_CALLSETUP);
        } else {
            // Store total call duration
            call_set_attribute(call, SIP_ATTR_TOTALDUR,
                               sip_calculate_duration(call->msgs, msg, dur));
        }
    } else {
        // This is actually a call
        if (reqresp == SIP_METHOD_INVITE)
            call_set_attribute(call, SIP_ATTR_CALLSTATE, SIP_CALLSTATE_CALLSETUP);
    }

}

void
call_add_media(sip_call_t *call, sdp_media_t *media)
{
    sdp_media_t *tmp;

    //! Sanity check
    if (!call || !media)
        return;

    //! Add the first media to the call
    if (!call->medias) {
        call->medias = media;
    } else {
        //! Look for the last media of the call
        for (tmp = call->medias; tmp->next; tmp = tmp->next)
            ;
        tmp->next = media;
    }
}

int
msg_get_reqresp(sip_msg_t *msg)
{
    regmatch_t pmatch[3];
    char reqresp[20];
    const char *payload = msg->payload;

    // If not already parsed
    if (!msg->reqresp) {
        // Initialize variables
        memset(reqresp, 0, sizeof(reqresp));

        // Method & CSeq
        if (regexec(&calls.reg_method, payload, 2, pmatch, 0) == 0) {
            sprintf(reqresp, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, payload + pmatch[1].rm_so);
            msg_set_attribute(msg, SIP_ATTR_METHOD, reqresp);
        }

        // Response code
        if (regexec(&calls.reg_response, payload, 3, pmatch, 0) == 0) {
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
msg_parse_payload(sip_msg_t *msg, const char *payload)
{
    regmatch_t pmatch[4];
    char date[12], time[20], cseq[11];

    // If message has already been parsed, we've finished
    if (msg->parsed)
        return 0;

    // Get Method and request for the following checks
    msg_get_reqresp(msg);

    // CSeq
    if (regexec(&calls.reg_cseq, payload, 2, pmatch, 0) == 0) {
        sprintf(cseq, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, payload + pmatch[1].rm_so);
        msg->cseq = atoi(cseq);
    }

    // X-Call-Id
    if (regexec(&calls.reg_xcallid, payload, 3, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_XCALLID, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
    }

    // From
    if (regexec(&calls.reg_from, payload, 4, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_SIPFROM, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
        msg_set_attribute(msg, SIP_ATTR_SIPFROMUSER, "%.*s", pmatch[3].rm_eo - pmatch[3].rm_so,
                          payload + pmatch[3].rm_so);
    }

    // To
    if (regexec(&calls.reg_to, payload, 4, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_SIPTO, "%.*s", pmatch[2].rm_eo - pmatch[2].rm_so,
                          payload + pmatch[2].rm_so);
        msg_set_attribute(msg, SIP_ATTR_SIPTOUSER, "%.*s", pmatch[3].rm_eo - pmatch[3].rm_so,
                          payload + pmatch[3].rm_so);
    }

    // Set Source and Destination attributes
    msg_set_attribute(msg, SIP_ATTR_SRC, "%s:%u", msg->src, msg->sport);
    msg_set_attribute(msg, SIP_ATTR_DST, "%s:%u", msg->dst, msg->dport);

    // Set message Date attribute
    time_t t = (time_t) msg->pcap_header->ts.tv_sec;
    struct tm *timestamp = localtime(&t);
    strftime(date, sizeof(date), "%Y/%m/%d", timestamp);
    msg_set_attribute(msg, SIP_ATTR_DATE, date);

    // Set message Time attribute
    strftime(time, sizeof(time), "%H:%M:%S", timestamp);
    sprintf(time + 8, ".%06d", (int) msg->pcap_header->ts.tv_usec);
    msg_set_attribute(msg, SIP_ATTR_TIME, time);

    // Message payload has been parsed
    msg->parsed = 1;

    return 0;
}

void
msg_parse_media(sip_msg_t *msg)
{
    sdp_media_t *media;
    regmatch_t pmatch[4];
    sip_msg_t *req;
    char address[50];
    int port;

    // Check if this message has sdp
    if (regexec(&calls.reg_sdp, msg->payload, 0, 0, 0) != 0)
        return;

    // Message has SDP
    msg->sdp = 1;

    // SDP Address
    if (regexec(&calls.reg_sdp_addr, msg->payload, 2, pmatch, 0) == 0) {
        sprintf(address, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, msg->payload + pmatch[1].rm_so);
        msg_set_attribute(msg, SIP_ATTR_SDP_ADDRESS, address);
    }

    // SDP Port
    if (regexec(&calls.reg_sdp_port, msg->payload, 2, pmatch, 0) == 0) {
        msg_set_attribute(msg, SIP_ATTR_SDP_PORT, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so,
                          msg->payload + pmatch[1].rm_so);
        port = atoi(msg_get_attribute(msg, SIP_ATTR_SDP_PORT));
    }

    if (!strcmp(address, "0.0.0.0"))
        return;

    // Request methods create new media
    if (msg_is_request(msg)) {
        // Check if we have already added this media
        if (!media_find(msg->call->medias, address, port)) {
            call_add_media(msg->call, media_create(address, port));
        }
    } else {
        // Try to find the request for this response
        if ((req = msg_get_request_sdp(msg))) {
            const char *reqaddr = msg_get_attribute(req, SIP_ATTR_SDP_ADDRESS);
            int reqport = atoi(msg_get_attribute(req, SIP_ATTR_SDP_PORT));
            // Check if this media already exists
            if (!(media = media_find_pair(msg->call->medias, reqaddr, reqport, address, port))) {
                // Check if a media is pending to be completed
                if ((media = media_find_unpair(msg->call->medias, reqaddr, reqport))) {
                    media_add(media, address, port);
                }
            }
        }
    }
}

int
msg_is_retrans(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;

    // Sanity check
    if (!msg || !msg->call || !msg->payload)
        return 0;

    // Start on previous message
    prev = msg;

    // Check previous messages in same call
    while ((prev = call_get_prev_msg(msg->call, prev))) {
        // Check if the payload is exactly the same
        if (!strcasecmp(msg->payload, prev->payload)) {
            return 1;
        }
    }

    return 0;
}

int
msg_is_request(sip_msg_t *msg)
{
    return msg->reqresp < SIP_METHOD_SENTINEL;
}

sip_msg_t *
msg_get_request(sip_msg_t *msg)
{
    sip_msg_t *tmp = msg;

    if (!msg)
        return NULL;

    if (msg_is_request(msg))
        return NULL;

    for (tmp = call_get_prev_msg(msg->call, tmp); tmp; tmp = call_get_prev_msg(msg->call, tmp)) {
        if (!msg_is_request(tmp))
            continue;

        if (!strcmp(tmp->src, msg->dst) && tmp->sport == msg->dport)
            return tmp;
    }

    return NULL;
}

sip_msg_t *
msg_get_request_sdp(sip_msg_t *msg)
{
    sip_msg_t *tmp = msg;
    while (tmp) {
        if ((tmp = msg_get_request(tmp)) && tmp->sdp)
            return tmp;
    }

    return NULL;

}

char *
msg_get_header(sip_msg_t *msg, char *out)
{
    // Source and Destination address
    char from_addr[80], to_addr[80];

    // We dont use Message attributes here because it contains truncated data
    // This should not overload too much as all results should be already cached
    sprintf(from_addr, "%s:%u", sip_address_format(msg->src), msg->sport);
    sprintf(to_addr, "%s:%u", sip_address_format(msg->dst), msg->dport);

    // Get msg header
    sprintf(out, "%s %s %s -> %s", DATE(msg), TIME(msg), from_addr, to_addr);
    return out;
}

void
sip_calls_clear()
{
    // Remove first call until no first call exists
    pthread_mutex_lock(&calls.lock);
    while (calls.first) {
        sip_call_destroy(calls.first);
    }
    // Create again the callid hash table
    hdestroy();
    hcreate(calls.limit);

    pthread_mutex_unlock(&calls.lock);
}

const char *
sip_calculate_duration(const sip_msg_t *start, const sip_msg_t *end, char *dur)
{
    int seconds;
    char duration[20];
    // Differnce in secons
    seconds = end->pcap_header->ts.tv_sec - start->pcap_header->ts.tv_sec;
    // Set Human readable format
    sprintf(duration, "%d:%02d", seconds / 60, seconds % 60);
    sprintf(dur, "%7s", duration);
    return dur;
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

void
call_set_attribute(sip_call_t *call, enum sip_attr_id id, const char *fmt, ...)
{
    char value[512];

    // Get the actual value for the attribute
    va_list ap;
    va_start(ap, fmt);
    vsprintf(value, fmt, ap);
    va_end(ap);

    sip_attr_set(call->attrs, id, value);
}

const char *
call_get_attribute(sip_call_t *call, enum sip_attr_id id)
{
    if (!call)
        return NULL;

    switch (id) {
        case SIP_ATTR_CALLINDEX:
        case SIP_ATTR_MSGCNT:
        case SIP_ATTR_CALLSTATE:
        case SIP_ATTR_CONVDUR:
        case SIP_ATTR_TOTALDUR:
            return sip_attr_get(call->attrs, id);
        default:
            return msg_get_attribute(call_get_next_msg(call, NULL), id);
    }

    return NULL;
}

void
msg_set_attribute(sip_msg_t *msg, enum sip_attr_id id, const char *fmt, ...)
{
    char value[512];

    // Get the actual value for the attribute
    va_list ap;
    va_start(ap, fmt);
    vsprintf(value, fmt, ap);
    va_end(ap);

    sip_attr_set(msg->attrs, id, value);
}

const char *
msg_get_attribute(sip_msg_t *msg, enum sip_attr_id id)
{
    if (!msg)
        return NULL;

    return sip_attr_get(msg->attrs, id);
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

const char *
sip_address_format(const char *address)
{

    // Return address formatted depending on active settings
    if (is_option_enabled("sngrep.displayalias")) {
        return get_alias_value(address);
    } else if (is_option_enabled("capture.lookup") && is_option_enabled("sngrep.displayhost")) {
        return lookup_hostname(address);
    } else {
        return address;
    }
}

const char *
sip_address_port_format(const char *addrport)
{
    static char aport[50];
    char address[50];
    int port;

    strncpy(aport, addrport, 50);
    if (sscanf(aport, "%[^:]:%d", address, &port) == 2) {
        sprintf(aport, "%s:%d", sip_address_format(address), port);
    }

    return aport;
}
