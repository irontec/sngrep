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

#include "config.h"
#include <pcap.h>
#include <sys/time.h>
#include <pthread.h>
#include <arpa/inet.h>
#ifdef WITH_PCRE
#include <pcre.h>
#endif
#include <regex.h>
#include "sip_attr.h"

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

//! SIP Call State
#define SIP_CALLSTATE_CALLSETUP "CALL SETUP"
#define SIP_CALLSTATE_INCALL    "IN CALL"
#define SIP_CALLSTATE_CANCELLED "CANCELLED"
#define SIP_CALLSTATE_REJECTED  "REJECTED"
#define SIP_CALLSTATE_COMPLETED "COMPLETED"

//! Shorter declaration of sip_call structure
typedef struct sip_call sip_call_t;
//! Shorter declaration of sip_msg structure
typedef struct sip_msg sip_msg_t;
//! Shorter declaration of sip_call_list structure
typedef struct sip_call_list sip_call_list_t;

/**
 * @brief Information of a single message withing a dialog.
 *
 * Most of the data is just stored to be displayed in the UI so
 * the formats may be no the best, but the simplest for this
 * purpose. It also works as a linked lists of messages in a
 * call.
 */
struct sip_msg {
    //! Message attribute list
    char *attrs[SIP_ATTR_SENTINEL];
    //! Source address
    char src[50];
    //! Source port
    u_short sport;
    //! Destination address
    char dst[50];
    //! Destination port
    u_short dport;
    //! Temporal payload data before being parsed
    char *payload;
    //! Flag to determine if message payload has been parsed
    int parsed;
    //! Color for this message (in color.cseq mode)
    int color;
    //! Request Method or Response Code @see sip_methods
    int reqresp;
    //! This message contains sdp data
    int sdp;
    //! Message RTP position indicator
    int rtp_pos;
    //! Message RTP packet count
    int rtp_count;
    //! Message Cseq
    int cseq;
    //! PCAP Packet Header data
    struct pcap_pkthdr *pcap_header;
    //! PCAP Packet data
    u_char *pcap_packet;
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
 */
struct sip_call {
    //! Call attribute list
    char *attrs[SIP_ATTR_SENTINEL];
    //! Flag this call as filtered so won't be displayed
    int filtered;
    //! List of messages of this call
    sip_msg_t *msgs;
    //! Pointer to the last added message
    sip_msg_t *last_msg;
    //! How many messages has this call
    int msgcnt;
    //! Message when conversation started
    sip_msg_t *cstart_msg;
    //! Calls double linked list
    sip_call_t *next, *prev;
};

/**
 * @brief call structures head list
 *
 * This structure acts as header of calls list
 */
struct sip_call_list {
    // First and Last calls of the list
    sip_call_t *first;
    sip_call_t *last;
    // Call counter
    int count;
    // Max call limit
    int limit;
    //! Only store dialogs starting with INVITE
    int only_calls;
    //! Only store dialogs starting with some Methods
    int ignore_incomplete;
    //! match expression text
    const char *match_expr;
#ifdef WITH_PCRE
    //! Compiled match expression
    pcre *match_regex;
#else
    //! Compiled match expression
    regex_t match_regex;
#endif
    //! Invert match expression result
    int match_invert;

    //! Regexp for payload matching
    regex_t reg_method;
    regex_t reg_callid;
    regex_t reg_xcallid;
    regex_t reg_response;
    regex_t reg_cseq;
    regex_t reg_from;
    regex_t reg_to;
    regex_t reg_sdp_addr;
    regex_t reg_sdp_port;

