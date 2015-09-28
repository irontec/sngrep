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

#include "config.h"
#include "sip.h"

//! Shorter declaration of sip_call_group structure
typedef struct sip_call_group sip_call_group_t;

/**
 * @brief Contains a list of calls
 *
 * This structure is used for displaying more than one dialog in the
 * same call flow. Instead of displaying a call flow, we will display
 * a calls group flow.
 */
struct sip_call_group {
    //! Calls array in the group
    vector_t *calls;
    //! Color of the last printed call in mode Color-by-Call
    int color;
    //! Only consider SDP messages from Calls
    int sdp_only;
};

/**
 * @brief Create a new groupt to hold Calls in it
 *
 * Allocate memory to create a new calls group
 *
 * @return Pointer to a new group
 */
sip_call_group_t *
call_group_create();

/**
 * @brief Deallocate memory of an existing group
 *
 * @param Pointer to an existing group
 */
void
call_group_destroy(sip_call_group_t *group);

/**
 * @brief Clone an existing call group
 *
 * Create a new call group with the same calls of the
 * original one. Beware: The call pointers are shared between
 * original and clone groups.
 *
 */
sip_call_group_t *
call_group_clone(sip_call_group_t *original);

/**
 * @brief Add a Call to the group
 *
 * @param group Pointer to an existing group
 * @param call Pointer to an existing call
 */
void
call_group_add(sip_call_group_t *group, sip_call_t *call);

/**
 * @brief Remove a call from the group
 *
 * @param group Pointer to an existing group
 * @param call Pointer to an existing call
 */
void
call_group_del(sip_call_group_t *group, sip_call_t *call);

/**
 * @brief Check if a call is in the group
 *
 * @param group Pointer to an existing group
 * @param call Pointer to an existing call
 * @return 1 if the call is in the group, 0 otherwise
 */
int
call_group_exists(sip_call_group_t *group, sip_call_t *call);

/**
 * @brief Return the color pair number of a call
 *
 * When color by callid mode is enabled, this function will
 * return the color pair number of the call depending its
 * position inside the call
 *
 * @param group Pointer to an existing group
 * @param call Pointer to an existing call
 * @return Color pair number
 */
int
call_group_color(sip_call_group_t *group, sip_call_t *call);

/**
 * @brief Return the next call in the group
 *
 * Return then next call after the given call parameter.
 * If NULL is used as parameter, return the first call.
 * It will return NULL if last call is given as parameter.
 *
 * @param group Pointer to an existing group
 * @param call Pointer to an existing call or NULL
 * @return Next call of the group, or NULL
 */
sip_call_t *
call_group_get_next(sip_call_group_t *group, sip_call_t *call);

/**
 * @brief Return number of calls in a group
 *
 * @param group Pointer to an existing group
 * @return How many calls the group has
 */
int
call_group_count(sip_call_group_t *group);

/**
 * @brief Return message count in the group
 *
 * Return the sum of messages of all calls in the group
 *
 * @param group Pointer to an existing group
 * @return How many messages have the calls in the group
 */
int
call_group_msg_count(sip_call_group_t *group);

/**
 * @brief Return Message position in the group
 *
 * Return how many messages are before the given message
 * sorting all messages in all group calls by timestamp
 *
 * @param group Pointer to an existing group
 * @param msg A sip message from a call in the group
 * @return The position of given message in the group
 */
int
call_group_msg_number(sip_call_group_t *group, sip_msg_t *msg);

/**
 * @brief Finds the next msg in a call group.
 *
 * If the passed msg is NULL it returns the first message
 * of the group.
 *
 * @param callgroup SIP call group structure
 * @param msg Actual SIP msg from any call of the group (can be NULL)
 * @return Next chronological message in the group or NULL
 */
sip_msg_t *
call_group_get_next_msg(sip_call_group_t *group, sip_msg_t *msg);

/**
 * @brief Find the previous message in a call group
 *
 * @param callgroup SIP call group structure
 * @param msg Actual SIP msg from any call of the group
 * @return Previous chronological message in the group or NULL
 */
sip_msg_t *
call_group_get_prev_msg(sip_call_group_t *group, sip_msg_t *msg);

rtp_stream_t *
call_group_get_next_stream(sip_call_group_t *group, rtp_stream_t *stream);

int
timeval_is_older(struct timeval t1, struct timeval t2);

/**
 * @brief Check if a message is older than other
 *
 * @todo Move this to sip.h
 *
 * @param one SIP message pointer
 * @param two SIP message pointer
 * @return 0 if the messages has the same timestamp
 * @return 1 if one is older than two
 * @return 2 if two is older than one
 */
int
sip_msg_is_older(sip_msg_t *one, sip_msg_t *two);

int
rtp_stream_is_older(rtp_stream_t *one, rtp_stream_t *two);

#endif /* __SNGREP_GROUP_H_ */
