/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
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
 * @file capture.h
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
#ifndef __SNGREP_CAPTURE_HEP_H
#define __SNGREP_CAPTURE_HEP_H
#include <glib.h>

//! Error reporting
#define CAPTURE_HEP_ERROR (capture_hep_error_quark())

//! Error codes
enum capture_hep_errors
{
    CAPTURE_HEP_ERROR_VERSION,
    CAPTURE_HEP_ERROR_URL_PARSE,
    CAPTURE_HEP_ERROR_SOCKET,
    CAPTURE_HEP_ERROR_BIND,
    CAPTURE_HEP_ERROR_CONNECT
};

typedef struct _CaptureHep              CaptureHep;
typedef struct _CaptureHepUrl           CaptureHepUrl;
typedef struct _CaptureHepChunk         CaptureHepChunk;
typedef struct _CaptureHepChunkIp4      CaptureHepChunkIp4;
typedef struct _CaptureHepChunkIp6      CaptureHepChunkIp6;

/**
 * @brief Hep URL Cliend/Server data
 */
struct _CaptureHepUrl {
    //! Protocol
    const gchar *proto;
    //! IP address to send/receive HEP data
    const gchar *host;
    //! Port to send/receive HEP data
    const gchar *port;
};

/**
 * @brief HEP Capture Input/Output private data
 */
struct _CaptureHep {
    //! Client/Server socket
    int socket;
    //! Capture agent id
    guint16 id;
    //! Capture connection data
    struct _CaptureHepUrl url;
    //! Hep Version for send/receive data (2 or 3)
    gint version;
    //! Password for authentication
    const gchar *password;
};

/* HEPv3 types */
struct _CaptureHepChunk {
    guint16 vendor_id;
    guint16 type_id;
    guint16 length;
}__attribute__((packed));


struct _CaptureHepChunkUint8 {
    struct _CaptureHepChunk chunk;
    guint8 data;
}__attribute__((packed));


struct _CaptureHepChunkUint16 {
    struct _CaptureHepChunk chunk;
    guint16 data;
}__attribute__((packed));


struct _CaptureHepChunkUint32 {
    struct _CaptureHepChunk chunk;
    guint32 data;
}__attribute__((packed));

struct _CaptureHepChunkIp4 {
    struct _CaptureHepChunk chunk;
    struct in_addr data;
}__attribute__((packed));

struct _CaptureHepChunkIp6 {
    struct _CaptureHepChunk chunk;
    struct in6_addr data;
}__attribute__((packed));


struct _CaptureHepCtrl {
    gchar id[4];
    guint16 length;
}__attribute__((packed));

struct CaptureHepGeneric {
    struct _CaptureHepCtrl header;
    struct _CaptureHepChunkUint8 ip_family;
    struct _CaptureHepChunkUint8 ip_proto;
    struct _CaptureHepChunkUint16 src_port;
    struct _CaptureHepChunkUint16 dst_port;
    struct _CaptureHepChunkUint32 time_sec;
    struct _CaptureHepChunkUint32 time_usec;
    struct _CaptureHepChunkUint8 proto_t;
    struct _CaptureHepChunkUint32 capt_id;
}__attribute__((packed));

struct _CaptureHepHdr {
    guint8 hp_v;            /* version */
    guint8 hp_l;            /* length */
    guint8 hp_f;            /* family */
    guint8 hp_p;            /* protocol */
    guint16 hp_sport;       /* source port */
    guint16 hp_dport;       /* destination port */
};

struct _CaptureHepTimeHdr {
    guint32 tv_sec;         /* seconds */
    guint32 tv_usec;        /* useconds */
    guint16 captid;         /* Capture ID node */
};

struct _CaptureHepIpHdr {
    struct in_addr hp_src;
    struct in_addr hp_dst;  /* source and dest address */
};

#ifdef USE_IPV6
struct _CaptureHepIp6Hdr {
    struct in6_addr hp6_src; /* source address */
    struct in6_addr hp6_dst; /* destination address */
};
#endif

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
void
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
capture_output_hep_write(CaptureOutput *output, packet_t *pkt);

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


#endif /* __SNGREP_CAPTURE_HEP_H */
