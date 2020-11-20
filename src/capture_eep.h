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
 * @brief Functions to manage eep protocol
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
#ifndef __SNGREP_CAPTURE_EEP_H
#define __SNGREP_CAPTURE_EEP_H
#include <pthread.h>
#include "capture.h"

//! HEP chunk types
enum
{
    CAPTURE_EEP_CHUNK_INVALID = 0,
    CAPTURE_EEP_CHUNK_FAMILY,
    CAPTURE_EEP_CHUNK_PROTO,
    CAPTURE_EEP_CHUNK_SRC_IP4,
    CAPTURE_EEP_CHUNK_DST_IP4,
    CAPTURE_EEP_CHUNK_SRC_IP6,
    CAPTURE_EEP_CHUNK_DST_IP6,
    CAPTURE_EEP_CHUNK_SRC_PORT,
    CAPTURE_EEP_CHUNK_DST_PORT,
    CAPTURE_EEP_CHUNK_TS_SEC,
    CAPTURE_EEP_CHUNK_TS_USEC,
    CAPTURE_EEP_CHUNK_PROTO_TYPE,
    CAPTURE_EEP_CHUNK_CAPT_ID,
    CAPTURE_EEP_CHUNK_KEEP_TM,
    CAPTURE_EEP_CHUNK_AUTH_KEY,
    CAPTURE_EEP_CHUNK_PAYLOAD,
    CAPTURE_EEP_CHUNK_CORRELATION_ID
};


//! Shorter declaration of capture_eep_config structure
typedef struct capture_eep_config  capture_eep_config_t;

/**
 * @brief EEP  Client/Server configuration
 */
struct capture_eep_config
{
    //! Client socket for sending EEP data
    int client_sock;
    //! Server socket for receiving EEP data
    int server_sock;
    //! Capture agent id
    int capt_id;
    //! Hep Version for sending data (2 or 3)
    int capt_version;
    //! IP address to sends EEP data
    const char *capt_host;
    //! Port to send EEP data
    const char *capt_port;
    //! Password for authenticate as client
    const char *capt_password;
    // HEp version for receiving data (2 or 3)
    int capt_srv_version;
    //! IP address to received EEP data
    const char *capt_srv_host;
    //! Local oort to receive EEP data
    const char *capt_srv_port;
    //! Server password to authenticate incoming connections
    const char *capt_srv_password;
    //! Server thread to parse incoming data
    pthread_t server_thread;
};

/* HEPv3 types */
struct hep_chunk
{
    u_int16_t vendor_id;
    u_int16_t type_id;
    u_int16_t length;
}__attribute__((packed));

typedef struct hep_chunk hep_chunk_t;

struct hep_chunk_uint8
{
    hep_chunk_t chunk;
    u_int8_t data;
}__attribute__((packed));

typedef struct hep_chunk_uint8 hep_chunk_uint8_t;

struct hep_chunk_uint16
{
    hep_chunk_t chunk;
    u_int16_t data;
}__attribute__((packed));

typedef struct hep_chunk_uint16 hep_chunk_uint16_t;

struct hep_chunk_uint32
{
    hep_chunk_t chunk;
    u_int32_t data;
}__attribute__((packed));

typedef struct hep_chunk_uint32 hep_chunk_uint32_t;

struct hep_chunk_str
{
    hep_chunk_t chunk;
    char *data;
}__attribute__((packed));

typedef struct hep_chunk_str hep_chunk_str_t;

struct hep_chunk_ip4
{
    hep_chunk_t chunk;
    struct in_addr data;
}__attribute__((packed));

typedef struct hep_chunk_ip4 hep_chunk_ip4_t;

struct hep_chunk_ip6
{
    hep_chunk_t chunk;
    struct in6_addr data;
}__attribute__((packed));

typedef struct hep_chunk_ip6 hep_chunk_ip6_t;

struct hep_ctrl
{
    char id[4];
    u_int16_t length;
}__attribute__((packed));

typedef struct hep_ctrl hep_ctrl_t;

struct hep_chunk_payload
{
    hep_chunk_t chunk;
    char *data;
}__attribute__((packed));

typedef struct hep_chunk_payload hep_chunk_payload_t;

/**
 * @brief Generic HEP header
 *
 * All EEP/HEP packets will contain at least this header.
 */
struct hep_generic
{
    hep_ctrl_t header;
    hep_chunk_uint8_t ip_family;
    hep_chunk_uint8_t ip_proto;
    hep_chunk_uint16_t src_port;
    hep_chunk_uint16_t dst_port;
    hep_chunk_uint32_t time_sec;
    hep_chunk_uint32_t time_usec;
    hep_chunk_uint8_t proto_t;
    hep_chunk_uint32_t capt_id;
}__attribute__((packed));

