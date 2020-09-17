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
 * @file rtp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage rtp captured packets
 *
 * @note RTP_VERSION and RTP_PAYLOAD_TYPE macros has been taken from wireshark
 *       source code: packet-rtp.c
 */

#ifndef __SNGREP_RTP_H
#define __SNGREP_RTP_H

#include "config.h"
#include "capture.h"
#include "media.h"

// Version is the first 2 bits of the first octet
#define RTP_VERSION(octet) ((octet) >> 6)
// Payload type is the last 7 bits
#define RTP_PAYLOAD_TYPE(octet) ((octet) & 0x7F)

// Handled RTP versions
#define RTP_VERSION_RFC1889 2

// RTP header length
#define RTP_HDR_LENGTH 12

// RTCP common header length
#define RTCP_HDR_LENGTH 4

// If stream does not receive a packet in this seconds, we consider it inactive
#define STREAM_INACTIVE_SECS 3

// RTCP header types
//! http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml
enum rtcp_header_types
{
    RTCP_HDR_SR = 200,
    RTCP_HDR_RR,
    RTCP_HDR_SDES,
    RTCP_HDR_BYE,
    RTCP_HDR_APP,
    RTCP_RTPFB,
    RTCP_PSFB,
    RTCP_XR,
    RTCP_AVB,
    RTCP_RSI,
    RTCP_TOKEN,
};

//! http://www.iana.org/assignments/rtcp-xr-block-types/rtcp-xr-block-types.xhtml
enum rtcp_xr_block_types
{
    RTCP_XR_LOSS_RLE = 1,
    RTCP_XR_DUP_RLE,
    RTCP_XR_PKT_RXTIMES,
    RTCP_XR_REF_TIME,
    RTCP_XR_DLRR,
    RTCP_XR_STATS_SUMRY,
    RTCP_XR_VOIP_METRCS,
    RTCP_XR_BT_XNQ,
    RTCP_XR_TI_VOIP,
    RTCP_XR_PR_LOSS_RLE,
    RTCP_XR_MC_ACQ,
    RTCP_XR_IDMS
};

//! Shorter declaration of rtp_encoding structure
typedef struct rtp_encoding rtp_encoding_t;
//! Shorter declaration of rtp_stream structure
typedef struct rtp_stream rtp_stream_t;

struct rtp_encoding {
    uint32_t id;
    const char *name;
    const char *format;
};

struct rtp_stream {
    //! Determine stream type
    uint32_t type;
    //! Source address
    address_t src;
    //! Destination address
    address_t dst;
    //! SDP media that setup this stream
    sdp_media_t *media;
    //! Packet count for this stream
    uint32_t pktcnt;
    //! Time of first received packet of stream
    struct timeval time;
    //! Unix timestamp of last received packet
    int lasttm;

    // Stream information (depending on type)
    union {
        struct {
            //! Format of first received packet of stre
            uint32_t fmtcode;
        } rtpinfo;
        struct {
            //! Sender packet count
            uint32_t spc;
            //! Fraction lost x/256
            uint8_t flost;
            //! uint8_t discarded x/256
            uint8_t fdiscard;
            //! MOS - listening Quality
            uint8_t mosl;
            //! MOS - Conversational Quality
            uint8_t mosc;
        } rtcpinfo;
    };
};

struct rtcp_hdr_generic
{
    //! version (V): 2 bits
    uint8_t version;
    //! packet type (PT): 8 bits
    uint8_t type;
    //! length: 16 bits
    uint16_t len;
};

struct rtcp_hdr_sr
{
    //! version (V): 2 bits
    uint8_t version:2;
    //! padding (P): 1 bit
    uint8_t padding:1;
    //! reception report count (RC): 5 bits
    uint8_t rcount:5;
    //! packet type (PT): 8 bits
    uint8_t type;
    //! length: 16 bits
    uint16_t len;
    //! SSRC: 32 bits
    uint32_t ssrc;
    //! NTP timestamp: 64 bits
    uint64_t ntpts;
    //! RTP timestamp: 32 bits
    uint32_t rtpts;
    //! sender's packet count: 32 bits
    uint32_t spc;
    //! sender's octet count: 32 bits
    uint32_t soc;
};

struct rtcp_blk_sr
{
    //! SSRC_n (source identifier): 32 bits
    uint32_t ssrc;
    //! fraction lost: 8 bits
    uint8_t flost;
    //! cumulative number of packets lost: 24 bits
    struct {
        uint8_t pl1;
        uint8_t pl2;
        uint8_t pl3;
    } plost;
    //! extended highest sequence number received: 32 bits
    uint32_t hseq;
    //! interarrival jitter: 32 bits
    uint32_t ijitter;
};

