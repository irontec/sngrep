/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
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
 * @file sip_call.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage sip calls
 *
 */
#ifndef __SNGREP_SIP_CALL_H
#define __SNGREP_SIP_CALL_H

#include "config.h"
#include <stdarg.h>
#include "vector.h"
#include "rtp.h"
#include "sip_msg.h"
#include "sip_attr.h"

//! Shorter declaration of sip_call structure
typedef struct sip_call sip_call_t;

//! SIP Call State
enum call_state
{
    SIP_CALLSTATE_CALLSETUP = 1,
    SIP_CALLSTATE_INCALL,
    SIP_CALLSTATE_CANCELLED,
    SIP_CALLSTATE_REJECTED,
    SIP_CALLSTATE_COMPLETED
};

/**
 * @brief Contains all information of a call and its messages
 *
 * This structure acts as header of messages list of the same
 * callid (considered a dialog). It contains some replicated
 * data from its messages to speed up searches.
 */
struct sip_call {
    // Call index in the call list
    int index;
    // Call identifier
    char *callid;
    //! Related Call identifier
    char *xcallid;
    //! Flag this call as filtered so won't be displayed
    signed char filtered;
    //! Call State. For dialogs starting with an INVITE method
    int state;

    //! List of messages of this call (sip_msg_t*)
    vector_t *msgs;
    //! Message when conversation started and ended
    sip_msg_t *cstart_msg, *cend_msg;
    //! RTP streams for this call (rtp_stream_t *)
    vector_t *streams;
    //! RTP packets for this call (capture_packet_t *)
    vector_t *rtp_packets;
};

/**
 * @brief Create a new call with the given callid (Minimum required data)
 *
 * Allocated required memory for a new SIP Call. The call acts as
 * header structure to all the messages with the same callid.
 *
 * @param callid Call-ID Header value
 * @return pointer to the sip_call created
 */
sip_call_t *
call_create(char *callid);

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
call_destroy(sip_call_t *call);


/**
 * @brief Wrapper around Message destroyer to clear call vectors
 */
void
call_destroyer(void *call);

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
call_add_message(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Append a new RTP stream to the call
 *
 * Add a new stream to be monitored
 *
 * @param call pointer to the call owner of the stream
 * @param stream RTP stream data
 */
void
call_add_stream(sip_call_t *call, rtp_stream_t *stream);

/**
 * @brief Append a new RTP packet to the call
 *
 * @param call pointer to the call owner of the stream
 * @param packet new RTP packet from call rtp streams
 */
void
call_add_rtp_packet(sip_call_t *call, capture_packet_t *packet);

/**
 * @brief Getter for call messages linked list size
 *
 * Return the number of messages stored in this call. All messages
 * share the same Call-ID
 *
 * @param call SIP call structure
 * @return how many messages are in the call
 */
int
call_msg_count(sip_call_t *call);

/**
 * @brief Finds the other leg of this call.
 *
 * If this call has a X-CID or X-Call-ID header, that call will be
 * find and returned. Otherwise, a call with X-CID or X-Call-ID header
 * matching the given call's Call-ID will be find or returned.
 *
 * @param call SIP call structure
 * @return The other call structure or NULL if none found
 */
sip_call_t *
call_get_xcall(sip_call_t *call);


int
call_is_active(void *item);

/**
 * @brief Determine if this call starts with an Invite request
 *
 * @param call SIP call structure
 * @return 1 if first call message has method INVITE, 0 otherwise
 */
int
call_is_invite(sip_call_t *call);

/**
 * @brief Check if a message is a retransmission
 *
 * This function will compare its payload with the previous message
 * in the dialog, to check if it has the same content.
 *
 * @param msg SIP message that will be checked
 * @return 1 if the previous message is equal to msg, 0 otherwise
 */
int
call_msg_is_retrans(sip_msg_t *msg);

/**
 * @brief Update Call State attribute with its last parsed message
 *
 * @param call Call structure to be updated
 * @param msg Last received message of this call
 */
void
call_update_state(sip_call_t *call, sip_msg_t *msg);

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
const char *
call_get_attribute(struct sip_call *call, enum sip_attr_id id, char *value);

/**
 * @brief Return the string represtation of a call state
 *
 */
const char *
call_state_to_str(int state);

#endif /* __SNGREP_SIP_CALL_H */
