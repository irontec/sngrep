/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2021 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2021 Irontec SL. All rights reserved.
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
 * @file packet_mrcp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 * @author Mayama Takeshi <mayamatakeshi@gmail.com>
 *
 * @brief functions to manage MRCPv2 protocol
 *
 */
#ifndef __SNGREP_PACKET_MRCP_H__
#define __SNGREP_PACKET_MRCP_H__

#include <glib.h>
#include "packet.h"
#include "dissector.h"

G_BEGIN_DECLS

#define PACKET_DISSECTOR_TYPE_MRCP packet_dissector_mrcp_get_type()
G_DECLARE_FINAL_TYPE(PacketDissectorMrcp, packet_dissector_mrcp, PACKET_DISSECTOR, MRCP, PacketDissector)

#define MRCP_VERSION "MRCP/2.0"
#define MRCP_VERSION_LEN 8
#define MRCP_CRLF "\r\n"

typedef struct _PacketMrcpData PacketMrcpData;
typedef struct _PacketMrcpCode PacketMrcpCode;

struct _PacketDissectorMrcp
{
    //! Parent structure
    PacketDissector parent;
};

//! MRCP Methods
enum PacketMrcpMethods
{
    MRCP_METHOD_SET_PARAMS = 1,
    MRCP_METHOD_GET_PARAMS,
    MRCP_METHOD_SPEAK,
    MRCP_METHOD_STOP,
    MRCP_METHOD_PAUSE,
    MRCP_METHOD_RESUME,
    MRCP_METHOD_BARGE_IN_OCCURRED,
    MRCP_METHOD_CONTROL,
    MRCP_METHOD_DEFINE_LEXICON,
    MRCP_METHOD_DEFINE_GRAMMAR,
    MRCP_METHOD_RECOGNIZE,
    MRCP_METHOD_INTERPRET,
    MRCP_METHOD_GET_RESULT,
    MRCP_METHOD_START_INPUT_TIMERS,
    MRCP_METHOD_START_PHRASE_ENROLLMENT,
    MRCP_METHOD_ENROLLMENT_ROLLBACK,
    MRCP_METHOD_END_PHRASE_ENROLLMENT,
    MRCP_METHOD_MODIFY_PHRASE,
    MRCP_METHOD_DELETE_PHRASE,
    MRCP_METHOD_RECORD,
    MRCP_METHOD_START_SESSION,
    MRCP_METHOD_END_SESSION,
    MRCP_METHOD_QUERY_VOICEPRINT,
    MRCP_METHOD_DELETE_VOICEPRINT,
    MRCP_METHOD_VERIFY,
    MRCP_METHOD_VERIFY_FROM_BUFFER,
    MRCP_METHOD_VERIFY_ROLLBACK,
    MRCP_METHOD_CLEAR_BUFFER,
    MRCP_METHOD_GET_INTERMEDIATE_RESULT,
};

enum PacketMrcpMessageTypes
{
    MRCP_MESSAGE_REQUEST,
    MRCP_MESSAGE_RESPONSE,
    MRCP_MESSAGE_EVENT,
};

/**
 * @brief Different Request/Response codes in MRCP Protocol
 */
struct _PacketMrcpCode
{
    guint id;
    gchar *text;
};

struct _PacketMrcpData
{
    //! Protocol information
    PacketProtocol proto;
    //! Request Method or Response code data
    gchar *method;
    //! Response Code or Method Id, zero for events
    guint code;
    //! MRCP Message type
    enum PacketMrcpMessageTypes type;
    //! MRCP Message payload
    GBytes *payload;
    //! MRCP Channel Header value
    gchar *channel;
    //! Content-Length header value
    guint64 content_len;
    //! Request-Id
    guint64 request_id;
};

guint
packet_mrcp_method_from_str(const gchar *method);

gchar *
packet_mrcp_payload_str(const Packet *packet);

const gchar *
packet_mrcp_method_str(const Packet *packet);

guint
packet_mrcp_method(const Packet *packet);

guint64
packet_mrcp_request_id(const Packet *packet);

gboolean
packet_mrcp_is_request(const Packet *packet);

PacketMrcpData *
packet_mrcp_data(const Packet *packet);

/**
 * @brief Create a MRCP parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_dissector_mrcp_new();

G_END_DECLS

#endif /* __SNGREP_PACKET_MRCP_H__ */