typedef struct hep_generic hep_generic_t;

struct hep_hdr
{
    u_int8_t hp_v;      /* version */
    u_int8_t hp_l;      /* length */
    u_int8_t hp_f;      /* family */
    u_int8_t hp_p;      /* protocol */
    u_int16_t hp_sport; /* source port */
    u_int16_t hp_dport; /* destination port */
};

struct hep_timehdr
{
    u_int32_t tv_sec;   /* seconds */
    u_int32_t tv_usec;  /* useconds */
    u_int16_t captid;   /* Capture ID node */
};

struct hep_iphdr
{
    struct in_addr hp_src;
    struct in_addr hp_dst; /* source and dest address */
};

#ifdef USE_IPV6
struct hep_ip6hdr
{
    struct in6_addr hp6_src; /* source address */
    struct in6_addr hp6_dst; /* destination address */
};
#endif

/**
 * @brief Initialize EEP proccess
 *
 * This funtion will setup all required sockets both for
 * send and receiving information depending on sngrep configuration.
 *
 * It will also launch a thread to received EEP data if configured
 * to do so.
 *
 * @return 1 on any error occurs, 0 otherwise
 */
int
capture_eep_init();

/**
 * @brief Unitialize EEP process
 *
 * Close used sockets for receive and send data and stop server
 * thread if server mode is enabled.
 */
void
capture_eep_deinit();

/**
 * @brief Return the remote port where HEP packets are sent
 *
 * @return Remote port or NULL if HEP send mode is not running
 */
const char *
capture_eep_send_port();

/**
 * @brief Return the local port where HEP packets are received
 *
 * @return Local listen port or NULL if HEP listen mode is not running
 */
const char *
capture_eep_listen_port();

/**
 * @brief Wrapper for sending packet in configured EEP version
 *
 * @param pkt Packet Structure data
 * @return 1 on any error occurs, 0 otherwise
 */
int
capture_eep_send(packet_t *pkt);

/**
 * @brief Send a captured packet (EEP version 2)
 *
 * Send a packet encapsulated into EEP through the client socket.
 * This function will only handle SIP packets if EEP client mode
 * has been enabled.
 *
 * @param pkt Packet Structure data
 * @return 1 on any error occurs, 0 otherwise
 */
int
capture_eep_send_v2(packet_t *pkt);

/**
 * @brief Send a captured packet (EEP version 3)
 *
 * Send a packet encapsulated into EEP through the client socket.
 * This function will only handle SIP packets if EEP client mode
 * has been enabled.
 *
 * @param pkt Packet Structure data
 * @return 1 on any error occurs, 0 otherwise
 */
int
capture_eep_send_v3(packet_t *pkt);

/**
 * @brief Wrapper for receiving packet in configured EEP version
 *
 * @return NULL on any error, packet structure otherwise
 */
packet_t *
capture_eep_receive();


/**
 * @brief Received a captured packet (EEP version 2)
 *
 * Wait for a packet to be received through the EEP server. This
 * function will parse received EEP data and create a new packet
 * structure.
 *
 * @return NULL on any error, packet structure otherwise
 */
packet_t *
capture_eep_receive_v2();

/**
 * @brief Received a captured packet (EEP version 3)
 *
 * Wait for a packet to be received through the EEP server. This
 * function will parse received EEP data and create a new packet
 * structure.
 *
 * @param pkt packet structure data, NULL if socket should be used
 * @param size size of packet structure data
 * @return NULL on any error, packet structure otherwise
 */
packet_t *
capture_eep_receive_v3(const u_char *pkt, uint32_t size);

/**
 * @brief Set EEP server url
 *
 * Set EEP servermode settings using a url in the format:
 *  - proto:address:port
 * For example:
 *  - udp:10.10.0.100:9060
 *  - udp:0.0.0.0:9960
 *
 * @param url URL to be parsed
 * @return 0 if url has been parsed, 1 otherwise
 */
int
capture_eep_set_server_url(const char *url);

/**
 * @brief Set EEP client url
 *
 * Set EEP clientmode settings using a url in the format:
 *  - proto:address:port
 * For example:
 *  - udp:10.10.0.100:9060
 *  - udp:0.0.0.0:9960
 *
 * @param url URL to be parsed
 * @return 0 if url has been parsed, 1 otherwise
 */
int
capture_eep_set_client_url(const char *url);

#endif /* __SNGREP_CAPTURE_EEP_H */
