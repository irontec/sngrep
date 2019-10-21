/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
 ** Copyright (C) 2012 Homer Project (http://www.sipcapture.org)
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
 * @file capture_hep.h
 *
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 * @author Alexandr Dubovikov <alexandr.dubovikov@gmail.com>
 *
 * @brief Functions to manage hep protocol
 *
 * This file contains declaration of structure and functions to send and
 * receive packet information through HEP-EEP (Extensible Encapsulation Protocol)
 *
 * Additional information about HEP-EEP protocol can be found in sipcature
 * repositories at https://github.com/sipcapture/HEP
 *
 * @note Most of this code has been taken from hep-c and sipgrep (originally
 * written by Alexandr Dubovikov). Modifications of sources to work with
 * sngrep packet structures has been made by Ivan Alonso (Kaian)
 *
 */
#ifndef __SNGREP_CAPTURE_HEP_H__
#define __SNGREP_CAPTURE_HEP_H__

#include <glib.h>
#include <glib-object.h>
#include "capture.h"
#include "capture_input.h"
#include "capture_output.h"

G_BEGIN_DECLS

#define CAPTURE_TYPE_INPUT_HEP capture_input_hep_get_type()
#define CAPTURE_TYPE_OUTPUT_HEP capture_output_hep_get_type()

G_DECLARE_FINAL_TYPE(CaptureInputHep, capture_input_hep, CAPTURE, INPUT_HEP, CaptureInput)

G_DECLARE_FINAL_TYPE(CaptureOutputHep, capture_output_hep, CAPTURE, OUTPUT_HEP, CaptureOutput)


//! Max allowed packet size
#define MAX_HEP_BUFSIZE 20480
//! Error reporting
#define CAPTURE_HEP_ERROR (capture_hep_error_quark())

//! Error codes
typedef enum
{
    CAPTURE_HEP_ERROR_VERSION,
    CAPTURE_HEP_ERROR_URL_PARSE,
    CAPTURE_HEP_ERROR_SOCKET,
    CAPTURE_HEP_ERROR_BIND,
    CAPTURE_HEP_ERROR_CONNECT
} CaptureHepErrors;

typedef struct _CaptureHepUrl CaptureHepUrl;

/**
 * @brief Hep URL Client/Server data
 */
struct _CaptureHepUrl
{
    //! Protocol
    const gchar *proto;
    //! IP address to send/receive HEP data
    const gchar *host;
    //! Port to send/receive HEP data
    const gchar *port;
};

/**
 * @brief HEP Capture Input data
 */
struct _CaptureInputHep
{
    //! Parent object attributes
    CaptureInput parent;
    //! Client/Server socket
    gint socket;
    //! Capture agent id
    guint16 id;
    //! Capture connection data
    struct _CaptureHepUrl url;
    //! Hep Version for send/receive data (2 or 3)
    gint version;
    //! Password for authentication
    const gchar *password;
};

/**
 * @brief HEP Capture Output data
 */
struct _CaptureOutputHep
{
    //! Parent object attributes
    CaptureOutput parent;
    //! Client/Server socket
    gint socket;
    //! Capture agent id
    guint16 id;
    //! Capture connection data
    struct _CaptureHepUrl url;
    //! Hep Version for send/receive data (2 or 3)
    gint version;
    //! Password for authentication
    const gchar *password;
};

/**
 * @brief Get Capture domain struct for GError
 */
GQuark
capture_hep_error_quark();

/**
 * @brief Generate a new capture input for HEP
 *
 * @param url Listen url in proto:host:ip format
 * @param error GError with failure description (optional)
 *
 * @return capture input struct pointer or NULL on failure
 */
CaptureInput *
capture_input_hep(const gchar *url, GError **error);

/**
 * @brief Start HEP Server Thread
 *
 * This function is used as worker thread for receiving packets from
 * listening socket.
 */
void *
capture_input_hep_start(CaptureInput *input);

/**
 * @brief Stop HEP Server Thread
 *
 * Close HEP server socket and stop capture thread
 */
void
capture_input_hep_stop(CaptureInput *input);

/**
 * @brief Return the local port where HEP packets are received
 *
 * @return Local listen port or NULL if HEP listen mode is not running
 */
const gchar *
capture_input_hep_port(CaptureManager *manager);

/**
 * @brief Create a new Capture output for HEP tech
 *
 * Create a client to send HEP packets to a HEP capable server (like Homer)
 *
 * @param url Server url in proto:host:ip format
 * @param error GError with failure description (optional)
 * @return capture output struct pointer or NULL on failure
 */
CaptureOutput *
capture_output_hep(const gchar *url, GError **error);

/**
 * @brief Send a captured packet
 *
 * Send a packet encapsulated into HEP through the client socket.
 * This function will only handle SIP packets if HEP client mode
 * has been enabled.
 *
 * @param output Capture output data
 * @param pkt Packet Structure data
 */
void
capture_output_hep_write(CaptureOutput *output, Packet *pkt);

/**
 * @brief Close a HEP capture output
 * @param output Capture output data
 */
void
capture_output_hep_close(CaptureOutput *output);

/**
 * @brief Return the remote port where HEP packets are sent
 *
 * @return Remote port or NULL if HEP send mode is not running
 */
const gchar *
capture_output_hep_port(CaptureManager *manager);

G_END_DECLS

#endif /* __SNGREP_CAPTURE_HEP_H__ */
