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
 * @file packet_televt.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 * @author Mayama Takeshi <mayamatakeshi@gmail.com>
 *
 * @brief functions to manage DTMF event packets (RFC4733)
 *
 */
#ifndef __SNGREP_PACKET_DTMF_H__
#define __SNGREP_PACKET_DTMF_H__

#include <glib.h>
#include "packet.h"
#include "dissector.h"

G_BEGIN_DECLS

#define PACKET_DISSECTOR_TYPE_DTMF packet_dissector_televt_get_type()
G_DECLARE_FINAL_TYPE(PacketDissectorTelEvt, packet_dissector_televt, PACKET_DISSECTOR, DTMF, PacketDissector)

#define DTMF_MAX_EVENT 16

typedef struct _PacketTelEvtData PacketTelEvtData;
typedef struct _PacketTelEvtHdr PacketTelEvtHdr;
typedef struct _PacketTelEvtCode PacketTelEvtCode;


struct _PacketDissectorTelEvt
{
    //! Parent structure
    PacketDissector parent;
};

struct _PacketTelEvtHdr
{
    guint8 event;
# if __BYTE_ORDER == __LITTLE_ENDIAN
    guint8 volume:6;
    guint8 res:1;
    guint8 end:1;
# endif
# if __BYTE_ORDER == __BIG_ENDIAN
    guint8 end:1;
    guint8 res:1;
    guint8 volume:6;
# endif
    guint16 duration;
} __attribute__((packed));

struct _PacketTelEvtCode
{
    //! Event code
    guint code;
    //! Event value
    gchar value;
    //! Event description
    const gchar *desc;
};

struct _PacketTelEvtData
{
    //! Protocol information
    PacketProtocol proto;
    //! DTMF value based on Telephony event
    gchar value;
    //! Telephony event end flag
    gboolean end;
    //! Event volume
    guint8 volume;
    //! Event Duration
    guint16 duration;
};

PacketTelEvtData *
packet_televt_data(const Packet *packet);

/**
 * @brief Create a Telephony Event packet parser
 *
 * @return a protocols' parsers pointer
 */
PacketDissector *
packet_dissector_televt_new();

G_END_DECLS

#endif /* __SNGREP_PACKET_DTMF_H__ */
