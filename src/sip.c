/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
 **
 ** Copyright (C) 2013 Ivan Alonso (Kaian)
 ** Copyright (C) 2013 Irontec SL. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "sip.h"

/** 
 * @brief Linked list of parsed calls
 *
 * All parsed calls will be added to this list, only accesible from
 * this awesome pointer, so, keep it thread-safe.
 */
sip_call_t *calls = NULL;

/**
 * @brief Warranty thread-safe access to the calls list.
 *
 * This lock should be used whenever the calls pointer is updated, but
 * before single call locking, it will be used everytime a thread access
 * single call data.
 */
pthread_mutex_t calls_lock;

sip_msg_t *
sip_parse_message(const char *header, const char *payload)
{
    char *callid;
    struct sip_call *call;

    // Get SIP payload Call-ID
    if (!(callid = get_callid(payload))) return NULL;

    // Get call structure for that Call-ID
    if (!(call = call_find_by_callid(callid))) {
        // Create a new call for the message Call-ID
        if (!(call = call_new(callid))) {
            free(callid);
            return NULL;
        }
    }

    // Deallocate parsed callid
    free(callid);

    // Return the parsed structure
    return call_add_message(call, header, payload);
}

char *
get_callid(const char* payload)
{
    char buffer[512];
    char *pch, *callid = NULL;
    char *body = strdup(payload);

    // Parse payload line by line
    pch = strtok(body, "\n");
    while (pch) {
        // fix last ngrep line character
        if (pch[strlen(pch) - 1] == '.') pch[strlen(pch) - 1] = '\0';

        if (!strncasecmp(pch, "Call-ID: ", 9)) {
            if (sscanf(pch, "Call-ID: %[^@\n]", buffer)) {
                callid = malloc(strlen(buffer) + 1);
                strcpy(callid, buffer);
                break;
            }
        }
        pch = strtok(NULL, "\n");
    }
    free(body);
    return callid;
}

struct sip_call *
call_new(const char *callid)
{
    // Initialize a new call structure
    struct sip_call *call = malloc(sizeof(struct sip_call));
    memset(call, 0, sizeof(sip_call_t));
    call->callid = strdup(callid);

    // Initialize call lock
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&call->lock, &attr);

    struct sip_call *cur, *prev;
    if (!calls) {
        calls = call;
    } else {
        for (cur = calls; cur; prev = cur, cur = cur->next)
            ;
        prev->next = call;
        call->prev = prev;
    }
    return call;
}

struct sip_msg *
call_add_message(struct sip_call *call, const char *header, const char *payload)
{
    struct sip_msg *msg;

    if (!(msg = malloc(sizeof(struct sip_msg)))) return NULL;
    memset(msg, 0, sizeof(sip_msg_t));
    msg->call = call;
    msg->headerptr = strdup(header);
    msg->payloadptr = strdup(payload);

    pthread_mutex_lock(&call->lock);
    // XXX Put this msg at the end of the msg list
    // Order is important!!!
    struct sip_msg *cur, *prev;
    if (!call->messages) {
        call->messages = parse_msg(msg);
    } else {
        for (cur = call->messages; cur; prev = cur, cur = cur->next)
            ;
        prev->next = msg;
    }
    pthread_mutex_unlock(&call->lock);
    return msg;
}

int
msg_parse_header(struct sip_msg *msg, const char *header)
{
    struct tm when = {
            0 };
    char time[20];

    if (sscanf(header, "U %d/%d/%d %d:%d:%d.%d %s -> %s", &when.tm_year, &when.tm_mon,
            &when.tm_mday, &when.tm_hour, &when.tm_min, &when.tm_sec, (int*) &msg->ts.tv_usec,
            msg->ip_from, msg->ip_to)) {

        // Fix some time data
        when.tm_year -= 1900; // C99 Years since 1900
        when.tm_mon--; // C99 0-11
        msg->timet = mktime(&when);
        msg->ts.tv_sec = (long int) msg - msg->timet;

        // Convert to string
        strftime(time, 20, "%H:%M:%S", localtime(&msg->timet));
        sprintf(msg->time, "%s.%06d", time, (int) msg->ts.tv_usec);
        return 0;
    }
    return 1;
}

