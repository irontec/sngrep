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
call_group_add(sip_call_group_t *group, sip_call_t *call)
{

    if (!group || !call || call_group_exists(group, call)) return;
    group->calls[group->callcnt++] = call;
}

void
call_group_del(sip_call_group_t *group, sip_call_t *call)
{
    int i;
    if (!group || !call || !call_group_exists(group, call)) return;
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
        if (call == group->calls[i]) return 1;
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

extern int
call_group_msg_count(sip_call_group_t *group)
{
    sip_msg_t *msg;
    int msgcnt = 0, i;

    for (i = 0; i < group->callcnt; i++) {
        while ((msg = call_get_next_msg(group->calls[i], msg))) {
            msgcnt++;
        }
    }
    return msgcnt;
}

sip_msg_t *
call_group_get_next_msg(sip_call_group_t *group, sip_msg_t *msg)
{
    sip_msg_t *next = NULL;
    sip_msg_t *cand;
    int i;

    for (i = 0; i < group->callcnt; i++) {
        cand = NULL;
        while ((cand = call_get_next_msg(group->calls[i], cand))) {
            // candidate must be between msg and next
            if (sip_msg_is_older(cand, msg) && (!next || !sip_msg_is_older(cand, next))) {
                next = cand;
                break;
            }
        }
    }
    return next;
}

int
sip_msg_is_older(sip_msg_t *one, sip_msg_t *two)
{
    // Yes, you are older than nothing
    if (!two) return 1;
    // Compare seconds
    if (one->ts.tv_sec > two->ts.tv_sec) return 1;
    // Compare useconds if seconds are equal
    if (one->ts.tv_sec == two->ts.tv_sec && one->ts.tv_usec > two->ts.tv_usec) return 1;
    // Otherwise
    return 0;
}
