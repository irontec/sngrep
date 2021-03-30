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
 * @brief functions to manage DTMF events (RFC4733)
 *
 */

#include "config.h"
#include "glib-extra/glib.h"
#include "packet.h"
#include "packet_televt.h"
#include "storage/storage.h"

G_DEFINE_TYPE(PacketDissectorTelEvt, packet_dissector_televt, PACKET_TYPE_DISSECTOR)

/**
 *  @brief Events from RFC 4733 Page 39
 *  +------------+--------------------------------+-----------+
 *  | Event Code | Event Name                     | Reference |
 *  +------------+--------------------------------+-----------+
 *  |          0 | DTMF digit "0"                 |  RFC 4733 |
 *  |          1 | DTMF digit "1"                 |  RFC 4733 |
 *  |          2 | DTMF digit "2"                 |  RFC 4733 |
 *  |          3 | DTMF digit "3"                 |  RFC 4733 |
 *  |          4 | DTMF digit "4"                 |  RFC 4733 |
 *  |          5 | DTMF digit "5"                 |  RFC 4733 |
 *  |          6 | DTMF digit "6"                 |  RFC 4733 |
 *  |          7 | DTMF digit "7"                 |  RFC 4733 |
 *  |          8 | DTMF digit "8"                 |  RFC 4733 |
 *  |          9 | DTMF digit "9"                 |  RFC 4733 |
 *  |         10 | DTMF digit "*"                 |  RFC 4733 |
 *  |         11 | DTMF digit "#"                 |  RFC 4733 |
 *  |         12 | DTMF digit "A"                 |  RFC 4733 |
 *  |         13 | DTMF digit "B"                 |  RFC 4733 |
 *  |         14 | DTMF digit "C"                 |  RFC 4733 |
 *  |         15 | DTMF digit "D"                 |  RFC 4733 |
 *  +------------+--------------------------------+-----------+
 *
 *      Table 7: audio/telephone-event Event Code Registry
 */
PacketTelEvtCode event_codes[] = {
    { 0,    '0',    "DTMF Zero 0" },
    { 1,    '1',    "DTMF One 1" },
    { 2,    '2',    "DTMF Two 2" },
    { 3,    '3',    "DTMF Three 3" },
    { 4,    '4',    "DTMF Four 4" },
    { 5,    '5',    "DTMF Five 5" },
    { 6,    '6',    "DTMF Six 6" },
    { 7,    '7',    "DTMF Seven 7" },
    { 8,    '8',    "DTMF Eight 8" },
    { 9,    '9',    "DTMF Nine 9" },
    { 10,   '*',    "DTMF Star *" },
    { 11,   '#',    "DTMF Pound #" },
    { 12,   'A',    "DTMF Pound A" },
    { 13,   'B',    "DTMF Pound B" },
    { 14,   'C',    "DTMF Pound C" },
    { 15,   'D',    "DTMF Pound D" },
};

PacketTelEvtData *
packet_televt_data(const Packet *packet)
{
    g_return_val_if_fail(packet != NULL, NULL);

    // Get Packet DTMF data
    PacketTelEvtData *televt = packet_get_protocol_data(packet, PACKET_PROTO_TELEVT);
    g_return_val_if_fail(televt != NULL, NULL);

    return televt;
}

static GBytes *
packet_dissector_televt_dissect(PacketDissector *self, Packet *packet, GBytes *data)
{
    gchar value = ' ';

    // Not enough data for a DTMF packet
    if (g_bytes_get_size(data) != sizeof(PacketTelEvtHdr))
        return data;

    PacketTelEvtHdr *hdr = (PacketTelEvtHdr *) g_bytes_get_data(data, NULL);
    // Convert telephony event to value
    for (guint i = 0; i < DTMF_MAX_EVENT; i++) {
        if (hdr->event == event_codes[i].code) {
            value = event_codes[i].value;
            break;
        }
    }

    // Check event is a DTMF
    if (g_ascii_isspace(value)) {
        return data;
    }

    // Allocate packet televt data
    PacketTelEvtData *televt_data = g_malloc0(sizeof(PacketTelEvtData));
    televt_data->proto.id = PACKET_PROTO_TELEVT;
    televt_data->end = hdr->end == 1;
    televt_data->volume = hdr->volume;
    televt_data->duration = g_ntohs(hdr->duration);
    televt_data->value = value;

    // Add SIP information to the packet
    packet_set_protocol_data(packet, PACKET_PROTO_TELEVT, televt_data);

    // Pass data to sub-dissectors
    packet_dissector_next(self, packet, data);

    return data;
}

static void
packet_dissector_televt_free(Packet *packet)
{
    PacketTelEvtData *televt_data = packet_televt_data(packet);
    g_return_if_fail(televt_data != NULL);
    g_free(televt_data);
}

static void
packet_dissector_televt_class_init(PacketDissectorTelEvtClass *klass)
{
    PacketDissectorClass *dissector_class = PACKET_DISSECTOR_CLASS(klass);
    dissector_class->dissect = packet_dissector_televt_dissect;
    dissector_class->free_data = packet_dissector_televt_free;
}

static void
packet_dissector_televt_init(G_GNUC_UNUSED PacketDissectorTelEvt *self)
{
}

PacketDissector *
packet_dissector_televt_new()
{
    return g_object_new(
            PACKET_DISSECTOR_TYPE_DTMF,
            "id", PACKET_PROTO_TELEVT,
            "name", "DTMF",
            NULL
    );
}
