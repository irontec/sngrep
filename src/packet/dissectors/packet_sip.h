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
 * @file packet_sip.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage SIP packets
 *
 */
#ifndef __SNGREP_PACKET_SIP_H
#define __SNGREP_PACKET_SIP_H

#include <glib.h>
#include "../packet.h"
#include "packet_sdp.h"

typedef struct _PacketSipData PacketSipData;
typedef struct _PacketSipCode PacketSipCode;
typedef struct _DissectorSipData DissectorSipData;

//! SIP Methods
enum sip_methods {
    SIP_METHOD_REGISTER = 1,
    SIP_METHOD_INVITE,
    SIP_METHOD_SUBSCRIBE,
    SIP_METHOD_NOTIFY,
    SIP_METHOD_OPTIONS,
    SIP_METHOD_PUBLISH,
    SIP_METHOD_CANCEL,
    SIP_METHOD_INFO,
    SIP_METHOD_REFER,
    SIP_METHOD_UPDATE,
    SIP_METHOD_MESSAGE,
    SIP_METHOD_ACK,
    SIP_METHOD_PRACK,
    SIP_METHOD_BYE,
};

//! SIP Headers
enum sip_headers {
    SIP_HEADER_FROM = 0,
    SIP_HEADER_TO,
    SIP_HEADER_CALLID,
    SIP_HEADER_XCALLID,
    SIP_HEADER_CSEQ,
    SIP_HEADER_REASON,
    SIP_HEADER_WARNING,
    SIP_HEADER_COUNT,
};

/**
 * @brief Different Request/Response codes in SIP Protocol
 */
struct _PacketSipCode
{
    int id;
    const char *text;
};

struct _PacketSipData
{
    //! Request Method or Response Code @see sip_methods
    guint reqresp;
    //!  Response text if it doesn't matches an standard
    char *resp_str;
    //! SIP payload (Headers + Body)
    gchar *payload;
    //! Parsed headers
    GPtrArray *headers;
    //! SIP Call-Id Heder value
    gchar *callid;
    //! SIP X-Call-Id Heder value
    gchar *xcallid;

    //! Message Cseq
    int cseq;

    gchar *reasontxt;
    int warning;
};

struct _DissectorSipData
{
    //! Regexp for payload matching
    GRegex *reg_method;
    GRegex *reg_callid;
    GRegex *reg_xcallid;
    GRegex *reg_response;
    GRegex *reg_cseq;
    GRegex *reg_from;
    GRegex *reg_to;
    GRegex *reg_valid;
    GRegex *reg_cl;
    GRegex *reg_body;
    GRegex *reg_reason;
    GRegex *reg_warning;
};

int
sip_method_from_str(const char *method);

const char *
sip_method_str(int method);

const gchar *
packet_sip_payload(const Packet *packet);

const gchar *
packet_sip_header(const Packet *packet, enum sip_headers header);

const gchar *
packet_sip_method_str(const Packet *packet);

const guint
packet_sip_method(const Packet *packet);

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