    // Warranty thread-safe access to the calls list
    pthread_mutex_t lock;
};

/**
 * @brief Initialize SIP Storage structures
 *
 * @param limit Max number of Stored calls
 * @param only_calls only parse dialogs starting with INVITE
 * @param no_incomplete only parse dialog starting with some methods
 */
void
sip_init(int limit, int only_calls, int no_incomplete);


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
sip_msg_create(const char *payload);

/**
 * @brief Destroy a SIP message and free its memory
 *
 * Deallocate memory of an existing SIP Message.
 * This function will remove the message from the call and the
 * passed pointer will be NULL.
 *
 * @param msg SIP message to be deleted
 */
void
sip_msg_destroy(sip_msg_t *msg);

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
sip_call_create(char *callid);

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
sip_call_destroy(sip_call_t *call);

/**
 * @brief Parses Call-ID header of a SIP message payload
 *
 * Mainly used to check if a payload contains a callid.
 *
 * @param payload SIP message payload
 * @return callid parsed from Call-ID header
 */
char *
sip_get_callid(const char* payload);

/**
 * @brief Loads a new message from raw header/payload
 *
 * Use this function to convert raw data into call and message
 * structures. This is mainly used to load data from a file or
 *
 * @todo This functions should stop using ngrep header format
 *
 * @param header Raw ngrep header
 * @param payload Raw ngrep payload
 * @return a SIP msg structure pointer
 */
sip_msg_t *
sip_load_message(const struct pcap_pkthdr *header, const char *src, u_short sport,
                 const char *dst, u_short dport, u_char *payload);

/**
 * @brief Getter for calls linked list size
 *
 * @return how many calls are linked in the list
 */
int
sip_calls_count();

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
 * @brief Find a call structure in calls linked list given an callid
 *
 *
 *
 * @param callid Call-ID Header value
 * @return pointer to the sip_call structure found or NULL
 */
sip_call_t *
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
sip_call_t *
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
sip_msg_t *
call_get_next_msg(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Finds the prev msg in a call.
 *
 * If the passed msg is the first message in the call
 * this function will return NULL
 *
 * @param call SIP call structure
 * @param msg Actual SIP msg from the call
 * @return Previous chronological message in the call
 */
sip_msg_t *
call_get_prev_msg(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Get next call
 *
 * General getter for call list. Never access calls list
 * directly, use this instead.
 *
 * @param cur Current call. Pass NULL to get the first call.
 * @return Next call in the list or NULL if there is no next call
 */
sip_call_t *
call_get_next(sip_call_t *cur);

/**
 * @brief Get previous call
 *
 * General getter for call list. Never access calls list
 * directly, use this instead.
 *
 * @param cur Current call
 * @return Prev call in the list or NULL if there is no previous call
 */
sip_call_t *
call_get_prev(sip_call_t *cur);

/**
 * @brief Get next call after applying filters and ignores
 *
 * @param cur Current call. Pass NULL to get the first call.
 * @return Next call in the list or NULL if there is no next call
 */
sip_call_t *
call_get_next_filtered(sip_call_t *cur);

/**
 * @brief Get previous call applying filters and ignores
 *
 * General getter for call list. Never access calls list
 * directly, use this instead.
 *
 * @param cur Current call
 * @return Prev call in the list or NULL if there is no previous call
 */
sip_call_t *
call_get_prev_filtered(sip_call_t *cur);

/**
 * @brief Update Call State attribute with its last parsed message
 *
 * @param call Call structure to be updated
 * @param msg Last received message of this call
 */
void
call_update_state(sip_call_t *call, sip_msg_t *msg);

/**
 * @brief Get message Request/Response code
 *
 * Parse Payload to get Message Request/Response code.
 *
 * @param msg SIP Message to be parsed
 * @return numeric representation of Request/ResponseCode
 */
int
msg_get_reqresp(sip_msg_t *msg);

/**
 * @brief Parse SIP Message payload to fill sip_msg structe
 *
 * Parse the payload content to set message attributes.
 *
 * @param msg SIP message structure
 * @param payload SIP message payload
 * @return 0 in all cases
 */
int
msg_parse_payload(sip_msg_t *msg, const char *payload);

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
 * @brief Get summary of message header data
 *
 * For raw prints, it's handy to have the ngrep header style message
 * data.
 *
 * @param msg SIP message
 * @param out pointer to allocated memory to contain the header output
 * @returns pointer to out
 */
char *
msg_get_header(sip_msg_t *msg, char *out);

/**
 * @brief Remove al calls
 *
 * This funtion will clear the call list invoking the destroy
 * function for each one.
 */
void
sip_calls_clear();

/**
 * @brief Calculate the time difference between two messages
 *
 * @param start First cronological message
 * @param start Second cronological message
 * @return Human readable time difference in MMM:SS format
 */
const char *
sip_calculate_duration(const sip_msg_t *start, const sip_msg_t *end, char *dur);

/**
 * @brief Set Capture Matching expression
 *
 * @param expr String containing matching expreson
 * @param insensitive 1 for case insensitive matching
 * @param invert 1 for reverse matching
 * @return 0 if expresion is valid, 1 otherwise
 */
int
sip_set_match_expression(const char *expr, int insensitive, int invert);

/**
 * @brief Checks if a given payload matches expression
 *
 * @param payload Packet payload
 * @return 1 if matches, 0 otherwise
 */
int
sip_check_match_expression(const char *payload);

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

/**
 * @brief Get String value for a Method
 *
 * @param method One of the methods defined in @sip_methods
 * @return a string representing the method text
 */
const char *
sip_method_str(enum sip_methods method);

/**
 * @brief Converts Request Name or Response code to number
 *
 * If the argument is a method, the corresponding value of @sip_methods
 * will be returned. If a Resposne code, the numeric value of the code
 * will be returned.
 *
 * @param a string representing the Request/Resposne code text
 * @return numeric representation of Request/Response code
 */
int
sip_method_from_str(const char *method);

/**
 * @brief Return address formatted depending on active settings
 *
 * Addresses can be printed in many formats depending on active settings.
 * Alias, resolving or just printing address as is will be
 *
 * @param address Address in string format
 * @return address formatted
 */
const char *
sip_address_format(const char *address);


/**
 * @brief Return address:port formatted depending on active settings
 *
 * Addresses can be printed in many formats depending on active settings.
 * Alias, resolving or just printing address as is will be
 *
 * @param addrport Address:Port in string format
 * @return address:port formatted
 */
const char *
sip_address_port_format(const char *address);

#endif
