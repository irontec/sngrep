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

typedef gchar *(*AttributeGetterFunc)(Attribute *, Message *);

/**
 * @brief Available SIP Attributes
 *
 * This enum contains the list of available attributes
 * a call or message can have.
 */

#define ATTR_CALLINDEX      "callindex"         //! Call index in the Call List
#define ATTR_SIPFROM        "sipfrom"           //! SIP Message From: header
#define ATTR_SIPFROMUSER    "sipfromuser"       //! SIP Message User of From: header
#define ATTR_SIPTO          "sipto"             //! SIP Message To: header
#define ATTR_SIPTOUSER      "siptouser"         //! SIP Message User of To: header
#define ATTR_SRC            "src"               //! Package IP source address and port
#define ATTR_DST            "dst"               //! Package IP destination address and port
#define ATTR_CALLID         "callid"            //! SIP Message Call-ID header
#define ATTR_XCALLID        "xcallid"           //! SIP Message X-Call-ID or X-CID header
#define ATTR_DATE           "date"              //! SIP Message Date
#define ATTR_TIME           "time"              //! SIP Message Time
#define ATTR_METHOD         "method"            //! SIP Message Method or Response code
#define ATTR_TRANSPORT      "transport"         //! SIP Message transport
#define ATTR_MSGCNT         "msgcnt"            //! SIP Call message counter
#define ATTR_CALLSTATE      "callstate"         //! SIP Call state
#define ATTR_CONVDUR        "convdur"           //! Conversation duration
#define ATTR_TOTALDUR       "totaldur"          //! Total call duration
#define ATTR_REASON_TXT     "reason_txt"        //! Text from SIP Reason header
#define ATTR_WARNING        "warning"           //! Warning Header


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
    //! Name (Unique identifier)
    gchar *name;
    //! Column title (Displayed in Call List window)
    gchar *title;
    //! Description (Displayed in column selection list)
    gchar *desc;
    //! Determine if this attribute value changes over time
    gboolean mutable;
    //! Determine attribute prefered length
    gint length;

    //! Regular expression pattern
    const gchar *regexp_pattern;
    //! Compiled regexp
    GRegex *regex;

    //! This function calculates attribute value
    AttributeGetterFunc getterFunc;
    //! This function determines the color of this attribute in CallList
    AttributeColorFunc colorFunc;
};

/**
 * @brief Single Attribute value
 */
typedef struct
{
    //! Pointer to Attribute Header
    Attribute *attr;
    //! Actual attribute value
    gchar *value;
} AttributeValue;

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
attribute_get_description(Attribute *attribute);

/**
 * @brief Set Attribute description
 *
 * This function does not allocate any memory and reuses desc pointer
 *
 * @param attr Attribute pointer to update
 * @param desc pointer to Attribute description
 */
void
attribute_set_description(Attribute *attr, const gchar *desc);

/**
 * @brief Get Attribute title
 *
 * Retrieve title of given attribute from its
 * header structure.
 *
 * @param attr Attribute pointer
 * @return Attribute title from its header
 */
const gchar *
attribute_get_title(Attribute *attr);

/**
 * @brief Set Attribute title
 *
 * This function does not allocate any memory and reuses title pointer
 *
 * @param attr Attribute pointer to update
 * @param title pointer to Attribute title
 */
void
attribute_set_title(Attribute *attr, const gchar *title);

/**
 * @brief Get Attribute name
 *
 * Retrieve name of given attribute from its
 * header structure.
 *
 * @param attr Attribute pointer
 * @return Attribute name from its header
 */
const gchar *
attribute_get_name(Attribute *attr);

/**
 * @brief Set Attribute display length
 *
 * @param attr Attribute pointer to update
 * @param length Preferred display length
 */
gint
attribute_get_length(Attribute *attr);

/**
 * @brief Get Attribute display length
 *
 * @param attr Attribute pointer
 * @return Attribute preferred display length
 */
void
attribute_set_length(Attribute *attr, gint length);

/**
 * @brief Get Attribute id from its name
 *
 * Retrieve attribute id of the given attribute name.
 *
 * @param name Attribute name
 * @return Attribute header pointer or NULL if not found
 */
Attribute *
attribute_find_by_name(const gchar *name);

/**
 * @brief Return Attribute value for a given message
 * @param name Name of the attribute
 * @param msg Msg to get attribute from
 * @return string representation of the attribute
 */
gchar *
attribute_get_value(Attribute *attr, Message *msg);

/**
 * @brief Determine the color of the attribute in Call List
 *
 * Return the color pair to display an attribute in
 * call list or -1 if default color must be used.
 */
gint
attribute_get_color(Attribute *attr, const gchar *value);

/**
 * @brief Determine the color of the attribute in Call List
 *
 * This function can be used to show the Method attribute
 * with different colours in Call List.
 */
gint
attribute_color_sip_method(const gchar *value);

/**
 * @brief Determine the color of the attribute in Call List
 *
 * This function can be used to show the state attribute
 * with different colours in Call List.
 */
gint
attribute_color_call_state(const gchar *value);

/**
 * @brief Allocate memory to store a cached attribute value
 */
AttributeValue *
attribute_value_new(Attribute *attr, gchar *value);

/**
 * @brief Free allocated memory for an attribute value
 */
void
attribute_value_free(AttributeValue *attr_value);

GPtrArray *
attribute_get_internal_array();

void
attribute_from_setting(const gchar *setting, const gchar *value);

void
attribute_init();

void
attribute_deinit();

void
attribute_dump();

#endif /* __SNGREP_ATTRIBUTE_H */
