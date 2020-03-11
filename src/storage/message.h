/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
 * @file message.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage sip messages
 *
 */
#ifndef __SNGREP_MESSAGE_H
#define __SNGREP_MESSAGE_H

#include <glib.h>
#include "packet/packet_sdp.h"
#include "address.h"
#include "attribute.h"
#include "datetime.h"

//! Shorter declaration of sip_msg structure
typedef struct _Message Message;
//! Forward declaration of SipCall type
typedef struct _Call Call;

/**
 * @brief Information of a single message withing a dialog.
 *
 * Most of the data is just stored to be displayed in the UI so
 * the formats may be no the best, but the simplest for this
 * purpose. It also works as a linked lists of messages in a
 * call.
 */
struct _Message
{
    //! Message owner
    Call *call;
    //! Captured packet for this message
    Packet *packet;
    //! Attribute list for this message
    GSList *attributes;
    //! Message is a retransmission from other message
    gint retrans;
};


/**
 * @brief Create a new message from the readed header and payload
 *
 * Allocate required memory for a new SIP message. This function
 * will only store the given information, but wont parse it until
 * needed.
 *
 * @param packet Packet containing SIP message data
 * @return a new allocated message
 */
Message *
msg_new(Packet *packet);

/**
 * @brief Destroy a SIP message and free its memory
 *
 * Deallocate memory of an existing SIP Message.
 * This function will remove the message from the call and the
 * passed pointer will be NULL.
 *
 * @param msg SIP message to be free'd
 */
void
msg_free(Message *msg);

/**
 * @brief Return the call owner of this message
 */
Call *
msg_get_call(const Message *msg);

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
msg_media_count(Message *msg);

/**
 * @brief Get Media information for a given destination address
 * @param msg SIP message structure*
 * @param dst Address structure
 * @return Media info or NULL for not match
 */
PacketSdpMedia *
msg_media_for_addr(Message *msg, Address dst);

/**
 * @brief Check if given message has spd content
 */
gboolean
msg_has_sdp(void *item);

/**
 * @brief Get Source address pointer for given SIP message
 * @param msg
 * @return
 */
Address
msg_src_address(Message *msg);

/**
 * @brief Get Destination address pointer for given SIP message
 * @param msg
 * @return
 */
Address
msg_dst_address(Message *msg);

/**
 * @brief Check if a message is a Request or response
 *
 * @param msg SIP message that will be checked
 * @return TRUE if the message is a request, FALSE if a response
 */
gboolean
msg_is_request(Message *msg);

/**
 * @brief Return Message method code
 * @param msg SIP message pointer
 * @return Method code if message is a request, Response code if message is a response
 */
guint
msg_get_method(Message *msg);

/**
 * @brief Return Message method in string
 * @param msg SIP message pointer
 * @return Method string if message is a request, Response text if message is a response
 */
const gchar *
msg_get_method_str(Message *msg);

/**
 * @brief Determine if message is part of initial transaction
 *
 * @param msg SIP message that will be checked
 * @return TRUE if the message is part of the initial transaction
 */
gboolean
msg_is_initial_transaction(Message *msg);

/**
 * @brief Get CSeq Number for given SIP Message
 * @return CSeq numeric header value
 */
guint64
msg_get_cseq(Message *msg);

/**
 * @brief Get SIP Message payload
 *
 * Payload data must be freed when not longer required
 */
gchar *
msg_get_payload(Message *msg);

/**
 * @brief Get Time of message from packet header
 *
 * @param msg SIP message
 * @return seconds in unix timestamp microseconds
 */
guint64
msg_get_time(const Message *msg);

/**
 * @brief Return a message attribute value
 *
 * This function will be used to avoid accessing call structure
 * fields directly.
 *
 * @param msg SIP message structure
 * @param id Attribute id
 * @return Attribute value or NULL if not found
 */
const gchar *
msg_get_attribute(Message *msg, gint id);

const gchar *
msg_get_preferred_codec_alias(Message *msg);

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
msg_get_header(Message *msg, gchar *out);

/**
 * @brief Check if given message is a retransmission
 *
 * Compare message payload with the previous messages in the dialog,
 * to check if it one has the same content.
 *
 * @param msg SIP Message
 * @return TRUE is message is a Retransmission, FALSE otherwise
 */
gboolean
msg_is_retransmission(Message *msg);

/**
 * @brief Check if given message is a retransmission
 *
 * Compare message payload with the previous messages in the dialog,
 * to check if it one has the same content.
 *
 * @param msg SIP Message
 * @return pointer to original message or NULL if message is not a retransmission
 */
const Message *
msg_get_retransmission_original(Message *msg);

/**
 * @brief Check if the given message is a capture duplicate
 *
 * Some interfaces duplicates captured packets so, in order to detect a message as
 * duplicated instead of retrans, the time difference with the original must be below
 * the t1 timer.
 *
 * @param msg SIP Message
 * @return return TRUE if message is considered duplicated, FALSE otherwise
 */
gboolean
msg_is_duplicate(Message *msg);

/**
 * @brief Get Authorization-Header content
 * @param msg SIP Message
 * @return Authorization-Header content or NULL if message has no Auth header
 */
const gchar *
msg_get_auth_hdr(const Message *msg);

#endif /* __SNGREP_MESSAGE_H */
