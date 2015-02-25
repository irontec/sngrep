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
 * @todo This functions should be recoded. They parse the payload searching
 * for fields with sscanf and can fail easily.
 * We could use an external parser library (osip maybe?) but I prefer recoding
 * this to avoid more dependencies.
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
#include "sip.h"
#include "option.h"
#include "capture.h"

/**
 * @brief Linked list of parsed calls
 *
 * All parsed calls will be added to this list, only accesible from
 * this awesome structure, so, keep it thread-safe.
 */
static sip_call_list_t calls =
    {
      .first = NULL,
      .last = NULL,
      .count = 0 };

sip_msg_t *
sip_msg_create(const char *payload)
{
    sip_msg_t *msg;

    if (!(msg = malloc(sizeof(sip_msg_t))))
        return NULL;
    memset(msg, 0, sizeof(sip_msg_t));
    msg->attrs = NULL;
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
    // Initialize a new call structure
    sip_call_t *call = malloc(sizeof(sip_call_t));
    memset(call, 0, sizeof(sip_call_t));

    // Initialize call lock
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
    pthread_mutex_init(&call->lock, &attr);

    pthread_mutex_lock(&calls.lock);
    if (!calls.count) {
        calls.first = call;
    } else {
        call->prev = calls.last;
        calls.last->next = call;
    }
    calls.last = call;
    calls.count++;
    // Store current call Index
    call_set_attribute(call, SIP_ATTR_CALLINDEX, "%d", calls.count);
    pthread_mutex_unlock(&calls.lock);
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
    char *body = strdup(payload);
    char *pch, *callid = NULL;
    char value[256];

    for (pch = strtok(body, "\n"); pch; pch = strtok(NULL, "\n")) {
        if (!strncasecmp(pch, "Call-ID", 7)) {
            if (sscanf(pch, "Call-ID: %[^@\r\n]", value) == 1) {
                callid = strdup(value);
            }
        }
    }
    free(body);
    return callid;
}

sip_msg_t *
sip_load_message(struct timeval tv, struct in_addr src, u_short sport, struct in_addr dst,
                 u_short dport, u_char *payload)
{
    sip_msg_t *msg;
    sip_call_t *call;
    char *callid;
    char date[12], time[20];

    // Skip messages if capture is disabled
    if (!is_option_enabled("sip.capture")) {
        return NULL;
    }

    // Get the Call-ID of this message
    if (!(callid = sip_get_callid((const char*) payload))) {
        return NULL;
    }

    // Create a new message from this data
    if (!(msg = sip_msg_create((const char*) payload))) {
        return NULL;
    }

    // Parse the package payload to fill message attributes
    if (msg_parse_payload(msg, (const char*) payload) != 0) {
        sip_msg_destroy(msg);
        return NULL;
    }

    // Fill message data
    msg->ts = tv;
    msg->src = src;
    msg->sport = sport;
    msg->dst = dst;
    msg->dport = dport;

    // Set Source and Destination attributes
    msg_set_attribute(msg, SIP_ATTR_SRC, "%s:%u", inet_ntoa(src), htons(sport));
    msg_set_attribute(msg, SIP_ATTR_DST, "%s:%u", inet_ntoa(dst), htons(dport));

    // Set Source and Destination lookpued hosts
    if (is_option_enabled("capture.lookup")) {
        msg_set_attribute(msg, SIP_ATTR_SRC_HOST, "%.15s:%u", lookup_hostname(&src), htons(sport));
        msg_set_attribute(msg, SIP_ATTR_DST_HOST, lookup_hostname(&dst), htons(dport));
    }
    msg_set_attribute(msg, SIP_ATTR_SRC_HOST, "%s:%u", inet_ntoa(src), htons(sport));
    msg_set_attribute(msg, SIP_ATTR_DST_HOST, "%s:%u", inet_ntoa(dst), htons(dport));

    // Set message Date attribute
    time_t t = (time_t) msg->ts.tv_sec;
    struct tm *timestamp = localtime(&t);
    strftime(date, sizeof(date), "%Y/%m/%d", timestamp);
    msg_set_attribute(msg, SIP_ATTR_DATE, date);

    // Set message Time attribute
    strftime(time, sizeof(time), "%H:%M:%S", timestamp);
    sprintf(time + 8, ".%06d", (int) msg->ts.tv_usec);
    msg_set_attribute(msg, SIP_ATTR_TIME, time);

    // Find the call for this msg
    if (!(call = call_find_by_callid(callid))) {
        // Only create a new call if the first msg
        // is a request message in the following gorup
        if (get_option_int_value("sip.ignoreincomplete")) {
            // Get Message method / response code
            const char *method = msg_get_attribute(msg, SIP_ATTR_METHOD);
            if (method && strncasecmp(method, "INVITE", 6) && strncasecmp(method, "REGISTER", 8)
                    && strncasecmp(method, "SUBSCRIBE", 9) && strncasecmp(method, "OPTIONS", 7)
                    && strncasecmp(method, "PUBLISH", 7) && strncasecmp(method, "MESSAGE", 7)
                    && strncasecmp(method, "NOTIFY", 6)) {
                // Deallocate message memory
                sip_msg_destroy(msg);
                return NULL;
            }
        }

        // Create the call if not found
        if (!(call = sip_call_create(callid))) {
            // Deallocate message memory
            sip_msg_destroy(msg);
            return NULL;
        }
    }

    // Set message callid
    msg_set_attribute(msg, SIP_ATTR_CALLID, callid);
    // Dellocate callid memory
    free(callid);

    // Add the message to the found/created call
    call_add_message(call, msg);

    // Update Call State
    call_update_state(call);

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
    sip_msg_t *cur, *prev;

    pthread_mutex_lock(&call->lock);
    // Set the message owner
    msg->call = call;

    // XXX Put this msg at the end of the msg list
    // Order is important!!!
    if (!call->msgs) {
        call->msgs = msg;
    } else {
        for (cur = call->msgs; cur; prev = cur, cur = cur->next)
            ;
        prev->next = msg;
    }
    // Store message count
    call_set_attribute(call, SIP_ATTR_MSGCNT, "%d", call_msg_count(call));
    pthread_mutex_unlock(&call->lock);
}

