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
 * @file packet_sip.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP packets
 *
 */
#ifndef __SNGREP_PACKET_SIP_H
#define __SNGREP_PACKET_SIP_H

#include <glib.h>
#include "parser/packet.h"
#include "packet_sdp.h"

#define SIP_VERSION "SIP/2.0"
#define SIP_VERSION_LEN 7
#define SIP_CRLF "\r\n"

typedef struct _PacketSipData PacketSipData;
typedef struct _PacketSipCode PacketSipCode;

//! SIP Methods
enum SipMethods
{
    SIP_METHOD_REGISTER = 1,
    SIP_METHOD_INVITE,
    SIP_METHOD_SUBSCRIBE,
    SIP_METHOD_NOTIFY,
    SIP_METHOD_OPTIONS,
    SIP_METHOD_PUBLISH,
    SIP_METHOD_INFO,
    SIP_METHOD_REFER,
    SIP_METHOD_UPDATE,
    SIP_METHOD_MESSAGE,
    SIP_METHOD_CANCEL,
    SIP_METHOD_ACK,
    SIP_METHOD_PRACK,
    SIP_METHOD_BYE,
};

/**
 * @brief Different Request/Response codes in SIP Protocol
 */
struct _PacketSipCode
{
    guint id;
    gchar *text;
};

struct _PacketSipData
{
    //! Request Method or Response code data
    PacketSipCode code;
    //! Is this an initial request? (no to-tag)
    gboolean initial;
    //! SIP payload (Headers + Body)
    gchar *payload;
    //! Content-Length header value
    guint64 content_len;
    //! SIP Call-Id Header value
    gchar *callid;
    //! SIP X-Call-Id Header value
    gchar *xcallid;
    //! Message Cseq
    guint64 cseq;
    //! SIP Authentication Header value
    gchar *auth;
};

guint
packet_sip_method_from_str(const gchar *method);

const gchar *
sip_method_str(guint method);

const gchar *
packet_sip_payload(const Packet *packet);

const gchar *
packet_sip_method_str(const Packet *packet);

guint
packet_sip_method(const Packet *packet);

guint64
packet_sip_cseq(const Packet *packet);

gboolean
packet_sip_initial_transaction(const Packet *packet);

const gchar *
packet_sip_auth_data(const Packet *packet);

PacketSipData *
packet_sip_data(const Packet *packet);

/**
 * @brief Create a SIP parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_sip_new();

#endif /* __SNGREP_PACKET_SIP_H */