int
msg_parse_payload(struct sip_msg *msg, const char *payload)
{
    char *body = strdup(payload);
    char * pch;
    char rest[256];

    for (pch = strtok(body, "\n"); pch; pch = strtok(NULL, "\n")) {
        // fix last ngrep line character
        if (pch[strlen(pch) - 1] == '.') pch[strlen(pch) - 1] = '\0';

        // Copy the payload line by line (easier to process by the UI)
        msg->payload[msg->plines++] = strdup(pch);

        if (sscanf(pch, "X-Call-ID: %[^@\n]", msg->call->xcallid)) {
            continue;
        }
        if (sscanf(pch, "X-CID: %[^@\n]", msg->call->xcallid)) {
            continue;
        }
        if (sscanf(pch, "SIP/2.0 %[^\n]", msg->type)) {
            continue;
        }
        if (sscanf(pch, "CSeq: %d %[^\t\n\r]", &msg->cseq, rest)) {
            if (!strlen(msg->type)) strcpy(msg->type, rest);
            continue;
        }
        if (sscanf(pch, "From: %[^:]:%[^\t\n\r>;]", rest, msg->sip_from)) {
            continue;
        }
        if (sscanf(pch, "To: %[^:]:%[^\t\n\r>;]", rest, msg->sip_to)) {
            continue;
        }
        if (!strncasecmp(pch, "Content-Type: application/sdp", 31)) {
            strcat(msg->type, " (SDP)");
            continue;
        }
    }
    free(body);
    return 0;
}

struct sip_call *
call_find_by_callid(const char *callid)
{
    struct sip_call *cur = calls;
    // XXX LOCKING

    while (cur) {
        if (!strcmp(cur->callid, callid)) return cur;
        cur = cur->next;
    }
    return NULL;
}

struct sip_call *
call_find_by_xcallid(const char *xcallid)
{
    struct sip_call *cur = calls;
    // XXX LOCKING

    while (cur) {
        if (!strcmp(cur->xcallid, xcallid)) return cur;
        cur = cur->next;
    }
    return NULL;
}

int
get_n_calls()
{
    int callcnt = 0;
    struct sip_call *call = calls;
    while (call) {
        callcnt++;
        call = call->next;
    }
    return callcnt;
}

int
get_n_msgs(struct sip_call *call)
{
    int msgcnt = 0;
    pthread_mutex_lock(&call->lock);
    struct sip_msg *msg = call->messages;
    while (msg) {
        msgcnt++;
        msg = msg->next;
    }
    pthread_mutex_unlock(&call->lock);
    return msgcnt;
}

struct sip_call *
get_ex_call(struct sip_call *call)
{
    if (strlen(call->xcallid)) {
        return call_find_by_callid(call->xcallid);
    } else {
        return call_find_by_xcallid(call->callid);
    }
}

struct sip_msg *
get_next_msg(struct sip_call *call, struct sip_msg *msg)
{
    sip_msg_t *ret;
    pthread_mutex_lock(&call->lock);
    if (msg == NULL) {
        ret = call->messages;
    } else {
        ret = parse_msg(msg->next);
    }
    pthread_mutex_unlock(&call->lock);
    return ret;
}

struct sip_msg *
get_next_msg_ex(struct sip_call *call, struct sip_msg *msg)
{

    struct sip_msg *msg1 = NULL, *msg2 = NULL;
    struct sip_call *call2;

    pthread_mutex_lock(&call->lock);
    // Let's assume that call is always present, but call2 may not
    if (!(call2 = get_ex_call(call))) return get_next_msg(call, msg);

    if (!msg) {
        // No msg, compare the first one of both calls
        msg1 = get_next_msg(call, NULL);
        msg2 = get_next_msg(call2, NULL);
    } else if (msg->call == call) {
        // Message is from first call, get the next message in the call
        msg1 = get_next_msg(call, msg);
        // Get the chronological next message in second call
        while ((msg2 = get_next_msg(call2, msg2))) {
            // Compare with the actual message
            if (msg->ts.tv_sec < msg2->ts.tv_sec || (msg->ts.tv_sec == msg2->ts.tv_sec
                    && msg->ts.tv_usec < msg2->ts.tv_usec)) break;
        }
    } else if (msg->call == call2) {
        // Message is from second call, get the next message in the call
        msg1 = get_next_msg(call2, msg);
        // Get the chronological next message in first call
        while ((msg2 = get_next_msg(call, msg2))) {
            // Compare with the actual message
            if (msg->ts.tv_sec < msg2->ts.tv_sec || (msg->ts.tv_sec == msg2->ts.tv_sec
                    && msg->ts.tv_usec < msg2->ts.tv_usec)) break;
        }
    }
    pthread_mutex_unlock(&call->lock);

    if ((!msg2) || (msg1 && msg1->ts.tv_sec < msg2->ts.tv_sec) || (msg1 && msg1->ts.tv_sec
            == msg2->ts.tv_sec && msg1->ts.tv_usec < msg2->ts.tv_usec)) {
        return msg1;
    } else {
        return msg2;
    }
}

sip_msg_t *
parse_msg(sip_msg_t *msg)
{

    // Nothing to parse
    if (!msg) return NULL;

    // Message already parsed
    if (msg->parsed) return msg;

    // Parse message header
    if (msg_parse_header(msg, msg->headerptr) != 0) return NULL;

    // Parse message payload
    if (msg_parse_payload(msg, msg->payloadptr) != 0) return NULL;

    // Free message pointers
    free(msg->headerptr);
    free(msg->payloadptr);

    // Mark as parsed
    msg->parsed = 1;
    // Return the parsed message
    return msg;
}