sip_call_t *
call_find_by_callid(const char *callid)
{
    const char *cur_callid;
    pthread_mutex_lock(&calls.lock);
    sip_call_t *cur = calls.first;

    while (cur) {
        cur_callid = call_get_attribute(cur, SIP_ATTR_CALLID);
        if (cur_callid && !strcmp(cur_callid, callid)) {
            pthread_mutex_unlock(&calls.lock);
            return cur;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&calls.lock);
    return NULL;
}

sip_call_t *
call_find_by_xcallid(const char *xcallid)
{
    const char *cur_xcallid;

    pthread_mutex_lock(&calls.lock);
    sip_call_t *cur = calls.first;

    while (cur) {
        cur_xcallid = call_get_attribute(cur, SIP_ATTR_XCALLID);
        if (cur_xcallid && !strcmp(cur_xcallid, xcallid)) {
            pthread_mutex_unlock(&calls.lock);
            return cur;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&calls.lock);
    return NULL;
}

//@todo replace this for Msgcnt attribute
int
call_msg_count(sip_call_t *call)
{
    int msgcnt = 0;
    pthread_mutex_lock(&call->lock);
    sip_msg_t *msg = call->msgs;
    while (msg) {
        msgcnt++;
        msg = msg->next;
    }
    pthread_mutex_unlock(&call->lock);
    return msgcnt;
}

sip_call_t *
call_get_xcall(sip_call_t *call)
{
    if (call_get_attribute(call, SIP_ATTR_XCALLID)) {
        return call_find_by_callid(call_get_attribute(call, SIP_ATTR_XCALLID));
    } else {
        return call_find_by_xcallid(call_get_attribute(call, SIP_ATTR_CALLID));
    }
}

sip_msg_t *
call_get_next_msg(sip_call_t *call, sip_msg_t *msg)
{
    sip_msg_t *ret;
    pthread_mutex_lock(&call->lock);
    if (msg == NULL) {
        ret = call->msgs;
    } else {
        ret = msg->next;
    }
    pthread_mutex_unlock(&call->lock);
    return ret;
}

sip_msg_t *
call_get_prev_msg(sip_call_t *call, sip_msg_t *msg)
{
    sip_msg_t *ret = NULL, *cur;
    pthread_mutex_lock(&call->lock);
    if (msg == NULL) {
        // No message, no previous
        ret = NULL;
    } else {
        // Get previous message
        for (cur = call->msgs; cur; ret = cur, cur = cur->next) {
            // If cur is the message, ret will be the previous one
            if (cur == msg)
                break;
        }
    }
    pthread_mutex_unlock(&call->lock);
    return ret;
}

sip_call_t *
call_get_next(sip_call_t *cur)
{

    sip_call_t * next;
    if (!cur) {
        next = calls.first;
    } else {
        next = cur->next;
    }

    if (next && sip_check_call_ignore(next)) {
        return call_get_next(next);
    }
    return next;
}

sip_call_t *
call_get_prev(sip_call_t *cur)
{

    sip_call_t *prev;
    if (!cur) {
        prev = calls.first;
    } else {
        prev = cur->prev;
    }

    if (prev && sip_check_call_ignore(prev)) {
        return call_get_prev(prev);
    }
    return prev;
}

void
call_update_state(sip_call_t *call)
{
    sip_msg_t *msg;
    const char *callstate;
    const char *method;

    // Sanity check
    if (!call || !call->msgs)
        return;

    // Get Last call message
    msg = call->msgs;
    while (msg->next) {
        msg = msg->next;
    }

    // Get Message method / response code
    if (!(method = msg_get_attribute(msg, SIP_ATTR_METHOD))) {
        return;
    }

    // If this message is actually a call, get its current state
    if ((callstate = call_get_attribute(call, SIP_ATTR_CALLSTATE))) {
        if (!strcmp(callstate, "CALL SETUP")) {
            if (!strncasecmp(method, "200", 3)) {
                call_set_attribute(call, SIP_ATTR_CALLSTATE, "IN CALL");
            } else if (!strncasecmp(method, "CANCEL", 6)) {
                call_set_attribute(call, SIP_ATTR_CALLSTATE, "CANCELLED");
            } else if (*method == '4' || *method == '5' || *method == '6') {
                call_set_attribute(call, SIP_ATTR_CALLSTATE, "REJECTED");
            }
        } else if (!strcmp(callstate, "IN CALL")) {
            if (!strncasecmp(method, "BYE", 3)) {
                call_set_attribute(call, SIP_ATTR_CALLSTATE, "COMPLETED");
            }
        } else if (!strncasecmp(method, "INVITE", 6)) {
            call_set_attribute(call, SIP_ATTR_CALLSTATE, "CALL SETUP");
        }
    } else {
        // This is actually a call
        if (!strncasecmp(method, "INVITE", 6))
            call_set_attribute(call, SIP_ATTR_CALLSTATE, "CALL SETUP");
    }

}

int
msg_parse_payload(sip_msg_t *msg, const char *payload)
{
    char *body;
    char *pch;
    char value[256];
    char rest[256];

    // Sanity check
    if (!msg || !payload)
        return 1;

    // Duplicate payload to cut into lines
    body = strdup(payload);

    for (pch = strtok(body, "\n"); pch; pch = strtok(NULL, "\n")) {
        if (!strlen(pch))
            continue;

        if (sscanf(pch, "X-Call-ID: %[^@\t\n\r]", value) == 1) {
            msg_set_attribute(msg, SIP_ATTR_XCALLID, value);
            continue;
        }
        if (sscanf(pch, "X-CID: %[^@\t\n\r]", value) == 1) {
            msg_set_attribute(msg, SIP_ATTR_XCALLID, value);
            continue;
        }
        if (sscanf(pch, "SIP/2.0 %[^\t\n\r]", value)) {
            if (!msg_get_attribute(msg, SIP_ATTR_METHOD)) {
                msg_set_attribute(msg, SIP_ATTR_METHOD, value);
            }
            continue;
        }
        if (sscanf(pch, "CSeq: %s %[^\t\n\r]", rest, value)) {
            if (!msg_get_attribute(msg, SIP_ATTR_METHOD)) {
                msg_set_attribute(msg, SIP_ATTR_REQUEST, "1");
                msg_set_attribute(msg, SIP_ATTR_METHOD, value);
            }
            msg_set_attribute(msg, SIP_ATTR_CSEQ, rest);
            continue;
        }
        if (sscanf(pch, "From: %*[^:]:%[^@]@%[^\t\n\r]", value, rest) == 2) {
            msg_set_attribute(msg, SIP_ATTR_SIPFROMUSER, value);
        }
        if (sscanf(pch, "From: %[^:]:%[^\t\n\r>;]", rest, value)) {
            msg_set_attribute(msg, SIP_ATTR_SIPFROM, value);
            continue;
        }
        if (sscanf(pch, "To: %*[^:]:%[^@]@%[^\t\n\r]", value, rest) == 2) {
            msg_set_attribute(msg, SIP_ATTR_SIPTOUSER, value);
        }
        if (sscanf(pch, "To: %[^:]:%[^\t\n\r>;]", rest, value)) {
            msg_set_attribute(msg, SIP_ATTR_SIPTO, value);
            continue;
        }
        if (!strncasecmp(pch, "Content-Type: application/sdp", 29)) {
            msg_set_attribute(msg, SIP_ATTR_SDP, "1");
            continue;
        }
        if (sscanf(pch, "c=%*s %*s %s", value) == 1) {
            msg_set_attribute(msg, SIP_ATTR_SDP_ADDRESS, value);
            continue;
        }
        if (sscanf(pch, "m=%*s %s", value) == 1) {
            msg_set_attribute(msg, SIP_ATTR_SDP_PORT, value);
            continue;
        }
    }
    free(body);
    return 0;
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

char *
msg_get_header(sip_msg_t *msg, char *out)
{
    // Source and Destination address
    char from_addr[80], to_addr[80];

    // We dont use Message attributes here because it contains truncated data
    // This should not overload too much as all results should be already cached
    if (is_option_enabled("capture.lookup") && is_option_enabled("sngrep.displayhost")) {
        sprintf(from_addr, "%s:%u", lookup_hostname(&msg->src), htons(msg->sport));
        sprintf(to_addr, "%s:%u", lookup_hostname(&msg->dst), htons(msg->dport));
    } else {
        sprintf(from_addr, "%s:%u", inet_ntoa(msg->src), htons(msg->sport));
        sprintf(to_addr, "%s:%u", inet_ntoa(msg->dst), htons(msg->dport));
    }

    // Get msg header
    sprintf(out, "%s %s %s -> %s", DATE(msg), TIME(msg), from_addr, to_addr);
    return out;
}

void
sip_calls_clear()
{
    pthread_mutex_lock(&calls.lock);
    // Remove first call until no first call exists
    while (calls.first) {
        sip_call_destroy(calls.first);
    }

    pthread_mutex_unlock(&calls.lock);
}
