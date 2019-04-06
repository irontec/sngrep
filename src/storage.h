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
 * @file storage.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage calls, messages and streams Storage
 *
 */

#ifndef __SNGREP_STORAGE_H
#define __SNGREP_STORAGE_H

#include <glib.h>
#include "call.h"

#define MAX_SIP_PAYLOAD 10240

//! Shorter declaration of sip_call_list structure
typedef struct _Storage Storage;
//! Shorter declaration of sip stats
typedef struct _StorageStats StorageStats;

//! Shorter declaration of structs
typedef struct _StorageSortOpts StorageSortOpts;
typedef struct _StorageMatchOpts StorageMatchOpts;
typedef struct _StorageCaptureOpts StorageCaptureOpts;

struct _StorageSortOpts
{
    //! Sort call list by this attribute
    enum AttributeId by;
    //! Sory by attribute ascending
    gboolean asc;
};

struct _StorageMatchOpts
{
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

struct _StorageCaptureOpts
{
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
struct _StorageStats
{
    //! Total number of captured dialogs
    guint total;
    //! Total number of displayed dialogs after filtering
    guint displayed;
};

/**
 * @brief call structures head list
 *
 * This structure acts as header of calls list
 */
struct _Storage
{
    //! Matching options
    StorageMatchOpts match;
    //! Capture options
    StorageCaptureOpts capture;
    //! Sort call list following this options
    StorageSortOpts sort;
    //! List of all captured calls
    GPtrArray *calls;
    //! Changed flag. For interface optimal updates
    gboolean changed;
    //! Last created id
    guint last_index;
    //! Call-Ids hash table
    GHashTable *callids;
    //! Streams hash table
    GHashTable *streams;
    //! Storage thread
    guint source;
};

gpointer
storage_check_packet(Packet *packet);

/**
 * @brief Initialize SIP Storage structures
 *
 * FIXME This function initializes storage structure and starts storage
 * thread.
 */
gboolean
storage_init(StorageCaptureOpts capture_options,
             StorageMatchOpts match_options,
             StorageSortOpts sort_options,
             GError **error);

/**
 * @brief Deallocate all memory used for SIP calls
 *
 * Stop storage thread and free its memory
 */
void
storage_deinit();

/**
 * @brief Return if the call list has changed
 *
 * Check if the call list has changed since the last time
 * this function was invoked. We consider list has changed when a new
 * call has been added or removed.
 *
 * @return true if list has changed, false otherwise
 */
gboolean
storage_calls_changed();

/**
 * @brief Getter for calls linked list size
 *
 * @return how many calls are linked in the list
 */
guint
storage_calls_count();

/**
 * @brief Return the call list
 */
GPtrArray *
storage_calls();


/**
 * @brief Return stats from call list
 *
 * @param total Total calls processed
 * @param displayed number of calls matching filters
 */
StorageStats
storage_calls_stats();

/**
 * @brief Remove al calls
 *
 * This function will clear the call list invoking the destroy
 * function for each one.
 */
void
storage_calls_clear();

/**
 * @brief Remove al calls
 *
 * This function will clear the call list of calls other than ones
 * fitting the current filter
 */
void
storage_calls_clear_soft();

/**
 * @brief Get Storage Matching options
 *
 * @return Struct containing matching options
 */
StorageMatchOpts
storage_match_options();

/**
 * @brief Get Storage Sorting options
 *
 * @return Struct containing sorting options
 */
StorageSortOpts
storage_sort_options();

/**
 * @brief Get Storage Capture options
 *
 * @return Struct containing capture options
 */
StorageCaptureOpts
storage_capture_options();

/**
 * @brief Set Storage Sorting options
 *
 * @param sort Struct with sorting information
 */
void
storage_set_sort_options(StorageSortOpts sort);

/**
 * @brief Return queued packets to be checked count
 *
 * @return Pending packets current length
 */
gint
storage_pending_packets();

#endif /* __SNGREP_STORAGE_H */
