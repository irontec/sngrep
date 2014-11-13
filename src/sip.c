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
 * this awesome pointer, so, keep it thread-safe.
 */
static sip_call_t *calls = NULL;

/**
 * @brief Warranty thread-safe access to the calls list.
 *
 * This lock should be used whenever the calls pointer is updated, but
 * before single call locking, it will be used everytime a thread access
 * single call data.
 */
static pthread_mutex_t calls_lock;

static sip_attr_hdr_t attrs[] =
    {
        { .id = SIP_ATTR_SIPFROM, .name = "sipfrom", .desc = "SIP From" },
        { .id = SIP_ATTR_SIPTO, .name = "sipto", .desc = "SIP To" },
        { .id = SIP_ATTR_SRC, .name = "src", .desc = "Source" },
        { .id = SIP_ATTR_SRC_HOST, .name = "srchost", .desc = "Source" },
        { .id = SIP_ATTR_DST, .name = "dst", .desc = "Destiny" },
        { .id = SIP_ATTR_DST_HOST, .name = "dsthost", .desc = "Destiny" },
        { .id = SIP_ATTR_CALLID, .name = "callid", .desc = "Call-ID" },
        { .id = SIP_ATTR_XCALLID, .name = "xcallid", .desc = "X-Call-ID" },
        { .id = SIP_ATTR_DATE, .name = "date", .desc = "Date" },
        { .id = SIP_ATTR_TIME, .name = "time", .desc = "Time" },
        { .id = SIP_ATTR_METHOD, .name = "method", .desc = "Method" },
        { .id = SIP_ATTR_REQUEST, .name = "request", .desc = "Request" },
        { .id = SIP_ATTR_CSEQ, .name = "CSeq", .desc = "CSeq" },
        { .id = SIP_ATTR_SDP, .name = "sdp", .desc = "Has SDP" },
        { .id = SIP_ATTR_SDP_ADDRESS, .name = "sdpaddress", .desc = "SDP Address" },
        { .id = SIP_ATTR_SDP_PORT, .name = "sdpport", .desc = "SDP Port" },
        { .id = SIP_ATTR_TRANSPORT, .name = "transport", .desc = "Trans" },
        { .id = SIP_ATTR_STARTING, .name = "starting", .desc = "Starting" },
        { .id = SIP_ATTR_MSGCNT, .name = "msgcnt", .desc = "Msgs" }, };

sip_msg_t *
sip_msg_create(const char *payload)
{
    sip_msg_t *msg;

    if (!(msg = malloc(sizeof(sip_msg_t))))
        return NULL;
    memset(msg, 0, sizeof(sip_msg_t));
    msg->attrs = NULL;
    msg->payloadptr = strdup(payload);
    msg->parsed = 0;
    msg->color = -1;
    return msg;
}

void
sip_msg_destroy(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;
    int i;

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
    if (msg->payloadptr)
        free(msg->payloadptr);

    // Free message payload
    for (i = 0; i < msg->plines; i++)
        free(msg->payload[i]);

    // Free packet data
    free(msg->pcap_header);
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
    call->attrs = NULL;
    call->color = -1;

    // Initialize call lock
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
    pthread_mutex_init(&call->lock, &attr);

    //@todo Add the call to the end of the list.
    //@todo This should be improved
    pthread_mutex_lock(&calls_lock);
    sip_call_t *cur, *prev;
    if (!calls) {
        calls = call;
    } else {
        for (cur = calls; cur; prev = cur, cur = cur->next)
            ;
        prev->next = call;
        call->prev = prev;
    }
    pthread_mutex_unlock(&calls_lock);
    return call;
}

void
sip_call_destroy(sip_call_t *call)
{
    // No call to destroy
    if (!call)
        return;

    // Remove all messages 
    while (call->msgs)
        sip_msg_destroy(call->msgs);

    // Remove all call attributes
    sip_attr_list_destroy(call->attrs);

    // Free it!
    free(call);
    call = NULL;
}

