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
 * @file packet_rtcp.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions and definitions to manage RTCP packets
 *
 */

#ifndef __SNGREP_PACKET_RTCP_H__
#define __SNGREP_PACKET_RTCP_H__

#include <glib.h>

G_BEGIN_DECLS

#define PACKET_DISSECTOR_TYPE_RTCP packet_dissector_rtcp_get_type()
G_DECLARE_FINAL_TYPE(PacketDissectorRtcp, packet_dissector_rtcp, PACKET_DISSECTOR, RTCP, PacketDissector)

typedef struct _PacketRtcpData PacketRtcpData;

struct _PacketDissectorRtcp
{
    //! Parent structure
    PacketDissector parent;
};

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

struct rtcp_hdr_generic
{
    //! version (V): 2 bits
    guint8 version;
    //! packet type (PT): 8 bits
    guint8 type;
    //! length: 16 bits
    guint16 len;
};

struct rtcp_hdr_sr
{
    //! version (V): 2 bits
    guint8 version:2;
    //! padding (P): 1 bit
    guint8 padding:1;
    //! reception report count (RC): 5 bits
    guint8 rcount:5;
    //! packet type (PT): 8 bits
    guint8 type;
    //! length: 16 bits
    guint16 len;
    //! SSRC: 32 bits
    guint32 ssrc;
    //! NTP timestamp: 64 bits
    guint64 ntpts;
    //! RTP timestamp: 32 bits
    guint32 rtpts;
    //! sender's packet count: 32 bits
    guint32 spc;
    //! sender's octet count: 32 bits
    guint32 soc;
};

struct rtcp_blk_sr
{
    //! SSRC_n (source identifier): 32 bits
    guint32 ssrc;
    //! fraction lost: 8 bits
    guint8 flost;
    //! cumulative number of packets lost: 24 bits
    struct
    {
        guint8 pl1;
        guint8 pl2;
        guint8 pl3;
    } plost;
    //! extended highest sequence number received: 32 bits
    guint32 hseq;
    //! interarrival jitter: 32 bits
    guint32 ijitter;
};

struct rtcp_hdr_xr
{
    //! version (V): 2 bits
    guint8 version:2;
    //! padding (P): 1 bit
    guint8 padding:1;
    //! reserved: 5 bits
    guint8 reserved:5;
    //! packet type (PT): 8 bits
    guint8 type;
    //! length: 16 bits
    guint16 len;
    //! SSRC: 32 bits
    guint32 ssrc;
};

struct rtcp_blk_xr
{
    //! block type (BT): 8 bits
    guint8 type;
    //! type-specific: 8 bits
    guint8 specific;
    //! length: 16 bits
    guint16 len;
};

struct rtcp_blk_xr_voip
{
    //! block type (BT): 8 bits
    guint8 type;
    //! type-specific: 8 bits
    guint8 reserved;
    //! length: 16 bits
    guint16 len;
    //! SSRC: 32 bits
    guint32 ssrc;
    //! loss rate: 8 bits
    guint8 lrate;
    //! discard rate: 8 bits
    guint8 drate;
    //! burst density: 8 bits
    guint8 bdens;
    //! gap density: 8 bits
    guint8 gdens;
    //! burst duration: 16 bits
    guint16 bdur;
    //! gap duration: 16 bits
    guint16 gdur;
    //! round trip delay: 16 bits
    guint16 rtd;
    //! end system delay: 16 bits
    guint16 esd;
    //! signal level: 8 bits
    guint8 slevel;
    //! noise level: 8 bits
    guint8 nlevel;
    //! residual echo return loss (RERL): 8 bits
    guint8 rerl;
    //! Gmin: 8 bits
    guint8 gmin;
    //! R factor: 8 bits
    guint8 rfactor;
    //! ext. R factor: 8 bits
    guint8 xrfactor;
    //! MOS-LQ: 8 bits
    guint8 moslq;
    //! MOS-CQ: 8 bits
    guint8 moscq;
    //! receiver configuration byte (RX config): 8 bits
    guint8 rxc;
    //! packet loss concealment (PLC): 2 bits
    guint8 plc:2;
    //! jitter buffer adaptive (JBA): 2 bits
    guint8 jba:2;
    //! jitter buffer rate (JB rate): 4 bits
    guint8 jbrate:4;
    //! reserved: 8 bits
    guint8 reserved2;
    //! jitter buffer nominal delay (JB nominal): 16 bits
    guint16 jbndelay;
    //! jitter buffer maximum delay (JB maximum): 16 bits
    guint16 jbmdelay;
    //! jitter buffer absolute maximum delay (JB abs max): 16 bits
    guint16 jbadelay;
};

struct _PacketRtcpData
{
    //! Sender packet count
    guint32 spc;
    //! Fraction lost x/256
    guint8 flost;
    //! guint8 discarded x/256
    guint8 fdiscard;
    //! MOS - listening Quality
    guint8 mosl;
    //! MOS - Conversational Quality
    guint8 mosc;
};

PacketDissector *
packet_dissector_rtcp_new();

G_END_DECLS

#endif  /* __SNGREP_PACKET_RTCP_H__ */
