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
#include <regex.h>
#ifdef WITH_PCRE
#include <pcre.h>
#endif
#include "sip_call.h"
#include "vector.h"

#define MAX_SIP_PAYLOAD 10240

//! Shorter declaration of sip_call_list structure
typedef struct sip_call_list sip_call_list_t;

/**
 * @brief call structures head list
 *
 * This structure acts as header of calls list
 */
struct sip_call_list {
    //! List of all captured calls
    vector_t *list;

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
 * @brief Deallocate all memory used for SIP calls
 */
void
sip_deinit();

/**
 * @brief Parses Call-ID header of a SIP message payload
 *
 * Mainly used to check if a payload contains a callid.
 *
 * @param payload SIP message payload
 * @param callid Character array to store callid
 * @return callid parsed from Call-ID header
 */
char *
sip_get_callid(const char* payload, char *callid);

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
sip_load_message(capture_packet_t *packet, const char *src, u_short sport, const char* dst, u_short dport);

/**
 * @brief Getter for calls linked list size
 *
 * @return how many calls are linked in the list
 */
int
sip_calls_count();

/**
 * @brief Return an iterator of call list
 */
vector_iter_t
sip_calls_iterator();

/**
 * @brief Return stats from call list
 *
 * @param total Total calls processed
 * @param displayed number of calls matching filters
 */
void
sip_calls_stats(int *total, int *displayed);

/**
 * @brief Find a call structure in calls linked list given an callid
 *
 * @param callid Call-ID Header value
 * @return pointer to the sip_call structure found or NULL
 */
sip_call_t *
sip_find_by_callid(const char *callid);

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
sip_find_by_xcallid(const char *xcallid);

/**
 * @brief Remove al calls
 *
 * This funtion will clear the call list invoking the destroy
 * function for each one.
 */
void
sip_calls_clear();

/**
 * @brief Get message Request/Response code
 *
 * Parse Payload to get Message Request/Response code.
 *
 * @param msg SIP Message to be parsed
 * @return numeric representation of Request/ResponseCode
 */
int
sip_get_msg_reqresp(sip_msg_t *msg, const u_char *payload);

/**
 * @brief Parse SIP Message payload if not parsed
 *
 * This function can be used for delayed parsing. This way
 * the message will only use the minimun required memory
 * to store basic information.
 *
 * @param msg SIP message structure
 * @return parsed message
 */
sip_msg_t *
sip_parse_msg(sip_msg_t *msg);

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
sip_parse_msg_payload(sip_msg_t *msg, const u_char *payload);

/**
 * @brief Parse SIP Message payload for SDP media streams
 *
 * Parse the payload content to get SDP information
 *
 * @param msg SIP message structure
 * @return 0 in all cases
 */
void
sip_parse_msg_media(sip_msg_t *msg, const u_char *payload);

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
sip_get_msg_header(sip_msg_t *msg, char *out);

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
