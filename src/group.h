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
 * @file option.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage call groups
 *
 * Call groups are used to pass a set of calls between different panels of
 * sngrep.
 *
 */

#ifndef __SNGREP_GROUP_H_
#define __SNGREP_GROUP_H_

#include "sip.h"

//! Shorter declaration of sip_call_group structure
typedef struct sip_call_group sip_call_group_t;

/**
 * @brief Contains a list of calls
 *
 * This structure is used for displaying more than one dialog in the
 * same call flow. Instead of displaying a call flow, we will display
 * a calls group flow.
 *
 * @note Trying to merge extended and normal callflow into a unique
 *       panel and allowing multiple dialog in a callflow.
 *
 * @fixme Remove 1024 "limitation"
 */
struct sip_call_group
{
    sip_call_t *calls[1024];
    int callcnt;
    int color;
};

extern sip_call_group_t *
call_group_create();

extern void
call_group_add(sip_call_group_t *group, sip_call_t *call);

extern void
call_group_del(sip_call_group_t *group, sip_call_t *call);

extern int
call_group_exists(sip_call_group_t *group, sip_call_t *call);

extern int
call_group_color(sip_call_group_t *group, sip_call_t *call);

extern int
call_group_msg_count(sip_call_group_t *group);

extern int
call_group_msg_number(sip_call_group_t *group, sip_msg_t *msg);

/**
 * @brief Finds the next msg in a call group.
 *
 * If the passed msg is NULL it returns the first message
 * of the group.
 *
 * @param callgroup SIP call group structure
 * @param msg Actual SIP msg from any call of the group (can be NULL)
 * @return Next chronological message in the group
 */
extern sip_msg_t *
call_group_get_next_msg(sip_call_group_t *group, sip_msg_t *msg);

extern int
sip_msg_is_older(sip_msg_t *one, sip_msg_t *two);

#endif /* __SNGREP_GROUP_H_ */
