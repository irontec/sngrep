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
 * @file capture.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage captures
 *
 */
#ifndef __SNGREP_CAPTURE_H__
#define __SNGREP_CAPTURE_H__

#include <glib.h>
#include <glib-object.h>
#include "storage/address.h"

G_BEGIN_DECLS

//! Capture modes
typedef enum
{
    CAPTURE_MODE_INVALID = 0,
    CAPTURE_MODE_ONLINE,
    CAPTURE_MODE_OFFLINE,
} CaptureMode;

//! Capture techs
typedef enum
{
    CAPTURE_TECH_INVALID = 0,
    CAPTURE_TECH_PCAP,
    CAPTURE_TECH_HEP,
    CAPTURE_TECH_TXT,
} CaptureTech;

//! Capture function types
typedef struct _CaptureInput CaptureInput;
typedef struct _CaptureOutput CaptureOutput;
typedef struct _CaptureManager CaptureManager;
typedef struct _Packet Packet;

/**
 * @brief Capture common configuration
 *
 * Store capture configuration and global data
 */
struct _CaptureManager
{
    //! Key file for TLS decrypt
    const gchar *keyfile;
    //! capture filter expression text
    gchar *filter;
    //! TLS Server address
    Address *tls_server;
    //! Flag to skip captured packets
    gboolean paused;
    //! Packet capture inputs (CaptureInput *)
    GSList *inputs;
    //! Packet capture outputs (CaptureOutput *)
    GSList *outputs;
    //! Packet main loop thread
    GThread *thread;
    //! Capture Main loop
    GMainLoop *loop;
};


/**
 * @brief Initialize main capture manager
 *
 * Create a new instance of capture manager. Only a single instance of
 * capture manager can exist at the same tame.
 *
 * Use capture_manager to retrieve the global capture manager instance.
 */
CaptureManager *
capture_manager_new();

/**
 * @brief De-initialize capture data
 */
void
capture_manager_free(CaptureManager *manager);

/**
 * @brief Get Global capture manager
 * @internal This method is used for backwards compatibility during refactor.
 *
 * @return pointer to the global capture manager
 */
CaptureManager *
capture_manager_get_instance();

/**
 * @brief Start all capture inputs in given manager
 * @param manager
 * @return
 */
void
capture_manager_start(CaptureManager *manager);

/**
 * @brief Stop all capture inputs in given manager
 * @param manager
 * @return
 */
void
capture_manager_stop(CaptureManager *manager);

/**
 * @brief Get Current loaded percentage
 * @param manager
 * @return Loaded percentage
 */
guint
capture_manager_load_progress(CaptureManager *manager);

/**
 * @brief Set a bpf filter in open capture
 *
 * @param filter String containing the BPF filter text
 * @return TRUE if valid, FALSE otherwise
 */
gboolean
capture_manager_set_filter(CaptureManager *manager, gchar *filter, GError **error);

/**
 * @brief Set keyfile to decrypt TLS packets
 *
 * @param keyfile Full path to keyfile
 */
void
capture_manager_set_keyfile(CaptureManager *manager, const gchar *keyfile, GError **error);

/**
 * @brief Get the Keyfile to decrypt TLS packets
 *
 * @return String containing Keyfile name or NULL
 */
const gchar *
capture_manager_filter(CaptureManager *manager);

/**
 * @brief Add a new Capture Input to the given manager
 */
void
capture_manager_add_input(CaptureManager *manager, CaptureInput *input);

/**
 * @brief Add a new Capture Output to the given manager
 */
void
capture_manager_add_output(CaptureManager *manager, CaptureOutput *output);

/**
 * @brief Store the given packet in call outputs
 */
void
capture_manager_output_packet(CaptureManager *manager, Packet *packet);

/**
 * @brief Return a string representing current capture status
 */
const gchar *
capture_status_desc(CaptureManager *manager);

/**
 * @brief Get Key file from decrypting TLS packets
 *
 * @return given keyfile
 */
const gchar *
capture_keyfile(CaptureManager *manager);

/**
 * @brief Check if capture is in Online mode
 *
 * @return TRUE if capture is online, FALSE if offline
 */
gboolean
capture_is_online(CaptureManager *manager);

/**
 * @brief Get TLS Server address if configured
 * @return address structure pointer
 */
Address *
capture_tls_server(CaptureManager *manager);

/**
 * @brief Set pause status in given capture manager
 */
void
capture_manager_set_pause(CaptureManager *manager, gboolean paused);

/**
 * @brief Determine if any of the capture inputs is running
 * @return TRUE if at least one Input is running
 */
gboolean
capture_is_running();

/**
 * @brief Determine if any of the capture inputs is paused by user
 * @return TRUE if user paused capture
 */
gboolean
capture_is_paused();

G_END_DECLS

#endif  /* __SNGREP_CAPTURE_H__ */
