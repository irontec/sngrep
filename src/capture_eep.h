/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
#include <pthread.h>
#include "capture.h"

typedef struct capture_eep_config  capture_eep_config_t;
typedef struct eep_info eep_info_t;

struct capture_eep_config
{
    int client_sock;
    int server_sock;
    int capt_id;
    const char *capt_host;
    const char *capt_port;
    const char *capt_password;
    const char *capt_srv_host;
    const char *capt_srv_port;
    const char *capt_srv_password;
    pthread_t server_thread;
};


struct eep_info {
    uint8_t     ip_family;  /* IP family IPv6 IPv4 */
    uint8_t     ip_proto;   /* IP protocol ID : tcp/udp */
    uint8_t     proto_type; /* SIP: 0x001, SDP: 0x03 */
    const char  *ip_src;
    const char  *ip_dst;
    uint16_t    sport;
    uint16_t    dport;
    uint32_t    time_sec;
    uint32_t    time_usec;
} ;

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

/* Structure of HEP */

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

int
capture_eep_init();

void
capture_eep_deinit();

int
capture_eep_send(capture_packet_t *pkt);

capture_packet_t *
capture_eep_receive();

int
capture_eep_set_server_url(const char *url);

int
capture_eep_set_client_url(const char *url);

