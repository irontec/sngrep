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
 * @file sip_msg.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage sip messages
 *
 */
#ifndef __SNGREP_SIP_MSG_H
#define __SNGREP_SIP_MSG_H

#include <stdarg.h>
#include <glib.h>
#include <packet/dissectors/packet_sdp.h>
#include "packet/packet.h"
#include "attribute.h"
#include "timeval.h"

//! Get IP Address info from message's packet
#define msg_src_address(msg)   (packet_src_address(msg->packet))
#define msg_dst_address(msg)   (packet_dst_address(msg->packet))


//! Shorter declaration of sip_msg structure
typedef struct _SipMsg SipMsg;
//! Forward declarition of SipCall type
typedef struct _SipCall SipCall;

/**
 * @brief Information of a single message withing a dialog.
 *
 * Most of the data is just stored to be displayed in the UI so
 * the formats may be no the best, but the simplest for this
 * purpose. It also works as a linked lists of messages in a
 * call.
 */
struct _SipMsg {
    //! SDP payload information (sdp_media_t *)
    GList *medias;
    //! Captured packet for this message
    Packet *packet;
    //! Message owner
    SipCall *call;
    //! Message is a retransmission from other message
    const SipMsg *retrans;
};


/**
 * @brief Create a new message from the readed header and payload
 *
 * Allocate required memory for a new SIP message. This function
 * will only store the given information, but wont parse it until
 * needed.
 *
 * @param payload Raw payload content
 * @return a new allocated message
 */
SipMsg *
msg_create();

/**
 * @brief Destroy a SIP message and free its memory
 *
 * Deallocate memory of an existing SIP Message.
 * This function will remove the message from the call and the
 * passed pointer will be NULL.
 *
 * @param nsg SIP message to be deleted
 */
void
msg_destroy(gpointer item);

/**
 * @brief Return the call owner of this message
 */
SipCall *
msg_get_call(const SipMsg *msg);

/**
 * @brief Getter for media of given messages
 *
 * Return the number of media structures of given msg
 * stored in this call.
 *
 * @param msg SIP message structure
 * @return how many media structures are in the msg
 */
guint
msg_media_count(SipMsg *msg);

/**
 * @brief Get Media information for a given destination address
 * @param msg SIP message structure*
 * @param dst Address structure
 * @return Media info or NULL for not match
 */
PacketSdpMedia *
msg_media_for_addr(SipMsg *msg, Address dst);

/**
 * @brief Check if given message has spd content
 */
gboolean
msg_has_sdp(void *item);

/**
 * @brief Check if a message is a Request or response
 *
 * @param msg SIP message that will be checked
 * @return TRUE if the message is a request, FALSE if a response
 */
gboolean
msg_is_request(SipMsg *msg);

/**
 * @brief Get SIP Message payload
 */
const gchar *
msg_get_payload(SipMsg *msg);

/**
 * @brief Get Time of message from packet header
 *
 * @param msg SIP message
 * @return timeval structure with message first packet time
 */
GTimeVal
msg_get_time(const SipMsg *msg);

/**
 * @brief Return a message attribute value
 *
 * This function will be used to avoid accessing call structure
 * fields directly.
 *
 * @param msg SIP message structure
 * @param id Attribute id
 * @param out Buffer to store attribute value
 * @return Attribute value or NULL if not found
 */
const gchar *
msg_get_attribute(SipMsg *msg, gint id, char *value);

const gchar *
msg_get_preferred_codec_alias(SipMsg *msg);

/**
 * @brief Get summary of message header data
 *
 * For raw prints, it's handy to have the ngrep header style message
 * data.
 *
 * @param msg SIP message
 * @param out pointer to allocated memory to contain the header output
 * @returns pointer to out
 */
const gchar *
msg_get_header(SipMsg *msg, gchar *out);

/**
 * @brief Check if given message is a retransmission
 *
 * Compare message payload with the previous messages in the dialog,
 * to check if it one has the same content.
 *
 * @param msg SIP Message
 * @return pointer to original message or NULL if message is not a retransmission
 */
const SipMsg *
msg_is_retrans(SipMsg *msg);

#endif /* __SNGREP_SIP_MSG_H */
