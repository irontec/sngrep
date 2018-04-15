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

//! Shorter declaration of sip codes structure
typedef struct sip_code sip_code_t;

/**
 * @brief Different Request/Response codes in SIP Protocol
 */
struct sip_code
{
    int id;
    const char *text;
};

#if 0
#define SIP_CRLF            "\r\n"
#define SIP_VERSION         "SIP/2.0"
#define SIP_VERSION_LEN     7
#define SIP_MAX_PAYLOAD     10240

//! SIP Method ids
enum sip_method_ids {
    SIP_METHOD_REGISTER     = 1,
    SIP_METHOD_INVITE,
    SIP_METHOD_SUBSCRIBE,
    SIP_METHOD_NOTIFY,
    SIP_METHOD_OPTIONS,
    SIP_METHOD_PUBLISH,
    SIP_METHOD_MESSAGE,
    SIP_METHOD_CANCEL,
    SIP_METHOD_BYE,
    SIP_METHOD_ACK,
    SIP_METHOD_PRACK,
    SIP_METHOD_INFO,
    SIP_METHOD_REFER,
    SIP_METHOD_UPDATE,
    SIP_METHOD_DO,
    SIP_METHOD_QAUTH,
    SIP_METHOD_SPRACK
};

//! SIP Wanted Header ids
enum sip_header_ids {
	SIP_HEADER_CALLID	    = 1,
	SIP_HEADER_FROM,
	SIP_HEADER_TO,
	SIP_HEADER_CSEQ,
	SIP_HEADER_XCALLID,
	SIP_HEADER_CONTENTLEN,	
	SIP_HEADER_CONTENTTYPE,
	SIP_HEADER_REASON
};

//! Return values for sip_validate_packet
enum sip_payload_status {
    SIP_PAYLOAD_INVALID     = -1,
    SIP_PAYLOAD_INCOMPLETE  = 0,
    SIP_PAYLOAD_VALID       = 1
};

//! Type definitions for SIP structures
typedef struct _SIPMethod SIPMethod;
typedef struct _SIPHeader SIPHeader;
typedef struct _SIPPacket SIPPacket;
typedef struct _SIPParser SIPParser;
typedef struct _SIPRequest  SIPRequest;
typedef struct _SIPResponse SIPResponse;

//! SIP Method struct
struct _SIPMethod {
    //! Method ID
    enum sip_method_ids id;
    //! Method name
    const gchar *name;
};

//! SIP Response
struct _SIPRequest {
    int method;
    const gchar* text;
};

//! SIP Response
struct _SIPResponse {
    int code;
    const gchar* text;
};

//! SIP Headers struct
struct _SIPHeader {
    //! SIP Header Id
	enum sip_header_ids id;
	//! SIP parsin function
	void (*parse)(PacketDissector *, Packet *);
};

//! SIP specific packet data
struct _SIPPacket
{
    // SIP Request packet
    gboolean isrequest;

    //! SIP content payload
    gchar *payload;

    union {
        //! Request data
        SIPRequest req;
        //! Response data
        SIPResponse resp;
    };

	gchar *callid;
	gchar *from;
	gchar *fromuser;
	gchar *to;
	gchar *touser;
	gchar *xcallid;
	gchar *contenttype;
    gint contentlen;
    gint cseq;

	//! SDP packet specific data
	struct sdp_pvt *sdp;
};

//! SIP Pareser private data
struct _SIPParser
{
    GRegex *reg_request_line;
    GRegex *reg_status_line;
    GRegex *reg_header;
    GRegex *reg_callid;
    GRegex *reg_xcallid;
    GRegex *reg_from;
    GRegex *reg_to;
    GRegex *reg_cseq;
    GRegex *reg_clen;
    GRegex *reg_ctype;
};

#endif

/**
 * @brief Create a SIP parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_sip_new();

#endif /* __SNGREP_PACKET_SIP_H */
