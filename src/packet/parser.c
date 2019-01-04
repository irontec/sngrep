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
 * @file parser.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage captured packet parsers
 *
 */

#include "config.h"
#include <glib.h>
#include "glib-extra.h"
#include "dissector.h"
#include "packet/dissectors/packet_link.h"
#include "packet/dissectors/packet_ip.h"
#include "packet/dissectors/packet_udp.h"
#include "packet/dissectors/packet_tcp.h"
#include "packet/dissectors/packet_sip.h"
#include "packet/dissectors/packet_sdp.h"
#include "packet/dissectors/packet_rtp.h"
#include "packet/dissectors/packet_rtcp.h"
#ifdef WITH_SSL
#include "packet/dissectors/packet_tls.h"
#endif
#ifdef USE_HEP
#include "packet/dissectors/packet_hep.h"
#endif
#include "parser.h"

PacketParser *
packet_parser_new(CaptureInput *input)
{
    PacketParser *parser = g_malloc0(sizeof(PacketParser));
    parser->input = input;

    // Created Protos array
    parser->protos = g_ptr_array_sized_new(PACKET_PROTO_COUNT);
    g_ptr_array_set_size(parser->protos, PACKET_PROTO_COUNT);

    // Created Dissectors information array
    parser->dissectors = g_ptr_array_sized_new(PACKET_PROTO_COUNT);
    g_ptr_array_set_size(parser->dissectors, PACKET_PROTO_COUNT);

    // Dissectors tree root
    parser->dissector_tree = g_node_new(NULL);
    return parser;
}

static void
packet_parser_proto_deinit(PacketDissector *dissector, PacketParser *parser)
{
    if (dissector != NULL) {
        if (dissector->deinit != NULL)
            dissector->deinit(parser);
        g_slist_free(dissector->subdissectors);
        g_free(dissector);
    }
}

void
packet_parser_free(PacketParser *parser)
{

    g_ptr_array_foreach(parser->protos, (GFunc) packet_parser_proto_deinit, parser);
    g_ptr_array_free(parser->protos, TRUE);
    g_ptr_array_free(parser->dissectors, TRUE);
    g_node_destroy(parser->dissector_tree);
    g_free(parser);
}

PacketDissector *
packet_parser_add_proto(PacketParser *parser, GNode *parent, enum PacketProtoId id)
{
    PacketDissector *dissector = g_ptr_array_index(parser->protos, id);

    if (dissector == NULL) {
        switch (id) {
            case PACKET_LINK:
                dissector = packet_link_new();
                break;
            case PACKET_IP:
                dissector = packet_ip_new();
                break;
            case PACKET_UDP:
                dissector = packet_udp_new();
                break;
            case PACKET_TCP:
                dissector = packet_tcp_new();
                break;
            case PACKET_SIP:
                dissector = packet_sip_new();
                break;
            case PACKET_SDP:
                dissector = packet_sdp_new();
                break;
            case PACKET_RTP:
                dissector = packet_rtp_new();
                break;
            case PACKET_RTCP:
                dissector = packet_rtcp_new();
                break;
#ifdef USE_HEP
            case PACKET_HEP:
                dissector = packet_hep_new();
                break;
#endif
#ifdef WITH_SSL
            case PACKET_TLS:
                dissector = packet_tls_new();
                break;
#endif
            default:
                // Unsuported protocol id
                return NULL;
        }

        // Add to proto list
        g_ptr_array_set(parser->protos, id, dissector);

        // Initialize protocol data
        if (dissector->init) {
            dissector->init(parser);
        }
    }

    // Append this disector to the tree
    GNode *node = g_node_new(dissector);
    g_node_append(parent, node);

    // Add children dissectors
    for (GSList *l = dissector->subdissectors; l != NULL; l = l->next) {
        packet_parser_add_proto(parser, node, GPOINTER_TO_UINT(l->data));
    }

    return dissector;
}

GByteArray *
packet_parser_next_dissector(PacketParser *parser, Packet *packet, GByteArray *data)
{
    // No more dissection required
    if (data == NULL)
        return NULL;

    // Get current dissector node
    GNode *current = parser->current;

    // Call each subdissectors until data is parsed (and it returns NULL)
    for (guint i = 0; i < g_node_n_children(current); i++) {
        // Update current dissector node
        parser->current = g_node_nth_child(current, i);

        // Get dissector data
        PacketDissector *dissector = parser->current->data;

        // Dissect pending data
        if (dissector->dissect) {
            data = dissector->dissect(parser, packet, data);
            // All data dissected, we're done
            if (data == NULL) {
                break;
            }
        }
    }

    return data;
}

