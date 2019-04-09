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

//! Attribute types
typedef struct _Attribute Attribute;
typedef struct _Message Message;

typedef gint (*AttributeColorFunc)(const gchar *);

typedef gchar * (*AttributeGetterFunc)(Attribute *, Message *);

/**
 * @brief Available SIP Attributes
 *
 * This enum contains the list of available attributes
 * a call or message can have.
 */
enum AttributeId
{
    ATTR_CALLINDEX = 0,     //! Call index in the Call List
    ATTR_SIPFROM,           //! SIP Message From: header
    ATTR_SIPFROMUSER,       //! SIP Message User of From: header
    ATTR_SIPTO,             //! SIP Message To: header
    ATTR_SIPTOUSER,         //! SIP Message User of To: header
    ATTR_SRC,               //! Package IP source address and port
    ATTR_DST,               //! Package IP destination address and port
    ATTR_CALLID,            //! SIP Message Call-ID header
    ATTR_XCALLID,           //! SIP Message X-Call-ID or X-CID header
    ATTR_DATE,              //! SIP Message Date
    ATTR_TIME,              //! SIP Message Time
    ATTR_METHOD,            //! SIP Message Method or Response code
    ATTR_TRANSPORT,         //! SIP Message transport
    ATTR_MSGCNT,            //! SIP Call message counter
    ATTR_CALLSTATE,         //! SIP Call state
    ATTR_CONVDUR,           //! Conversation duration
    ATTR_TOTALDUR,          //! Total call duration
    ATTR_REASON_TXT,        //! Text from SIP Reason header
    ATTR_WARNING,           //! Warning Header
    ATTR_COUNT              //! SIP Attribute count
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
struct _Attribute
{
    //! Attribute id
    enum AttributeId id;
    //! Name (Unique identifier)
    gchar *name;
    //! Column title (Displayed in Call List window)
    gchar *title;
    //! Description (Displayed in column selection list)
    gchar *desc;
    //! Determine if this attribute value changes over time
    gboolean mutable;

    //! Regular expression pattern
    gchar *regexp_pattern;
    //! Compiled regexp
    GRegex *regex;

    //! This function calculates attribute value
    AttributeGetterFunc getterFunc;
    //! This function determines the color of this attribute in CallList
    AttributeColorFunc colorFunc;
};

/**
 * @brief Get the header information of an Attribute
 *
 * Retrieve header data from attribute list
 *
 * @param id Attribute id
 * @return Attribute header data structure pointer
 */
Attribute *
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
 * @brief Return Attribute value for a given message
 * @param name Name of the attribute
 * @param msg Msg to get attribute from
 * @return string representation of the attribute
 */
const gchar *
attr_get_value(const gchar *name, Message *msg);

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

void
attribute_init();

#endif /* __SNGREP_ATTRIBUTE_H */
