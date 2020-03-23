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
 * @file packet_hep.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage HEP protocol (on the wire)
 */

#ifndef __SNGREP_PACKET_HEP_H__
#define __SNGREP_PACKET_HEP_H__

#include "dissector.h"

G_BEGIN_DECLS

#define PACKET_DISSECTOR_TYPE_HEP packet_dissector_hep_get_type()
G_DECLARE_FINAL_TYPE(PacketDissectorHep, packet_dissector_hep, PACKET_DISSECTOR, HEP, PacketDissector)

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

typedef struct _PacketHepData PacketHepData;
typedef struct _CaptureHepHdr CaptureHepHdr;
typedef struct _CaptureHepCtrl CaptureHepCtrl;
typedef struct _CaptureHepGeneric CaptureHepGeneric;
typedef struct _CaptureHepChunk CaptureHepChunk;
typedef struct _CaptureHepChunkIp4 CaptureHepChunkIp4;
typedef struct _CaptureHepChunkIp6 CaptureHepChunkIp6;
typedef struct _CaptureHepChunkUint8 CaptureHepChunkUint8;
typedef struct _CaptureHepChunkUint16 CaptureHepChunkUint16;
typedef struct _CaptureHepChunkUint32 CaptureHepChunkUint32;

struct _PacketDissectorHep
{
    //! Parent structure
    PacketDissector parent;
};

/* HEPv3 types */
struct _CaptureHepChunk
{
    guint16 vendor_id;
    guint16 type_id;
    guint16 length;
}__attribute__((packed));


struct _CaptureHepChunkUint8
{
    struct _CaptureHepChunk chunk;
    guint8 data;
}__attribute__((packed));


struct _CaptureHepChunkUint16
{
    struct _CaptureHepChunk chunk;
    guint16 data;
}__attribute__((packed));


struct _CaptureHepChunkUint32
{
    struct _CaptureHepChunk chunk;
    guint32 data;
}__attribute__((packed));

struct _CaptureHepChunkIp4
{
    struct _CaptureHepChunk chunk;
    struct in_addr data;
}__attribute__((packed));

struct _CaptureHepChunkIp6
{
    struct _CaptureHepChunk chunk;
    struct in6_addr data;
}__attribute__((packed));


struct _CaptureHepCtrl
{
    gchar id[4];
    guint16 length;
}__attribute__((packed));

struct _CaptureHepGeneric
{
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

struct _CaptureHepHdr
{
    guint8 hp_v;            /* version */
    guint8 hp_l;            /* length */
    guint8 hp_f;            /* family */
    guint8 hp_p;            /* protocol */
    guint16 hp_sport;       /* source port */
    guint16 hp_dport;       /* destination port */
};

struct _CaptureHepTimeHdr
{
    guint32 tv_sec;         /* seconds */
    guint32 tv_usec;        /* useconds */
    guint16 captid;         /* Capture ID node */
};

struct _CaptureHepIpHdr
{
    struct in_addr hp_src;
    struct in_addr hp_dst;  /* source and dest address */
};

#ifdef USE_IPV6
struct _CaptureHepIp6Hdr
{
    struct in6_addr hp6_src; /* source address */
    struct in6_addr hp6_dst; /* destination address */
};
#endif

struct _PacketHepData
{
    gchar dummy;
};

/**
 * @brief Create an HEP parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_dissector_hep_new();

G_END_DECLS

#endif  /* __SNGREP_PACKET_HEP_H__ */
