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
 * @file sip_attr.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP calls and messages attributes
 */

#ifndef __SNGREP_SIP_ATTR_H
#define __SNGREP_SIP_ATTR_H

#include "sip.h"

/* Some very used macros */
#define CALLID(msg) msg_get_attribute(msg, SIP_ATTR_CALLID)
#define SRC(msg) msg_get_attribute(msg, SIP_ATTR_SRC)
#define DST(msg) msg_get_attribute(msg, SIP_ATTR_DST)
#define SRCHOST(msg) msg_get_attribute(msg, SIP_ATTR_SRC_HOST)
#define DSTHOST(msg) msg_get_attribute(msg, SIP_ATTR_DST_HOST)
#define TIME(msg) msg_get_attribute(msg, SIP_ATTR_TIME)
#define DATE(msg) msg_get_attribute(msg, SIP_ATTR_DATE)

//! Shorter declaration of sip_attr structure
typedef struct sip_attr_hdr sip_attr_hdr_t;
//! Shorter declaration of sip_attr structure
typedef struct sip_attr sip_attr_t;

// Forward struct declaration for calls and messages
struct sip_call;
struct sip_msg;

/**
 * @brief Available SIP Attributes
 *
 * This enum contains the list of available attributes
 * a call or message can have.
 */
enum sip_attr_id {
    //! Call index in the Call List
    SIP_ATTR_CALLINDEX = 0,
    //! SIP Message From: header
    SIP_ATTR_SIPFROM,
    //! SIP Message User of From: header
    SIP_ATTR_SIPFROMUSER,
    //! SIP Message To: header
    SIP_ATTR_SIPTO,
    //! SIP Message User of To: header
    SIP_ATTR_SIPTOUSER,
    //! Package IP source address and port
    SIP_ATTR_SRC,
    //! Package source lookup and port
    SIP_ATTR_SRC_HOST,
    //! Package IP destination address and port
    SIP_ATTR_DST,
    //! Package destination lookup and port
    SIP_ATTR_DST_HOST,
    //! SIP Message Call-ID header
    SIP_ATTR_CALLID,
    //! SIP Message X-Call-ID or X-CID header
    SIP_ATTR_XCALLID,
    //! SIP Message Date
    SIP_ATTR_DATE,
    //! SIP Message Time
    SIP_ATTR_TIME,
    //! SIP Message Method or Response code
    SIP_ATTR_METHOD,
    //! SIP Message is a request
    SIP_ATTR_REQUEST,
    //! SIP CSeq number
    SIP_ATTR_CSEQ,
    //! SIP Message has sdp
    SIP_ATTR_SDP,
    //! SDP Address
    SIP_ATTR_SDP_ADDRESS,
    //! SDP Port
    SIP_ATTR_SDP_PORT,
    //! SIP Message transport
    SIP_ATTR_TRANSPORT,
    //! SIP Call first message method
    SIP_ATTR_STARTING,
    //! SIP Call message counter
    SIP_ATTR_MSGCNT,
    //! SIP Call state
    SIP_ATTR_CALLSTATE,
    //! Conversation duration
    SIP_ATTR_CONVDUR,
    //! Total call duration
    SIP_ATTR_TOTALDUR,
    //! SIP Attribute count
    SIP_ATTR_SENTINEL
};

/**
 * @brief Attribute header data
 *
 * This sctructure contains the information about the
 * attribute, description, id, type and so. It's the
 * static information of the attributed shared by all
 * attributes pointer to its type.
 *
 */
struct sip_attr_hdr {
    //! Attribute id
    enum sip_attr_id id;
    //! Attribute name
    char *name;
    //! Attribute description
    char *desc;
    //! Attribute default display width
    int dwidth;
};

/**
 * @brief Attribute data structure
 *
 * This structure contains a single attribute value and acts
 * as a linked list for all attributes of a message (or call)
 * Right now, all the attributed are stored as strings, which may
 * not be the better option, but will fit our actual needs.
 */
struct sip_attr {
    //! Attribute header pointer
    sip_attr_hdr_t *hdr;
    //! Attribute value
    char *value;
    //! Next attribute in the linked list
    sip_attr_t *next;
};

/**
 * @brief Get the header information of an Attribute
 *
 * Retrieve header data from attribute list
 *
 * @param id Attribute id
 * @return Attribute header data structure pointer
 */
sip_attr_hdr_t *
sip_attr_get_header(enum sip_attr_id id);

/**
 * @brief Get Attribute description
 *
 * Retrieve description of given attribute from its
 * header structure.
 *
 * @param id Attribute id
 * @return Attribute description from its header
 */
const char *
sip_attr_get_description(enum sip_attr_id id);

/**
 * @brief Get Attribute name
 *
 * Retrieve name of given attribute from its
 * header structure.
 *
 * @param id Attribute id
 * @return Attribute name from its header
 */
const char *
sip_attr_get_name(enum sip_attr_id id);

/**
 * @brief Get Attribute prefered display width
 *
 * @param id Attribute id
 * @return prefered attribute width
 */
int
sip_attr_get_width(enum sip_attr_id id);

/**
 * @brief Get Attribute id from its name
 *
 * Retrieve attribute id of the given attribute name.
 *
 * @param name Attribut name
 * @return Attribute id or -1 if not found
 */
enum sip_attr_id
sip_attr_from_name(const char *name);

/**
 * @brief Deallocate all attributes of a list
 *
 * @param list Pointer to the attribute list
 */
void
sip_attr_list_destroy(sip_attr_t *list);

/**
 * @brief Sets the given attribute value to an attribute
 *
 * Primitive for setting an attribute value of a given attribute list.
 * This can be used for calls and message attributes.
 *
 * @param list Pointer to the attribute list
 * @param id Attribute id
 * @param value Attribute value
 */
void
sip_attr_set(sip_attr_t **list, enum sip_attr_id id, const char *value);

/**
 * @brief Gets the given attribute value to an attribute
 *
 * Primitive for getting an attribute value of a given attribute list.
 * This can be used for calls and message attributes.
 *
 */
const char *
sip_attr_get(sip_attr_t *list, enum sip_attr_id id);

/**
 * @brief Sets the attribute value for a given call
 *
 * This function acts as wrapper of sip call attributes
 *
 * @param call SIP call structure
 * @param id Attribute id
 * @param value Attribute value
 */
void
call_set_attribute(struct sip_call *call, enum sip_attr_id id, const char *fmt, ...);

/**
 * @brief Return a call attribute value
 *
 * This function will be used to avoid accessing call structure
 * fields directly.
 * @todo Code a proper way to store this information
 *
 * @param call SIP call structure
 * @param id Attribute id
 * @return Attribute value or NULL if not found
 */
const char *
call_get_attribute(struct sip_call *call, enum sip_attr_id id);

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

/**
 * @brief Check if this msg is affected by filters
 *
 * @param call Message to check
 * @return 1 if msg is filtered, 0 otherwise
 */
int
sip_check_msg_ignore(struct sip_msg *msg);

#endif /* __SNGREP_SIP_ATTR_H */
