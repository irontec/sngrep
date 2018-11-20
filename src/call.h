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
 * @file call.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage sip calls
 *
 */
#ifndef __SNGREP_CALL_H
#define __SNGREP_CALL_H

#include <stdarg.h>
#include <stdbool.h>
#include <glib.h>
#include "stream.h"
#include "message.h"
#include "attribute.h"

//! Shorter declaration of sip_call structure
typedef struct _Call Call;

//! SIP Call State
enum CallState
{
    CALL_STATE_CALLSETUP = 1,
    CALL_STATE_INCALL,
    CALL_STATE_CANCELLED,
    CALL_STATE_REJECTED,
    CALL_STATE_DIVERTED,
    CALL_STATE_BUSY,
    CALL_STATE_COMPLETED
};

/**
 * @brief Contains all information of a call and its messages
 *
 * This structure acts as header of messages list of the same
 * callid (considered a dialog). It contains some replicated
 * data from its messages to speed up searches.
 */
struct _Call {
    //! Call index in the call list
    guint index;
    //! Call identifier
    const gchar *callid;
    //! Related Call identifier
    const gchar *xcallid;
    //! Flag this call as filtered so won't be displayed
    gchar filtered;
    //! Call State. For dialogs starting with an INVITE method
    enum CallState state;
    //! Changed flag. For interface optimal updates
    gboolean changed;
    //! Locked flag. Calls locked are never deleted
    gboolean locked;
    //! Last reason text value for this call
    gchar *reasontxt;
    //! Last warning text value for this call
    gint warning;
    //! List of calls with with this call as X-Call-Id
    GPtrArray *xcalls;
    //! Cseq from invite startint the call
    gint invitecseq;
    //! Array of messages of this call (sip_msg_t*)
    GPtrArray *msgs;
    //! Message when conversation started and ended
    Message *cstart_msg, *cend_msg;
    //! RTP streams for this call (rtp_stream_t *)
    GPtrArray *streams;
    //! RTP packets for this call (capture_packet_t *)
    GSequence *rtp_packets;
};

/**
 * @brief Create a new call with the given callid (Minimum required data)
 *
 * Allocated required memory for a new SIP Call. The call acts as
 * header structure to all the messages with the same callid.
 *
 * @param callid Call-ID Header value
 * @param xcallid X-Call-ID Header value
 * @return pointer to the sip_call created
 */
Call *
call_create(const gchar *callid, const gchar *xcallid);

/**
 * @brief Free all related memory from a call and remove from call list
 *
 * Deallocate memory of an existing SIP Call.
 * This will also remove all messages, calling sip_msg_destroy for each
 * one.
 *
 * @param call Call to be destroyed
 */
void
call_destroy(gpointer item);

/**
 * @brief Append message to the call's message list
 *
 * Creates a relation between this call and the message, appending it
 * to the end of the message list and setting the message owner.
 *
 * @param call pointer to the call owner of the message
 * @param msg SIP message structure
 */
void
call_add_message(Call *call, Message *msg);

/**
 * @brief Append a new RTP stream to the call
 *
 * Add a new stream to be monitored
 *
 * @param call pointer to the call owner of the stream
 * @param stream RTP stream data
 */
void
call_add_stream(Call *call, RtpStream *stream);

/**
 * @brief Getter for call messages linked list size
 *
 * Return the number of messages stored in this call. All messages
 * share the same Call-ID
 *
 * @param call SIP call structure
 * @return how many messages are in the call
 */
guint
call_msg_count(const Call *call);


/**
 * @brief Get given call call state
 *
 * @param call SIP call structure
 * @return TRUE if call is during conversation
 */
enum CallState
call_state(Call *call);

/**
 * @brief Determine if this call starts with an Invite request
 *
 * @param call SIP call structure
 * @return TRUE if first call message has method INVITE, FALSE otherwise
 */
gboolean
call_is_invite(Call *call);

/**
 * @brief Update Call State attribute with its last parsed message
 *
 * @param call Call structure to be updated
 * @param msg Last received message of this call
 */
void
call_update_state(Call *call, Message *msg);

/**
 * @brief Return a call attribute value
 *
 * This function will be used to avoid accessing call structure
 * fields directly.
 *
 * @param call SIP call structure
 * @param id Attribute id
 * @return Attribute value or NULL if not found
 */
const gchar *
call_get_attribute(const Call *call, enum AttributeId id, char *value);

/**
 * @brief Return the string represtation of a call state
 *
 */
const gchar *
call_state_to_str(enum CallState state);

/**
 * @brief Compare two calls based on a given attribute
 *
 * @return 0 if call attributes are equal
 * @return 1 if first call is greater
 * @return -1 if first call is lesser
 */
gint
call_attr_compare(const Call *one, const Call *two, enum AttributeId id);

/**
 * @brief Relate this two calls
 *
 * Add a call to the internal xcalls vector of another call.
 * This calls are related by the SIP header X-Call-Id or X-CID
 *
 * @param call SIP call structure
 * @param xcall SIP call structure
 */
void
call_add_xcall(Call *call, Call *xcall);

RtpStream *
call_find_stream(Call *call, Address src, Address dst);

RtpStream *
call_find_stream_exact(Call *call, Address src, Address dst);

#endif /* __SNGREP_CALL_H */
