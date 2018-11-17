/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
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

#include <glib.h>
#include "storage.h"

//! Shorter declaration of sip_call_group structure
typedef struct _SipCallGroup SipCallGroup;

/**
 * @brief Contains a list of calls
 *
 * This structure is used for displaying more than one dialog in the
 * same call flow. Instead of displaying a call flow, we will display
 * a calls group flow.
 */
struct _SipCallGroup {
    //! For extended display, main call-id
    const gchar *callid;
    //! Calls array in the group
    GPtrArray *calls;
    //! Messages in the group
    GPtrArray *msgs;
    //! Streams in the group
    GPtrArray *streams;
    //! Color of the last printed call in mode Color-by-Call
    gint color;
    //! Only consider SDP messages from Calls
    gint sdp_only;
};

/**
 * @brief Create a new groupt to hold Calls in it
 *
 * Allocate memory to create a new calls group
 *
 * @return Pointer to a new group
 */
SipCallGroup *
call_group_new();

/**
 * @brief Deallocate memory of an existing group
 *
 * @param Pointer to an existing group
 */
void
call_group_free(SipCallGroup *group);

/**
 * @brief Check if any of the calls of the group has changed
 *
 * This checks all the group flags to check if any of the call has
 * changed.
 *
 * @param ggroup Call group structure pointer
 * @return true if any of the calls of the group has changed, false otherwise
 */
gboolean
call_group_changed(SipCallGroup *group);

/**
 * @brief Clone an existing call group
 *
 * Create a new call group with the same calls of the
 * original one. Beware: The call pointers are shared between
 * original and clone groups.
 *
 */
SipCallGroup *
call_group_clone(SipCallGroup *original);

/**
 * @brief Add a Call to the group
 *
 * @param group Pointer to an existing group
 * @param call Pointer to an existing call
 */
void
call_group_add(SipCallGroup *group, SipCall *call);

/**
 * @brief Add several Calls to the group
 *
 * @param group Pointer to an existing group
 * @param calls Pointer to a vector with calls
 */
void
call_group_add_calls(SipCallGroup *group, GPtrArray *calls);

/**
 * @brief Remove a call from the group
 *
 * @param group Pointer to an existing group
 * @param call Pointer to an existing call
 */
void
call_group_remove(SipCallGroup *group, SipCall *call);

/**
 * @brief Remove all calls from the group
 *
 * @param group
 */
void
call_group_remove_all(SipCallGroup *group);

/**
 * @brief Check if a call is in the group
 *
 * @param group Pointer to an existing group
 * @param call Pointer to an existing call
 * @return TRUE if the call is in the group, FALSE otherwise
 */
gboolean
call_group_exists(SipCallGroup *group, SipCall *call);

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
gint
call_group_color(SipCallGroup *group, SipCall *call);

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
SipCall *
call_group_get_next(SipCallGroup *group, SipCall *call);

/**
 * @brief Return number of calls in a group
 *
 * @param group Pointer to an existing group
 * @return How many calls the group has
 */
gint
call_group_count(SipCallGroup *group);

/**
 * @brief Return message count in the group
 *
 * Return the sum of messages of all calls in the group
 *
 * @param group Pointer to an existing group
 * @return How many messages have the calls in the group
 */
gint
call_group_msg_count(SipCallGroup *group);

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
SipMsg *
call_group_get_next_msg(SipCallGroup *group, SipMsg *msg);

/**
 * @brief Find the previous message in a call group
 *
 * @param callgroup SIP call group structure
 * @param msg Actual SIP msg from any call of the group
 * @return Previous chronological message in the group or NULL
 */
SipMsg *
call_group_get_prev_msg(SipCallGroup *group, SipMsg *msg);

/**
 * @brief Find the next stream in a call group
 *
 * @param callgroup SIP call group structure
 * @param msg Actual stream structure from any call of the group
 * @return next chronological stream in the group or NULL
 */
RtpStream *
call_group_get_next_stream(SipCallGroup *group, RtpStream *stream);

#endif /* __SNGREP_GROUP_H_ */
