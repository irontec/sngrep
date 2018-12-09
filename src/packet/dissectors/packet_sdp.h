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
 * @file packet_sdp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief functions to manage SDP protocol
 */

#ifndef PACKET_PACKET_SDP_H_
#define PACKET_PACKET_SDP_H_

#include <glib.h>
#include "../packet.h"
#include "packet/dissector.h"


//! Connection Data fields
#define SDP_CONN_NETTYPE    0
#define SDP_CONN_ADDRTYPE   1
#define SDP_CONN_ADDRESS    2

//! Media Description fields
#define SDP_MEDIA_MEDIA     0
#define SDP_MEDIA_PORT      1
#define SDP_MEDIA_PROTO     2
#define SDP_MEDIA_FORMAT    3

//! Attribute fields
#define SDP_ATTR_NAME       0
#define SDP_ATTR_VALUE      1


//! SDP handled media types
enum PacketSdpMediaType
{
    SDP_MEDIA_UNKNOWN       = -1,
    SDP_MEDIA_AUDIO         = 0,
    SDP_MEDIA_VIDEO         = 1,
    SDP_MEDIA_TEXT          = 2,
    SDP_MEDIA_APPLICATION   = 3,
    SDP_MEDIA_MESSAGE       = 4,
    SDP_MEDIA_IMAGE         = 5,
};

//! Shorter declaration of SDP structures
typedef struct _PacketSdpConnection PacketSdpConnection;
typedef struct _PacketSdpMedia      PacketSdpMedia;
typedef struct _PacketSdpFormat     PacketSdpFormat;
typedef struct _PacketSdpData       PacketSdpData;

/**
 * @brief SDP ConnectionData (c=) information
 *
 * c=<nettype> <addrtype> <connection-address>
 *
 * Note that sngrep only supports Nettype IN (Internat) and Address types
 * IP4 or IP6.
 *
 * RFC4566: A session description MUST contain either at least one "c="
 * field in each media description or a single "c=" field at the session
 * level. It MAY contain a single session-level "c=" field and additional "c="
 * field(s) per media description, in which case the per-media values
 * override the session-level settings for the respective media.
 *
 * We only support one connection data per media description. If multicast
 * connection strings are provided, only one will be parsed.
 */
struct _PacketSdpConnection
{
    //! Connection Address
    gchar address[ADDRESSLEN];
};

/**
 * @brief SDP Media description (m=) information
 *
 * m=<media> <port> <proto> <fmt> ...
 *
 * Note that sngrep only supports one port specifications and RTP/AVP transport
 * protocols. If format codes must match one of the well known formats or be
 * present in one of the media formats description lines.
 *
 */
struct _PacketSdpMedia
{
    //! Media type
    enum PacketSdpMediaType type;
    //! Session connection address (if not global)
    PacketSdpConnection *sconn;
    //! RTP Transport port
    guint16 rtpport;
    //! RTCP Transport port
    guint16 rtcpport;
    //! RTP Address (Connection IP + RTP port)
    Address address;
    //! Media formats list (SDPFormat)
    GList *formats;
};

/**
 * @brief SDP Format description information
 *
 * This structure is used both for well known SDP formats defined in formats[]
 * global array or specific media formats described in attribute lines of media.
 *
 * Note that sngrep only supports RTP transport protocol so all SDP format
 * ids are actually RTP Payload type numbers.
 */
struct _PacketSdpFormat
{
    //! RTP payload
    guint32 id;
    //! RTP Encoding name from RFC3551 or SDP fmt attribute
    const gchar *name;
    //! Shorter encoding representation
    const gchar *alias;
};

struct _PacketSdpData
{
    //! Session connection address (optional)
    PacketSdpConnection *sconn;
    //! SDP Media description list (SDPMedia)
    GList *medias;
};

const gchar*
packet_sdp_media_type_str(enum PacketSdpMediaType type);

PacketDissector *
packet_sdp_new();

#endif /* PACKET_PACKET_SDP_H_ */
