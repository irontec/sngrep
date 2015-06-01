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

#include "config.h"

/* Some very used macros */
#define CALLID(msg) msg_get_attribute(msg, SIP_ATTR_CALLID)
#define SRC(msg) msg_get_attribute(msg, SIP_ATTR_SRC)
#define DST(msg) msg_get_attribute(msg, SIP_ATTR_DST)
#define TIME(msg) msg_get_attribute(msg, SIP_ATTR_TIME)
#define DATE(msg) msg_get_attribute(msg, SIP_ATTR_DATE)

//! Shorter declaration of sip_attr structure
typedef struct sip_attr_hdr sip_attr_hdr_t;

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
    //! Package IP destination address and port
    SIP_ATTR_DST,
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
    //! SDP Address
    SIP_ATTR_SDP_ADDRESS,
    //! SDP Port
    SIP_ATTR_SDP_PORT,
    //! SIP Message transport
    SIP_ATTR_TRANSPORT,
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
    //! Attribute column title
    char *title;
    //! Attribute description
    char *desc;
    //! Attribute default display width
    int dwidth;
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
 * @brief Get Attribute title
 *
 * Retrieve title of given attribute from its
 * header structure.
 *
 * @param id Attribute id
 * @return Attribute title from its header
 */
const char *
sip_attr_get_title(enum sip_attr_id id);

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
sip_attr_list_destroy(char *attrs[]);

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
sip_attr_set(char *attrs[], enum sip_attr_id id, const char *value);

/**
 * @brief Gets the given attribute value to an attribute
 *
 * Primitive for getting an attribute value of a given attribute list.
 * This can be used for calls and message attributes.
 *
 */
const char *
sip_attr_get(char *attrs[], enum sip_attr_id id);

#endif /* __SNGREP_SIP_ATTR_H */
