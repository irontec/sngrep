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
 * @file capture_pcap.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage pcap files
 *
 * sngrep can parse a pcap file to display call flows.
 * This file include the functions that uses libpcap to do so.
 *
 */
#ifndef __SNGREP_CAPTURE_PCAP_H__
#define __SNGREP_CAPTURE_PCAP_H__

#include <glib.h>
#include <pcap.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "capture/capture_input.h"
#include "capture/capture_output.h"
#include "parser/address.h"

#ifndef __FAVOR_BSD
#define __FAVOR_BSD
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif

#if defined(BSD) || defined (__OpenBSD__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#endif

#include "parser/packet.h"
#include "capture.h"

G_BEGIN_DECLS

#define CAPTURE_TYPE_INPUT_PCAP capture_input_pcap_get_type()
#define CAPTURE_TYPE_OUTPUT_PCAP capture_output_pcap_get_type()

G_DECLARE_FINAL_TYPE(CaptureInputPcap, capture_input_pcap, CAPTURE, INPUT_PCAP, CaptureInput)

G_DECLARE_FINAL_TYPE(CaptureOutputPcap, capture_output_pcap, CAPTURE, OUTPUT_PCAP, CaptureOutput)

//! Max allowed packet length (for libpcap)
#define MAXIMUM_SNAPLEN 262144
//! Error reporting
#define CAPTURE_PCAP_ERROR (capture_pcap_error_quark())

//! Error codes
typedef enum
{
    CAPTURE_PCAP_ERROR_DEVICE_LOOKUP = 0,
    CAPTURE_PCAP_ERROR_DEVICE_OPEN,
    CAPTURE_PCAP_ERROR_FILE_OPEN,
    CAPTURE_PCAP_ERROR_UNKNOWN_LINK,
    CAPTURE_PCAP_ERROR_FILTER_COMPILE,
    CAPTURE_PCAP_ERROR_FILTER_APPLY,
    CAPTURE_PCAP_ERROR_SAVE_NOT_PCAP,
    CAPTURE_PCAP_ERROR_DUMP_OPEN
} CapturePcapErrors;

/**
 * @brief store all information related with input capture
 */
struct _CaptureInputPcap
{
    //! Parent object attributes
    CaptureInput parent;
    //! libpcap capture handler
    pcap_t *handle;
    //! Netmask of our sniffing device
    bpf_u_int32 mask;
    //! The IP of our sniffing device
    bpf_u_int32 net;
    //! libpcap link type
    gint link;
};

/**
 * @brief store all information related with output capture
 */
struct _CaptureOutputPcap
{
    //! Parent object attributes
    CaptureOutput parent;
    //! libpcap capture handler
    pcap_t *handle;
    //! libpcap dumper for capture outputs
    pcap_dumper_t *dumper;
    //! Netmask of our sniffing device
    bpf_u_int32 mask;
    //! The IP of our sniffing device
    bpf_u_int32 net;
    //! libpcap link type
    gint link;
};

/**
 * @brief Get Capture domain struct for GError
 */
GQuark
capture_pcap_error_quark();

/**
 * @brief Online capture function
 *
 * @param device Device to start capture from
 * @param error GError with failure description (optional)
 *
 * @return capture input struct pointer or NULL on failure
 */
CaptureInput *
capture_input_pcap_online(const gchar *dev, GError **error);

/**
 * @brief Read from pcap file and fill sngrep sctuctures
 *
 * @param infile File to read packets from
 * @param error GError with failure description (optional)
 *
 * @return input struct pointer or NULL on failure
 */
CaptureInput *
capture_input_pcap_offline(const gchar *infile, GError **error);

/**
 * @brief Return datalink type of this capture input
 */
gint
capture_input_pcap_datalink(CaptureInput *input);

#ifdef WITH_SSL

/**
 * @brief Set a capture input keyfile for TLS decrypt
 */
void
capture_input_pcap_set_keyfile(CaptureInput *input, const gchar *keyfile);

#endif

/**
 * @brief Get Input file from Offline mode
 *
 * @return Input file in Offline mode
 * @return NULL in Online mode
 */
const gchar *
capture_input_pcap_file(CaptureManager *manager);

/**
 * @brief Get Device interface from Online mode
 *
 * @return Device name used to capture packets
 * @return NULL in Offline or Mixed mode
 */
const gchar *
capture_input_pcap_device(CaptureManager *manager);

/**
 * @brief Open a new dumper file for capture handler
 */
CaptureOutput *
capture_output_pcap(const gchar *filename, GError **error);

#endif /* __SNGREP_CAPTURE_PCAP_H__ */