struct rtcp_hdr_xr
{
    //! version (V): 2 bits
    uint8_t version:2;
    //! padding (P): 1 bit
    uint8_t padding:1;
    //! reserved: 5 bits
    uint8_t reserved:5;
    //! packet type (PT): 8 bits
    uint8_t type;
    //! length: 16 bits
    uint16_t len;
    //! SSRC: 32 bits
    uint32_t ssrc;
};

struct rtcp_blk_xr
{
    //! block type (BT): 8 bits
    uint8_t type;
    //! type-specific: 8 bits
    uint8_t specific;
    //! length: 16 bits
    uint16_t len;
};

struct rtcp_blk_xr_voip
{
    //! block type (BT): 8 bits
    uint8_t type;
    //! type-specific: 8 bits
    uint8_t reserved;
    //! length: 16 bits
    uint16_t len;
    //! SSRC: 32 bits
    uint32_t ssrc;
    //! loss rate: 8 bits
    uint8_t lrate;
    //! discard rate: 8 bits
    uint8_t drate;
    //! burst density: 8 bits
    uint8_t bdens;
    //! gap density: 8 bits
    uint8_t gdens;
    //! burst duration: 16 bits
    uint16_t bdur;
    //! gap duration: 16 bits
    uint16_t gdur;
    //! round trip delay: 16 bits
    uint16_t rtd;
    //! end system delay: 16 bits
    uint16_t esd;
    //! signal level: 8 bits
    uint8_t slevel;
    //! noise level: 8 bits
    uint8_t nlevel;
    //! residual echo return loss (RERL): 8 bits
    uint8_t rerl;
    //! Gmin: 8 bits
    uint8_t gmin;
    //! R factor: 8 bits
    uint8_t rfactor;
    //! ext. R factor: 8 bits
    uint8_t xrfactor;
    //! MOS-LQ: 8 bits
    uint8_t moslq;
    //! MOS-CQ: 8 bits
    uint8_t moscq;
    //! receiver configuration byte (RX config): 8 bits
    uint8_t rxc;
    //! packet loss concealment (PLC): 2 bits
    uint8_t plc:2;
    //! jitter buffer adaptive (JBA): 2 bits
    uint8_t jba:2;
    //! jitter buffer rate (JB rate): 4 bits
    uint8_t jbrate:4;
    //! reserved: 8 bits
    uint8_t reserved2;
    //! jitter buffer nominal delay (JB nominal): 16 bits
    uint16_t jbndelay;
    //! jitter buffer maximum delay (JB maximum): 16 bits
    uint16_t jbmdelay;
    //! jitter buffer absolute maximum delay (JB abs max): 16 bits
    uint16_t jbadelay;
};

rtp_stream_t *
stream_create(sdp_media_t *media, address_t dst, int type);

rtp_stream_t *
stream_complete(rtp_stream_t *stream, address_t src);

void
stream_set_format(rtp_stream_t *stream, uint32_t format);

void
stream_add_packet(rtp_stream_t *stream, packet_t *packet);

uint32_t
stream_get_count(rtp_stream_t *stream);

struct sip_call *
stream_get_call(rtp_stream_t *stream);

const char *
stream_get_format(rtp_stream_t *stream);

const char *
rtp_get_standard_format(uint32_t code);

rtp_stream_t *
rtp_check_packet(packet_t *packet);

rtp_stream_t *
rtp_find_stream_format(address_t src, address_t dst, uint32_t format);

rtp_stream_t *
rtp_find_rtcp_stream(address_t src, address_t dst);

rtp_stream_t *
rtp_find_call_stream(struct sip_call *call, address_t src, address_t dst);

rtp_stream_t *
rtp_find_call_exact_stream(struct sip_call *call, address_t src, address_t dst);

/**
 * @brief Check if a message is older than other
 *
 * @param one rtp stream pointer
 * @param two rtp stream pointer
 * @return 1 if one is older than two
 * @return 0 if equal or two is older than one
 */
int
stream_is_older(rtp_stream_t *one, rtp_stream_t *two);

int
stream_is_complete(rtp_stream_t *stream);

/**
 * @brief Determine if a stream is still active
 *
 * This simply checks the timestamp of the last received packet of the stream
 * marking as inactive if it was before STREAM_INACTIVE_SECS ago
 *
 * @return 1 if stream is active
 * @return 0 if stream is inactive
 */
int
stream_is_active(rtp_stream_t *stream);

/**
 * @brief Check if the data is a RTP packet
 * RFC 5761 Section 4.  Distinguishable RTP and RTCP Packets
 * RFC 5764 Section 5.1.2.  Reception (packet demultiplexing)
 */
int
data_is_rtp(u_char *data, uint32_t len);

/**
 * @brief Check if the data is a RTCP packet
 * RFC 5761 Section 4.  Distinguishable RTP and RTCP Packets
 * RFC 5764 Section 5.1.2.  Reception (packet demultiplexing)
 */
int
data_is_rtcp(u_char *data, uint32_t len);

#endif /* __SNGREP_RTP_H */
