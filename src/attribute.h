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
 * @file attribute.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP calls and messages attributes
 */

#ifndef __SNGREP_ATTRIBUTE_H
#define __SNGREP_ATTRIBUTE_H

#include <glib.h>

//! Max attribute length
#define ATTR_MAXLEN 255

//! Shorter declaration of sip_attr_hdr structure
typedef struct _AttributeHeader AttributeHeader;
//! Shorter declaration of sip_attr structure
typedef struct _Attribute Attribute;

/**
 * @brief Available SIP Attributes
 *
 * This enum contains the list of available attributes
 * a call or message can have.
 */
enum AttributeId {
    //! Call index in the Call List
    ATTR_CALLINDEX = 0,
    //! SIP Message From: header
    ATTR_SIPFROM,
    //! SIP Message User of From: header
    ATTR_SIPFROMUSER,
    //! SIP Message To: header
    ATTR_SIPTO,
    //! SIP Message User of To: header
    ATTR_SIPTOUSER,
    //! Package IP source address and port
    ATTR_SRC,
    //! Package IP destination address and port
    ATTR_DST,
    //! SIP Message Call-ID header
    ATTR_CALLID,
    //! SIP Message X-Call-ID or X-CID header
    ATTR_XCALLID,
    //! SIP Message Date
    ATTR_DATE,
    //! SIP Message Time
    ATTR_TIME,
    //! SIP Message Method or Response code
    ATTR_METHOD,
    //! SIP Message transport
    ATTR_TRANSPORT,
    //! SIP Call message counter
    ATTR_MSGCNT,
    //! SIP Call state
    ATTR_CALLSTATE,
    //! Conversation duration
    ATTR_CONVDUR,
    //! Total call duration
    ATTR_TOTALDUR,
    //! Text from SIP Reason header
    ATTR_REASON_TXT,
    //! Warning Header
    ATTR_WARNING,
    //! SIP Attribute count
    ATTR_COUNT
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
struct _AttributeHeader {
    //! Attribute id
    enum AttributeId id;
    //! Attribute name
    gchar *name;
    //! Attribute column title
    gchar *title;
    //! Attribute description
    gchar *desc;
    //! Attribute default display width
    guint dwidth;
    //! This function determines the color of this attribute in CallList
    int (*color)(const char *value);
};

/**
 * @brief Attribute storage struct
 */
struct _Attribute {
    //! Attribute id
    enum AttributeId id;
    //! Attribute value
    gchar *value;
};

/**
 * @brief Get the header information of an Attribute
 *
 * Retrieve header data from attribute list
 *
 * @param id Attribute id
 * @return Attribute header data structure pointer
 */
AttributeHeader *
attr_header(enum AttributeId id);

/**
 * @brief Get Attribute description
 *
 * Retrieve description of given attribute from its
 * header structure.
 *
 * @param id Attribute id
 * @return Attribute description from its header
 */
const gchar *
attr_description(enum AttributeId id);

/**
 * @brief Get Attribute title
 *
 * Retrieve title of given attribute from its
 * header structure.
 *
 * @param id Attribute id
 * @return Attribute title from its header
 */
const gchar *
attr_title(enum AttributeId id);

/**
 * @brief Get Attribute name
 *
 * Retrieve name of given attribute from its
 * header structure.
 *
 * @param id Attribute id
 * @return Attribute name from its header
 */
const gchar *
attr_name(enum AttributeId id);

/**
 * @brief Get Attribute prefered display width
 *
 * @param id Attribute id
 * @return prefered attribute width
 */
int
attr_width(enum AttributeId id);

/**
 * @brief Get Attribute id from its name
 *
 * Retrieve attribute id of the given attribute name.
 *
 * @param name Attribut name
 * @return Attribute id or -1 if not found
 */
gint
attr_find_by_name(const gchar *name);

/**
 * @brief Determine the color of the attribute in Call List
 *
 * Return the color pair to display an attribute in
 * call list or -1 if default color must be used.
 */
gint
attr_color(enum AttributeId id, const gchar *value);

/**
 * @brief Determine the color of the attribute in Call List
 *
 * This function can be used to show the Method attribute
 * with different colours in Call List.
 */
gint
attr_color_sip_method(const gchar *value);

/**
 * @brief Determine the color of the attribute in Call List
 *
 * This function can be used to show the state attribute
 * with different colours in Call List.
 */
gint
attr_color_call_state(const gchar *value);

#endif /* __SNGREP_ATTRIBUTE_H */
