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
 * @file sip_msg.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage sip messages
 *
 */
#ifndef __SNGREP_SIP_MSG_H
#define __SNGREP_SIP_MSG_H

#include "config.h"
#include "vector.h"
#include "media.h"
#include "rtp.h"
#include "sip_attr.h"
#include "util.h"

//! Shorter declaration of sip_msg structure
typedef struct sip_msg sip_msg_t;

//! SIP Methods
enum sip_methods {
    SIP_METHOD_REGISTER = 1,
    SIP_METHOD_INVITE,
    SIP_METHOD_SUBSCRIBE,
    SIP_METHOD_NOTIFY,
    SIP_METHOD_OPTIONS,
    SIP_METHOD_PUBLISH,
    SIP_METHOD_MESSAGE,
    SIP_METHOD_CANCEL,
    SIP_METHOD_BYE,
    SIP_METHOD_ACK,
    SIP_METHOD_PRACK,
    SIP_METHOD_INFO,
    SIP_METHOD_REFER,
    SIP_METHOD_UPDATE,
    SIP_METHOD_SENTINEL,
};

/**
 * @brief Information of a single message withing a dialog.
 *
 * Most of the data is just stored to be displayed in the UI so
 * the formats may be no the best, but the simplest for this
 * purpose. It also works as a linked lists of messages in a
 * call.
 */
struct sip_call;
struct sip_msg {
    //! Temporal payload data before being parsed
    u_char *payload;
    //! Color for this message (in color.cseq mode)
    int color;
    //! Request Method or Response Code @see sip_methods
    int reqresp;
    //! Message Cseq
    int cseq;
    //! Message attribute list
    vector_t *attrs;
    //! SDP payload information (sdp_media_t *)
    vector_t *medias;
    //! Captured packets for this message (capture_packet_t *)
    vector_t *packets;
    //! Message owner
    struct sip_call *call;
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
sip_msg_t *
msg_create(const char *payload);

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
msg_destroy(sip_msg_t *msg);

/**
 * @brief Wrapper around Message destroyer to clear msg vectors
 */
void
msg_destroyer(void *msg);

/**
 * @brief Getter for media of given messages
 *
 * Return the number of media structures of given msg
 * stored in this call.
 *
 * @param msg SIP message structure
 * @return how many media structures are in the msg
 */
int
msg_media_count(sip_msg_t *msg);

/**
 * @brief Check if given message has spd content
 */
int
msg_has_sdp(void *item);

/**
 * @brief Add a media structure to a msg
 *
 * @param cmsg SIP Message to be updated
 * @param media Media structure to be added
 */
void
msg_add_media(sip_msg_t *msg, sdp_media_t *media);

/**
 * @brief Check if a package is a retransmission
 *
 * This function will compare its payload with the previous message
 * in the dialog, to check if it has the same content.
 *
 * @param msg SIP message that will be checked
 * @return 1 if the previous message is equal to msg, 0 otherwise
 */
int
msg_is_retrans(sip_msg_t *msg);

/**
 * @brief Check if a message is a Request or response
 *
 * @param msg SIP message that will be checked
 * @return 1 if the message is a request, 0 if a response
 */
int
msg_is_request(sip_msg_t *msg);

/**
 * @brief Add a new packet for given message
 *
 * A SIP message can have multiple fragmented packets to
 * store its payload;
 *
 * @param msg SIP message that will store this packet
 * @param packet Captured packet information
 */
void
msg_add_packet(sip_msg_t *msg, capture_packet_t *packet);

/**
 * @brief Get SIP Message payload
 */
const char *
msg_get_payload(sip_msg_t *msg);

/**
 * @brief Get Time of message from packet header
 *
 * @param msg SIP message
 * @return timeval structure with message first packet time
 */
struct timeval
msg_get_time(sip_msg_t *msg);

/**
 * @brief Sets the attribute value for a given message
 *
 * This function acts as wrapper of sip message attributes
 *
 * @param msg SIP message structure
 * @param id Attribute id
 * @param value Attribute value
 */
void
msg_set_attribute(struct sip_msg *msg, enum sip_attr_id id, const char *fmt, ...);

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
const char *
msg_get_attribute(struct sip_msg *msg, enum sip_attr_id id);


#endif /* __SNGREP_SIP_MSG_H */
