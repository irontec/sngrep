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
 * @file packet_sip.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_sip.h
 */

#include "config.h"
#include <stdlib.h>
#include <packet/old_packet.h>
#include "packet_sip.h"
#include "packet_ip.h"
#include "packet_tcp.h"
#include "packet_udp.h"
#include "sip.h"
#include "packet/old_packet.h"

/* @brief list of methods and responses */
sip_code_t sip_codes[] = {
    { SIP_METHOD_REGISTER,  "REGISTER" },
    { SIP_METHOD_INVITE,    "INVITE" },
    { SIP_METHOD_SUBSCRIBE, "SUBSCRIBE" },
    { SIP_METHOD_NOTIFY,    "NOTIFY" },
    { SIP_METHOD_OPTIONS,   "OPTIONS" },
    { SIP_METHOD_PUBLISH,   "PUBLISH" },
    { SIP_METHOD_MESSAGE,   "MESSAGE" },
    { SIP_METHOD_CANCEL,    "CANCEL" },
    { SIP_METHOD_BYE,       "BYE" },
    { SIP_METHOD_ACK,       "ACK" },
    { SIP_METHOD_PRACK,     "PRACK" },
    { SIP_METHOD_INFO,      "INFO" },
    { SIP_METHOD_REFER,     "REFER" },
    { SIP_METHOD_UPDATE,    "UPDATE" },
    { 100, "100 Trying" },
    { 180, "180 Ringing" },
    { 181, "181 Call is Being Forwarded" },
    { 182, "182 Queued" },
    { 183, "183 Session Progress" },
    { 199, "199 Early Dialog Terminated" },
    { 200, "200 OK" },
    { 202, "202 Accepted" },
    { 204, "204 No Notification" },
    { 300, "300 Multiple Choices" },
    { 301, "301 Moved Permanently" },
    { 302, "302 Moved Temporarily" },
    { 305, "305 Use Proxy" },
    { 380, "380 Alternative Service" },
    { 400, "400 Bad Request" },
    { 401, "401 Unauthorized" },
    { 402, "402 Payment Required" },
    { 403, "403 Forbidden" },
    { 404, "404 Not Found" },
    { 405, "405 Method Not Allowed" },
    { 406, "406 Not Acceptable" },
    { 407, "407 Proxy Authentication Required" },
    { 408, "408 Request Timeout" },
    { 409, "409 Conflict" },
    { 410, "410 Gone" },
    { 411, "411 Length Required" },
    { 412, "412 Conditional Request Failed" },
    { 413, "413 Request Entity Too Large" },
    { 414, "414 Request-URI Too Long" },
    { 415, "415 Unsupported Media Type" },
    { 416, "416 Unsupported URI Scheme" },
    { 417, "417 Unknown Resource-Priority" },
    { 420, "420 Bad Extension" },
    { 421, "421 Extension Required" },
    { 422, "422 Session Interval Too Small" },
    { 423, "423 Interval Too Brief" },
    { 424, "424 Bad Location Information" },
    { 428, "428 Use Identity Header" },
    { 429, "429 Provide Referrer Identity" },
    { 430, "430 Flow Failed" },
    { 433, "433 Anonymity Disallowed" },
    { 436, "436 Bad Identity-Info" },
    { 437, "437 Unsupported Certificate" },
    { 438, "438 Invalid Identity Header" },
    { 439, "439 First Hop Lacks Outbound Support" },
    { 470, "470 Consent Needed" },
    { 480, "480 Temporarily Unavailable" },
    { 481, "481 Call/Transaction Does Not Exist" },
    { 482, "482 Loop Detected." },
    { 483, "483 Too Many Hops" },
    { 484, "484 Address Incomplete" },
    { 485, "485 Ambiguous" },
    { 486, "486 Busy Here" },
    { 487, "487 Request Terminated" },
    { 488, "488 Not Acceptable Here" },
    { 489, "489 Bad Event" },
    { 491, "491 Request Pending" },
    { 493, "493 Undecipherable" },
    { 494, "494 Security Agreement Required" },
    { 500, "500 Server Internal Error" },
    { 501, "501 Not Implemented" },
    { 502, "502 Bad Gateway" },
    { 503, "503 Service Unavailable" },
    { 504, "504 Server Time-out" },
    { 505, "505 Version Not Supported" },
    { 513, "513 Message Too Large" },
    { 580, "580 Precondition Failure" },
    { 600, "600 Busy Everywhere" },
    { 603, "603 Decline" },
    { 604, "604 Does Not Exist Anywhere" },
    { 606, "606 Not Acceptable" },
    { -1 , NULL },
};

const char *
sip_method_str(int method)
{
    int i;

    // Standard method
    for (i = 0; sip_codes[i].id > 0; i++) {
        if (method == sip_codes[i].id)
            return sip_codes[i].text;
    }
    return NULL;
}

int
sip_method_from_str(const char *method)
{
    int i;

    // Standard method
    for (i = 0; sip_codes[i].id > 0; i++) {
        if (!g_strcmp0(method, sip_codes[i].text))
            return sip_codes[i].id;
    }
    return atoi(method);
}

GByteArray *
packet_sip_parse(PacketParser *parser G_GNUC_UNUSED, Packet *packet, GByteArray *data)
{
    packet_t *oldpkt = g_malloc0(sizeof(packet_t));

    PacketIpData *ipdata = g_ptr_array_index(packet->proto, PACKET_IP);
    oldpkt->src = ipdata->saddr;
    oldpkt->dst = ipdata->daddr;

    if (packet_has_type(packet, PACKET_TCP)) {
        PacketTcpData *tcpdata = g_ptr_array_index(packet->proto, PACKET_TCP);
        oldpkt->src.port = tcpdata->sport;
        oldpkt->dst.port = tcpdata->dport;
    } else {
        PacketUdpData *udpdata = g_ptr_array_index(packet->proto, PACKET_UDP);
        oldpkt->src.port = udpdata->sport;
        oldpkt->dst.port = udpdata->dport;
    }

    packet_set_payload(oldpkt, data->data, data->len);

    oldpkt->frames = g_sequence_new(NULL);
    for (GList *l = packet->frames; l != NULL; l = l->next) {
        PacketFrame *frame = l->data;
        packet_add_frame(oldpkt, frame->header, frame->data);
    }

    if (sip_check_packet(oldpkt) != NULL)
        return NULL;
    else
        return data;
}

static void
packet_sip_init(PacketParser *parser G_GNUC_UNUSED)
{

}

static void
packet_sip_deinit(PacketParser *parser G_GNUC_UNUSED)
{

}

PacketDissector *
packet_sip_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_SIP;
    proto->init = packet_sip_init;
    proto->dissect = packet_sip_parse;
    proto->deinit = packet_sip_deinit;
    return proto;
}