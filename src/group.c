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
 * @file group.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in group.h
 *
 */
#include <string.h>
#include <stdlib.h>
#include "group.h"

sip_call_group_t *
call_group_create()
{
    sip_call_group_t *group;
    if (!(group = malloc(sizeof(sip_call_group_t)))) {
        return NULL;
    }
    memset(group, 0, sizeof(sip_call_group_t));
    return group;
}

void
call_group_destroy(sip_call_group_t *group)
{
    free(group);
}

void
call_group_add(sip_call_group_t *group, sip_call_t *call)
{

    if (!group || !call || call_group_exists(group, call))
        return;
    group->calls[group->callcnt++] = call;
}

void
call_group_del(sip_call_group_t *group, sip_call_t *call)
{
    int i;
    if (!group || !call || !call_group_exists(group, call))
        return;
    for (i = 0; i < group->callcnt; i++) {
        if (call == group->calls[i]) {
            group->calls[i] = group->calls[i + 1];
            call = group->calls[i + 1];
        }
    }
    group->callcnt--;
}

int
call_group_exists(sip_call_group_t *group, sip_call_t *call)
{
    int i;
    for (i = 0; i < group->callcnt; i++) {
        if (call == group->calls[i])
            return 1;
    }
    return 0;
}

int
call_group_color(sip_call_group_t *group, sip_call_t *call)
{
    int i;
    for (i = 0; i < group->callcnt; i++) {
        if (call == group->calls[i]) {
            return (i % 7) + 1;
        }
    }
    return -1;
}

sip_call_t *
call_group_get_next(sip_call_group_t *group, sip_call_t *call)
{
    sip_msg_t *next, *first;
    int i;

    if (!group)
        return NULL;

    // Get call of the first message in group
    if (!call) {
        if ((next = call_group_get_next_msg(group, NULL))) {
            return next->call;
        }
        return NULL;
    }

    // Initialize candidate
    next = NULL;

    // Get the call with the next chronological message
    for (i = 0; i < group->callcnt; i++) {
        if (group->calls[i] == call)
            continue;

        // Get first message
        first = call_get_next_msg(group->calls[i], NULL);

        // Is first message of this call older?
        if (sip_msg_is_older(first, call->msgs) && (!next || !sip_msg_is_older(first, next))) {
            next = first;
            break;
        }
    }

    return (next) ? next->call : NULL;
}

int
call_group_count(sip_call_group_t *group)
{
    return group->callcnt;
}

int
call_group_msg_count(sip_call_group_t *group)
{
    sip_msg_t *msg = NULL;
    int msgcnt = 0, i;

    for (i = 0; i < group->callcnt; i++) {
        if (group->sdp_only) {
            while ((msg = call_get_next_msg(group->calls[i], msg))) {
                if (!msg->sdp)
                    continue;
                msgcnt++;
            }
        } else {
            msgcnt += call_msg_count(group->calls[i]);
        }
    }
    return msgcnt;
}

int
call_group_msg_number(sip_call_group_t *group, sip_msg_t *msg)
{
    int number = 0;
    sip_msg_t *cur = NULL;
    while ((cur = call_group_get_next_msg(group, cur))) {
        if (group->sdp_only && !msg->sdp)
            continue;

        if (cur == msg)
            return number;
        number++;
    }
    return 0;
}

sip_msg_t *
call_group_get_next_msg(sip_call_group_t *group, sip_msg_t *msg)
{
    sip_msg_t *next = NULL;
    sip_msg_t *cand;
    int i;
    // FIXME Performance hack for huge dialogs
    if (group->callcnt == 1) {
        cand = msg;
        while ((cand = call_get_next_msg(group->calls[0], cand))) {
            if (group->sdp_only && !cand->sdp)
                continue;
            break;
        }
        return cand;
    }

    for (i = 0; i < group->callcnt; i++) {
        cand = NULL;
        while ((cand = call_get_next_msg(group->calls[i], cand))) {
            if (group->sdp_only && !cand->sdp)
                continue;

            // candidate must be between msg and next
            if (sip_msg_is_older(cand, msg) && (!next || !sip_msg_is_older(cand, next))) {
                next = cand;
            }
        }
    }

    return next;
}

sip_msg_t *
call_group_get_prev_msg(sip_call_group_t *group, sip_msg_t *msg)
{
    sip_msg_t *next = NULL;
    sip_msg_t *prev = NULL;

    // FIXME Horrible performance for huge dialogs
    while ((next = call_group_get_next_msg(group, next))) {
        if (next == msg)
            break;
        prev = next;
    }

    return prev;
}

rtp_stream_t *
call_group_get_next_stream(sip_call_group_t *group, rtp_stream_t *stream)
{
    rtp_stream_t *next = NULL;
    rtp_stream_t *cand;
    int i;

    for (i = 0; i < group->callcnt; i++) {
        for (cand = group->calls[i]->streams; cand; cand = cand->next) {
            if (!stream_get_count(cand))
                continue;

            // candidate must be between msg and next
            if (rtp_stream_is_older(cand, stream) && (!next || rtp_stream_is_older(next, cand))) {
                next = cand;
            }
        }
    }

    return next;
}

int
timeval_is_older(struct timeval t1, struct timeval t2)
{
    long diff;
    diff = t2.tv_sec  * 1000000 + t2.tv_usec;
    diff -= t1.tv_sec * 1000000 + t1.tv_usec;
    return (diff < 0);
}

int
sip_msg_is_older(sip_msg_t *one, sip_msg_t *two)
{
    // Yes, you are older than nothing
    if (!two)
        return 1;

    // Otherwise
    return timeval_is_older(one->pcap_header->ts, two->pcap_header->ts);
}

int
rtp_stream_is_older(rtp_stream_t *one, rtp_stream_t *two)
{
    // Yes, you are older than nothing
    if (!two)
        return 1;

    // Otherwise
    return timeval_is_older(one->time, two->time);
}

