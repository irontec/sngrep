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

#include <stdbool.h>
#include <glib.h>
#include "sip_call.h"

#define MAX_SIP_PAYLOAD 10240

//! Shorter declaration of sip_call_list structure
typedef struct sip_call_list sip_call_list_t;

//! Shorter declaration of sip stats
typedef struct sip_stats sip_stats_t;
//! Shorter declaration of sip sort
typedef struct sip_sort sip_sort_t;
//! Shorter declaration of structs
typedef struct _SStorageSortOpts    SStorageSortOpts;
typedef struct _SStorageMatchOpts   SStorageMatchOpts;
typedef struct _SStorageCaptureOpts SStorageCaptureOpts;

//! Return values for sip_validate_packet
enum validate_result {
    VALIDATE_NOT_SIP        = -1,
    VALIDATE_PARTIAL_SIP    = 0,
    VALIDATE_COMPLETE_SIP   = 1,
    VALIDATE_MULTIPLE_SIP   = 2
};

struct _SStorageSortOpts {
    //! Sort call list by this attribute
    enum sip_attr_id by;
    //! Sory by attribute ascending
    gboolean asc;
};

struct _SStorageMatchOpts {
    //! Only store dialogs starting with INVITE
    gboolean invite;
    //! Only store dialogs starting with a Method without to-tag
    gboolean complete;
    //! Match expression text
    gchar *mexpr;
    //! Invert match expression result
    gboolean minvert;
    //! Ignore case while matching
    gboolean micase;
    //! Compiled match expression
    GRegex *mregex;
};

struct _SStorageCaptureOpts {
    //! Max number of calls in the list
    guint limit;
    //! Rotate first call when the limit is reached
    gboolean rotate;
    //! Keep captured RTP packets
    gboolean rtp;
    //! Save all stored packets in file
    gchar *outfile;
};

/**
 * @brief Structure to store dialog stats
 */
struct sip_stats
{
    //! Total number of captured dialogs
    int total;
    //! Total number of displayed dialogs after filtering
    int displayed;
};

/**
 * @brief Sorting information for the sip list
 */
struct sip_sort
{
    //! Sort call list by this attribute
    enum sip_attr_id by;
    //! Sory by attribute ascending
    bool asc;
};

/**
 * @brief call structures head list
 *
 * This structure acts as header of calls list
 */
struct sip_call_list {
    // Matching options
    struct _SStorageMatchOpts match;
    // Capture options
    struct _SStorageCaptureOpts capture;
    //! Sort call list following this options
    struct _SStorageSortOpts sort;
    //! List of all captured calls
    GSequence *list;
    //! List of active captured calls
    GSequence *active;
    //! Changed flag. For interface optimal updates
    bool changed;
    //! Last created id
    int last_index;
    //! Call-Ids hash table
    GHashTable *callids;
};

/**
 * @brief Get Capture domain struct for GError
 */
GQuark
capture_pcap_error_quark();

/**
 * @brief Initialize SIP Storage structures
 *
 * @param limit Max number of Stored calls
 * @param only_calls only parse dialogs starting with INVITE
 * @param no_incomplete only parse dialog starting with some methods
 */
gboolean
sip_init(SStorageCaptureOpts capture_options,
         SStorageMatchOpts match_options,
         SStorageSortOpts sort_options,
         GError **error);

/**
 * @brief Deallocate all memory used for SIP calls
 */
void
sip_deinit();

SStorageCaptureOpts
storage_capture_options();

/**
 * @brief Loads a new message from raw header/payload
 *
 * Use this function to convert raw data into call and message
 * structures. This is mainly used to load data from a file or
 *
 * @param packet Packet structure pointer
 * @return a SIP msg structure pointer
 */
sip_msg_t *
sip_check_packet(Packet *packet);

/**
 * @brief Return if the call list has changed
 *
 * Check if the call list has changed since the last time
 * this function was invoked. We consider list has changed when a new
 * call has been added or removed.
 *
 * @return true if list has changed, false otherwise
 */
bool
sip_calls_has_changed();

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
GSequenceIter *
sip_calls_iterator();

/**
 * @brief Return an iterator of call list
 *
 * We consider 'active' calls those that are willing to have
 * an rtp stream that will receive new packets.
 *
 */
GSequenceIter *
sip_active_calls_iterator();

/**
 * @brief Return if a call is in active's call vector
 *
 * @param call Call to be searched
 * @return TRUE if call is active, FALSE otherwise
 */
bool
sip_call_is_active(sip_call_t *call);

/**
 * @brief Return the call list
 */
GSequence *
sip_calls_vector();

/**
 * @brief Return the active call list
 */
GSequence *
sip_active_calls_vector();

/**
 * @brief Return stats from call list
 *
 * @param total Total calls processed
 * @param displayed number of calls matching filters
 */
sip_stats_t
sip_calls_stats();


/**
 * @brief Find a call structure in calls linked list given a call index
 *
 * @param index Position of the call in the calls vector
 * @return pointer to the sip_call structure found or NULL
 */
sip_call_t *
sip_find_by_index(int index);

/**
 * @brief Find a call structure in calls linked list given an callid
 *
 * @param callid Call-ID Header value
 * @return pointer to the sip_call structure found or NULL
 */
sip_call_t *
sip_find_by_callid(const char *callid);

/**
 * @brief Remove al calls
 *
 * This funtion will clear the call list invoking the destroy
 * function for each one.
 */
void
sip_calls_clear();

/**
 * @brief Remove al calls
 *
 * This funtion will clear the call list of calls other than ones
 * fitting the current filter
 */
void
sip_calls_clear_soft();

/**
 * @brief Remove first call in the call list
 *
 * This function removes the first call in the calls vector avoiding
 * reaching the capture limit.
 */
void
sip_calls_rotate();

/**
 * @brief Get full Response code (including text)
 *
 *
 */
const char *
sip_get_msg_reqresp_str(sip_msg_t *msg);

/**
 * @brief Parse SIP Message payload for SDP media streams
 *
 * Parse the payload content to get SDP information
 *
 * @param msg SIP message structure
 * @return 0 in all cases
 */
void
sip_parse_msg_media(sip_msg_t *msg);

/**
 * @brief Get Capture Matching expression
 *
 * @return String containing matching expression
 */
const char *
sip_get_match_expression();

/**
 * @brief Checks if a given payload matches expression
 *
 * @param payload Packet payload
 * @return 1 if matches, 0 otherwise
 */
int
sip_check_match_expression(const char *payload);

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

void
sip_set_sort_options(SStorageSortOpts sort);

SStorageSortOpts
sip_sort_options();

void
sip_sort_list();

gint
sip_list_sorter(gconstpointer a, gconstpointer b, gpointer user_data);

#endif
