/**************************************************************************
 **
 ** sngrep - SIP callflow viewer using ngrep
 **
 ** Copyright (C) 2013 Ivan Alonso (Kaian)
 ** Copyright (C) 2013 Irontec SL. All rights reserved.
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
 * @file sip.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP calls and messages
 *
 * This file contains the functions and structures to manage the SIP calls and
 * messages.
 */

#ifndef __SNGREP_SIP_H
#define __SNGREP_SIP_H

#include <sys/time.h>
#include <pthread.h>

//! Shorter declaration of sip_call structure
typedef struct sip_call sip_call_t;
//! Shorter declaration of sip_msg structure
typedef struct sip_msg sip_msg_t;
//! Shorter declaration of sip_attr structure
typedef struct sip_attr_hdr sip_attr_hdr_t;
//! Shorter declaration of sip_attr structure
typedef struct sip_attr sip_attr_t;

/**
 * @brief Available SIP Attributes
 *
 * This enum contains the list of available attributes
 * a call or message can have.
 */
enum sip_attr_id
{
    //! SIP Message From: header
    SIP_ATTR_SIPFROM = 1,
    //! SIP Message To: header
    SIP_ATTR_SIPTO,
    //! Package IP source address and port
    SIP_ATTR_SRC,
    //! Package IP destiny address and port
    SIP_ATTR_DST,
    //! SIP Message Call-ID header
    SIP_ATTR_CALLID,
    //! SIP Message X-Call-ID or X-CID header
    SIP_ATTR_XCALLID,
    //! SIP Message timestamp
    SIP_ATTR_TIME,
    //! SIP Message Method or Response code
    SIP_ATTR_METHOD,
    //! SIP Message
    SIP_ATTR_REQUEST,
    //! SIP Call first message method
    SIP_ATTR_STARTING,
    //! SIP Call message counter
    SIP_ATTR_MSGCNT,
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
struct sip_attr_hdr
{
    //! Attribute id
    enum sip_attr_id id;
    //! Attribute name
    char *name;
    //! Attribute description
    char *desc;
};

/**
 * @brief Attribute data structure
 *
 * This structure contains a single attribute value and acts
 * as a linked list for all attributes of a message (or call)
 * Right now, all the attributed are stored as strings, which may
 * not be the better option, but will fit our actual needs.
 */
struct sip_attr
{
    //! Attribute header pointer
    sip_attr_hdr_t *hdr;
    //! Attribute value
    const char *value;
    //! Next attribute in the linked list
    sip_attr_t *next;
};

/**
 * @brief Information of a single message withing a dialog.
 *
 * Most of the data is just stored to be displayed in the UI so
 * the formats may be no the best, but the simplest for this
 * purpose. It also works as a linked lists of messages in a
 * call.
 *
 */
struct sip_msg
{
    //! Message attribute list
    sip_attr_t *attrs;
    //! Timestamp of current message
    struct timeval ts;
    //! Temporal header data before being parsed
    char *headerptr;
    //! Temporal payload data before being parsed
    char *payloadptr;
    //! FIXME Payload in one struct
    const char *payload[80];
    //!! FIXME not required
    int plines;
    //! Flag to mark if payload data has been parsed
    int parsed;
    //! Message owner
    sip_call_t *call;
    //! Messages linked list
    sip_msg_t *next;
};

/**
 * @brief Contains all information of a call and its messages
 *
 * This structure acts as header of messages list of the same
 * callid (considered a dialog). It contains some replicated
 * data from its messages to speed up searches.
 *
 */
struct sip_call
{
    //! Call attribute list
    sip_attr_t *attrs;
    //! List of messages of this call
    sip_msg_t *msgs;
    // Call Lock
    pthread_mutex_t lock;
    //! Calls double linked list
    sip_call_t *next, *prev;
};

/**
 * @brief Create a new message from the readed header and payload
 *
 * Allocate required memory for a new SIP message. This function
 * will only store the given information, but wont parse it until
 * needed.
 *
 * @param header Raw header text
 * @param payload Raw payload content
 * @return a new allocated message
 */
sip_msg_t *
sip_msg_create(const char *header, const char *payload);

/**
 * @brief Create a new call with the given callid (Minimum required data)
 *
 * Allocated required memory for a new SIP Call. The call acts as
 * header structure to all the messages with the same callid.
 *
 * @param callid Call-ID Header value
 * @return pointer to the sip_call created
 */
extern sip_call_t *
sip_call_create(char *callid);

/**
 * @brief Parses Call-ID header of a SIP message payload
 *
 * Mainly used to check if a payload contains a callid.
 *
 * @param payload SIP message payload
 * @return callid parsed from Call-ID header
 */
extern char *
sip_get_callid(const char* payload);

/**
 * @brief Loads a new message from raw header/payload
 *
 * Use this function to convert raw data into call and message
 * structures. This is mainly used to load data from a file or
 *
 * @param header Raw ngrep header
 * @param payload Raw ngrep payload
 * @return a SIP msg structure pointer
 */
extern sip_msg_t *
sip_load_message(const char *header, const char *payload);

/**
 * @brief Getter for calls linked list size
 *
 * @return how many calls are linked in the list
 */
extern int
sip_calls_count();

/**
 * @brief Check if this call is affected by filters
 *
 * This function is internally used to check if the call should not
 * be returned by general getters because is filtered.
 *
 * @param call Call to check
 * @return 1 if call is filtered, 0 otherwise
 */
extern int
sip_check_call_ignore(sip_call_t *call);

/**
 * @brief Get the header information of an Attribute
 *
 * Retrieve header data from attribute list
 *
 * @param id Attribute id
 * @return Attribute header data structure pointer
 */
extern sip_attr_hdr_t *
sip_attr_get_header(enum sip_attr_id id);

/**
 * @brief Get Attribute description
 *
 * Retrieve description of given attribute from its
 * header structure.
 *
 * @param id Attribut id
 * @return Attribute description from its header
 */
extern const char *
sip_attr_get_description(enum sip_attr_id id);

/**
 * @brief Get Attribute name
 *
 * Retrieve name of given attribute from its
 * header structure.
 *
 * @param id Attribut id
 * @return Attribute name from its header
 */
extern const char *
sip_attr_get_name(enum sip_attr_id id);

/**
 * @brief Get Attribute id from its name
 *
 * Retrieve attribute id of the given attribute name.
 *
 * @param name Attribut name
 * @return Attribute id or 0 if not found
 */
extern enum sip_attr_id
sip_attr_from_name(const char *name);

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
extern void
sip_attr_set(sip_attr_t **list, enum sip_attr_id id, const char *value);

/**
 * @brief Gets the given attribute value to an attribute
 *
 * Primitive for getting an attribute value of a given attribute list.
 * This can be used for calls and message attributes.
 *
 */
extern const char *
sip_attr_get(sip_attr_t *list, enum sip_attr_id id);

/**
 * @brief Append message to the call's message list
 *
 * Creates a relation between this call and the message, appending it
 * to the end of the message list and setting the message owner.
 *
 * @param call pointer to the call owner of the message
 * @param msg SIP message structure
 */
extern void
call_add_message(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Find a call structure in calls linked list given an callid
 *
 *
 *
 * @param callid Call-ID Header value
 * @return pointer to the sip_call structure found or NULL
 */
extern sip_call_t *
call_find_by_callid(const char *callid);

/**
 * @brief Find a call structure in calls linked list given an xcallid
 *
 * Find the call that have the xcallid attribute equal tot he given
 * value.
 *
 * @param xcallid X-Call-ID or X-CID Header value
 * @return pointer to the sip_call structure found or NULL
 */
extern sip_call_t *
call_find_by_xcallid(const char *xcallid);

/**
 * @brief Getter for call messages linked list size
 *
 * Return the number of messages stored in this call. All messages
 * share the same Call-ID
 *
 * @param call SIP call structure
 * @return how many messages are in the call
 */
extern int
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
extern sip_call_t *
call_get_xcall(sip_call_t *call);

/**
 * @brief Finds the next msg in a call.
 *
 * If the passed msg is NULL it returns the first message
 * in the call
 *
 * @param call SIP call structure
 * @param msg Actual SIP msg from the call (can be NULL)
 * @return Next chronological message in the call
 */
extern sip_msg_t *
call_get_next_msg(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Finds the next msg in call and it's extended.
 *
 * If the passed msg is NULL it returns the first message
 * in the conversation
 *
 * @param call SIP call structure
 * @param msg Actual SIP msg from the call (can be NULL)
 * @return Next chronological message in the conversation
 *
 */
extern sip_msg_t *
call_get_next_msg_ex(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Get next call after applying filters and ignores
 *
 * General getter for call list. Never access calls list
 * directly, use this instead.
 *
 * @param cur Current call. Pass NULL to get the first call.
 * @return Next call in the list or NULL if there is no next call
 */
extern sip_call_t *
call_get_next(sip_call_t *cur);

/**
 * @brief Get previous call after applying filters and ignores
 *
 * General getter for call list. Never access calls list
 * directly, use this instead.
 *
 * @param cur Current call
 * @return Prev call in the list or NULL if there is no previous call
 */
extern sip_call_t *
call_get_prev(sip_call_t *cur);

/**
 * @brief Sets the attribute value for a given call
 *
 * This function acts as wrapper of sip call attributes
 *
 * @param call SIP call structure
 * @param id Attribute id
 * @param value Attribute value
 */
extern void
call_set_attribute(sip_call_t *call, enum sip_attr_id id, const char *value);

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
extern const char *
call_get_attribute(sip_call_t *call, enum sip_attr_id id);

/**
 * @brief Parse ngrep header line to get timestamps and ip addresses
 *
 * This function will convert the ngrep header line in format:
 *  U DD/MM/YY hh:mm:ss.uuuuuu fff.fff.fff.fff:pppp -> fff.fff.fff.fff:pppp
 *
 * to some attributes.
 *
 * @todo This MUST disappear someday.
 *
 * @param msg SIP message structure
 * @param header ngrep header generated by -qpt arguments
 * @return 0 on success, 1 on malformed header
 */
extern int
msg_parse_header(sip_msg_t *msg, const char *header);

/**
 * @brief Parse SIP Message payload to fill sip_msg structe
 *
 * Parse the payload content to set message attributes.
 *
 * @param msg SIP message structure
 * @param payload SIP message payload
 * @return 0 in all cases
 */
extern int
msg_parse_payload(sip_msg_t *msg, const char *payload);

/**
 * @brief Parse internal header and payload
 *
 * By default, only the first message of each call is parsed.
 * This function will parse the message (if it's not already parsed)
 * filling all internal fields.
 *
 * @param msg Not Parsed (or parsed) message
 * @return a parsed message
 */
extern sip_msg_t *
msg_parse(sip_msg_t *msg);

/**
 * @brief Sets the attribute value for a given message
 *
 * This function acts as wrapper of sip message attributes
 *
 * @param msg SIP message structure
 * @param id Attribute id
 * @param value Attribute value
 */
extern void
msg_set_attribute(sip_msg_t *msg, enum sip_attr_id id, const char *value);


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
extern const char *
msg_get_attribute(sip_msg_t *msg, enum sip_attr_id id);

#endif