char *
sip_get_callid(const char* payload)
{
    char *body = strdup(payload);
    char *pch, *callid = NULL;
    char value[256];

    for (pch = strtok(body, "\n"); pch; pch = strtok(NULL, "\n")) {
        if (!strncasecmp(pch, "Call-ID", 7)) {
            if (sscanf(pch, "Call-ID: %[^@\n]", value) == 1) {
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
    char date[12], time[20], from_addr[80], to_addr[80];

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

    // Fill message data
    msg->ts = tv;
    msg->src = src;
    msg->sport = sport;
    msg->dst = dst;
    msg->dport = dport;

    // Set Source and Destiny attributes
    sprintf(from_addr, "%s:%u", inet_ntoa(src), htons(sport));
    msg_set_attribute(msg, SIP_ATTR_SRC, from_addr);
    sprintf(to_addr, "%s:%u", inet_ntoa(dst), htons(dport));
    msg_set_attribute(msg, SIP_ATTR_DST, to_addr);

    // Set Source and Destiny lookpued hosts
    if (is_option_enabled("capture.lookup")) {
        sprintf(from_addr, "%.15s:%u", lookup_hostname(&src), htons(sport));
        sprintf(to_addr, "%.15s:%u", lookup_hostname(&dst), htons(dport));
    }
    msg_set_attribute(msg, SIP_ATTR_SRC_HOST, from_addr);
    msg_set_attribute(msg, SIP_ATTR_DST_HOST, to_addr);

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
            const char *method = msg_get_attribute(msg_parse(msg), SIP_ATTR_METHOD);
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

    // Return the loaded message
    return msg;
}

int
sip_calls_count()
{
    int callcnt = 0;
    pthread_mutex_lock(&calls_lock);
    sip_call_t *call = calls;
    while (call) {
        if (!sip_check_call_ignore(call))
            callcnt++;
        call = call->next;
    }
    pthread_mutex_unlock(&calls_lock);
    return callcnt;
}

int
sip_check_call_ignore(sip_call_t *call)
{
    int i;
    char filter_option[80];
    const char *filter;

    // Check if an ignore option exists
    for (i = 0; i < sizeof(attrs) / sizeof(*attrs); i++) {
        if (is_ignored_value(attrs[i].name, call_get_attribute(call, attrs[i].id))) {
            return 1;
        }
    }

    // Check enabled filters
    if (is_option_enabled("filter.enable")) {
        if ((filter = get_option_value("filter.sipfrom")) && strlen(filter)) {
            if (strstr(call_get_attribute(call, SIP_ATTR_SIPFROM), filter) == NULL) {
                return 1;
            }
        }
        if ((filter = get_option_value("filter.sipto")) && strlen(filter)) {
            if (strstr(call_get_attribute(call, SIP_ATTR_SIPTO), filter) == NULL) {
                return 1;
            }
        }
        if ((filter = get_option_value("filter.src")) && strlen(filter)) {
            if (strncasecmp(filter, call_get_attribute(call, SIP_ATTR_SRC), strlen(filter))) {
                return 1;
            }
        }
        if ((filter = get_option_value("filter.dst")) && strlen(filter)) {
            if (strncasecmp(filter, call_get_attribute(call, SIP_ATTR_DST), strlen(filter))) {
                return 1;
            }
        }

        // Check if a filter option exists
        memset(filter_option, 0, sizeof(filter_option));
        sprintf(filter_option, "filter.%s", call_get_attribute(call, SIP_ATTR_STARTING));
        if (!is_option_enabled(filter_option)) {
            return 1;
        }
    }
    return 0;
}

sip_attr_hdr_t *
sip_attr_get_header(enum sip_attr_id id)
{
    int i;
    for (i = 0; i < sizeof(attrs) / sizeof(*attrs); i++) {
        if (id == attrs[i].id) {
            return &attrs[i];
        }
    }
    return NULL;
}

const char *
sip_attr_get_description(enum sip_attr_id id)
{
    sip_attr_hdr_t *header;
    if ((header = sip_attr_get_header(id))) {
        return header->desc;
    }
    return NULL;
}

const char *
sip_attr_get_name(enum sip_attr_id id)
{
    sip_attr_hdr_t *header;
    if ((header = sip_attr_get_header(id))) {
        return header->name;
    }
    return NULL;
}

enum sip_attr_id
sip_attr_from_name(const char *name)
{
    int i;
    for (i = 0; i < sizeof(attrs) / sizeof(*attrs); i++) {
        if (!strcasecmp(name, attrs[i].name)) {
            return attrs[i].id;
        }
    }
    return 0;
}

void
sip_attr_list_destroy(sip_attr_t *list)
{
    sip_attr_t *attr;

    // If attribute already exists change its value
    while (list) {
        attr = list;
        list = attr->next;
        // Free attribute value
        free(attr->value);
        // Free attribute structure
        free(attr);
    }
}

void
sip_attr_set(sip_attr_t **list, enum sip_attr_id id, const char *value)
{
    sip_attr_t *attr;

    // If attribute already exists change its value
    for (attr = *list; attr; attr = attr->next) {
        if (id == attr->hdr->id) {
            // Free previous value
            free(attr->value);
            // Store the new value
            attr->value = strdup(value);
            return;
        }
    }

    // Otherwise add a new attribute
    if (!(attr = malloc(sizeof(sip_attr_t))))
        return;

    attr->hdr = sip_attr_get_header(id);
    attr->value = strdup(value);
    attr->next = *list;
    *list = attr;
}

const char *
sip_attr_get(sip_attr_t *list, enum sip_attr_id id)
{
    sip_attr_t *attr;
    for (attr = list; attr; attr = attr->next) {
        if (id == attr->hdr->id) {
            return attr->value;
        }
    }
    return NULL;
}

void
call_add_message(sip_call_t *call, sip_msg_t *msg)
{
    sip_msg_t *cur, *prev;
    char msgcnt[5];

    pthread_mutex_lock(&call->lock);
    // Set the message owner
    msg->call = call;

    // XXX Put this msg at the end of the msg list
    // Order is important!!!
    if (!call->msgs) {
        call->msgs = msg_parse(msg);
    } else {
        for (cur = call->msgs; cur; prev = cur, cur = cur->next)
            ;
        prev->next = msg;
    }
    // Store message count
    sprintf(msgcnt, "%d", call_msg_count(call));
    call_set_attribute(call, SIP_ATTR_MSGCNT, msgcnt);
    pthread_mutex_unlock(&call->lock);
}

sip_call_t *
call_find_by_callid(const char *callid)
{
    const char *cur_callid;
    pthread_mutex_lock(&calls_lock);
    sip_call_t *cur = calls;

    while (cur) {
        cur_callid = call_get_attribute(cur, SIP_ATTR_CALLID);
        if (cur_callid && !strcmp(cur_callid, callid)) {
            pthread_mutex_unlock(&calls_lock);
            return cur;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&calls_lock);
    return NULL;
}

sip_call_t *
call_find_by_xcallid(const char *xcallid)
{
    const char *cur_xcallid;

    pthread_mutex_lock(&calls_lock);
    sip_call_t *cur = calls;

    while (cur) {
        cur_xcallid = call_get_attribute(cur, SIP_ATTR_XCALLID);
        if (cur_xcallid && !strcmp(cur_xcallid, xcallid)) {
            pthread_mutex_unlock(&calls_lock);
            return cur;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&calls_lock);
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
        ret = msg_parse(msg->next);
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
    return msg_parse(ret);
}

sip_call_t *
call_get_next(sip_call_t *cur)
{

    sip_call_t * next;
    if (!cur) {
        next = calls;
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
        prev = calls;
    } else {
        prev = cur->prev;
    }

    if (prev && sip_check_call_ignore(prev)) {
        return call_get_prev(prev);
    }
    return prev;
}

void
call_set_attribute(sip_call_t *call, enum sip_attr_id id, const char *value)
{
    sip_attr_set(&call->attrs, id, value);
}

const char *
call_get_attribute(sip_call_t *call, enum sip_attr_id id)
{
    if (id == SIP_ATTR_MSGCNT) {
        return sip_attr_get(call->attrs, id);
    }
    if (id == SIP_ATTR_STARTING) {
        return msg_get_attribute(call_get_next_msg(call, NULL), SIP_ATTR_METHOD);
    }
    return msg_get_attribute(call_get_next_msg(call, NULL), id);
}

sip_msg_t *
msg_parse(sip_msg_t *msg)
{

    // Nothing to parse
    if (!msg)
        return NULL;

    // Message already parsed
    if (msg->parsed)
        return msg;

    // Parse message payload
    if (msg_parse_payload(msg, msg->payloadptr) != 0)
        return NULL;

    // Free message pointers
    //free(msg->payloadptr);
    //msg->payloadptr = NULL;

    // Mark as parsed
    msg->parsed = 1;
    // Return the parsed message
    return msg;
}

int
msg_parse_header(sip_msg_t *msg, const char *header)
{
    struct tm when =
        { 0 };
    char time[20], ipfrom[22], ipto[22];
    time_t timet;
    char proto;

    // Sanity check
    if (!msg || !header)
        return 1;

    if (sscanf(header, "%c %d/%d/%d %d:%d:%d.%d %s -> %s", &proto, &when.tm_year, &when.tm_mon,
               &when.tm_mday, &when.tm_hour, &when.tm_min, &when.tm_sec, (int*) &msg->ts.tv_usec,
               ipfrom, ipto)) {

        // Fix some time data
        when.tm_isdst = 1; // Daylight saving time flag
        when.tm_year -= 1900; // C99 Years since 1900
        when.tm_mon--; // C99 0-11
        timet = mktime(&when);
        msg->ts.tv_sec = (long int) timet;

        // Convert to string
        strftime(time, 20, "%H:%M:%S", localtime(&timet));
        sprintf(time + strlen(time), ".%06d", (int) msg->ts.tv_usec);

        msg_set_attribute(msg, SIP_ATTR_TIME, time);
        msg_set_attribute(msg, SIP_ATTR_SRC, ipfrom);
        msg_set_attribute(msg, SIP_ATTR_DST, ipto);

        return 0;
    }
    return 1;
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
        // Copy the payload line by line (easier to process by the UI)
        msg->payload[msg->plines++] = strdup(pch);

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
                // ACK Messages are not considered requests
                if (strcasecmp(value, "ACK"))
                    msg_set_attribute(msg, SIP_ATTR_REQUEST, "1");
                msg_set_attribute(msg, SIP_ATTR_METHOD, value);
            }
            msg_set_attribute(msg, SIP_ATTR_CSEQ, rest);
            continue;
        }
        if (sscanf(pch, "From: %[^:]:%[^\t\n\r>;]", rest, value)) {
            msg_set_attribute(msg, SIP_ATTR_SIPFROM, value);
            continue;
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

void
msg_set_attribute(sip_msg_t *msg, enum sip_attr_id id, const char *value)
{
    sip_attr_set(&msg->attrs, id, value);
}

const char *
msg_get_attribute(sip_msg_t *msg, enum sip_attr_id id)
{
    if (!msg)
        return NULL;

    return sip_attr_get(msg->attrs, id);
}

int
msg_is_retrans(sip_msg_t *msg)
{
    sip_msg_t *prev = NULL;
    int i;

    // Sanity check
    if (!msg || !msg->call)
        return 0;

    // Get previous message
    prev = call_get_prev_msg(msg->call, msg);

    // No previous message, this can not be a retransmission
    if (!prev)
        return 0;

    // Not even the same lines in playload
    if (msg->plines != prev->plines)
        return 0;

    // Check if they have the same payload
    for (i = 0; i < msg->plines; i++) {
        // If any line of payload is different, this is not a retrans
        if (strcasecmp(msg->payload[i], prev->payload[i])) {
            return 0;
        }
    }

    // All check passed, this package is equal to its previous
    return 1;
}

char *
msg_get_header(sip_msg_t *msg, char *out)
{
    // Source and Destiny address
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
    sprintf(out,  "%s %s %s -> %s", DATE(msg), TIME(msg), from_addr, to_addr);
    return out;
}

void
sip_calls_clear()
{
    sip_call_t *call = NULL;
    pthread_mutex_lock(&calls_lock);

    // Remove first call until no first call exists
    for (call = calls; call; call = call->next)
        sip_call_destroy(call);

    // Initialize calls list header
    calls = NULL;

    pthread_mutex_unlock(&calls_lock);
}
