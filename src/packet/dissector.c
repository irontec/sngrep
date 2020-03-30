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
 * @file dissector.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in dissector.h
 *
 */
#include "config.h"
#include <glib.h>
#include "glib-extra/glib_enum_types.h"
#include "glib-extra/glib.h"
#include "storage/storage.h"
#include "setting.h"
#include "packet/packet_link.h"
#include "packet/packet_ip.h"
#include "packet/packet_udp.h"
#include "packet/packet_tcp.h"
#include "packet/packet_sip.h"
#include "packet/packet_sdp.h"
#include "packet/packet_rtp.h"
#include "packet/packet_rtcp.h"
#ifdef USE_HEP
#include "packet/packet_hep.h"
#endif
#ifdef WITH_SSL
#include "packet/packet_tls.h"
#endif
#include "dissector.h"

enum
{
    PROP_PROTOCOL_ID = 1,
    PROP_DISSECTOR_NAME,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

//! Available Dissectors array
GPtrArray *dissectors = NULL;

typedef struct
{
    //! Protocol id
    PacketProtocolId id;
    //! Packet dissector name
    const gchar *name;
    //! SubProtocol children dissectors
    GSList *subdissectors;
} PacketDissectorPrivate;

// PacketDissector class definition
G_DEFINE_TYPE_WITH_PRIVATE(PacketDissector, packet_dissector, G_TYPE_OBJECT)


PacketDissector *
packet_dissector_find_by_id(PacketProtocolId id)
{
    // Initialize dissectors cache
    if (dissectors == NULL) {
        dissectors = g_ptr_array_sized_new(PACKET_PROTO_COUNT);
        g_ptr_array_set_size(dissectors, PACKET_PROTO_COUNT);
    }

    PacketDissector *dissector = g_ptr_array_index(dissectors, id);
    if (dissector == NULL) {
        switch (id) {
            case PACKET_PROTO_LINK:
                dissector = packet_dissector_link_new();
                break;
            case PACKET_PROTO_IP:
                dissector = packet_dissector_ip_new();
                break;
            case PACKET_PROTO_UDP:
                dissector = packet_dissector_udp_new();
                break;
            case PACKET_PROTO_TCP:
                dissector = packet_dissector_tcp_new();
                break;
            case PACKET_PROTO_SIP:
                dissector = packet_dissector_sip_new();
                break;
            case PACKET_PROTO_SDP:
                dissector = packet_dissector_sdp_new();
                break;
            case PACKET_PROTO_RTP:
                dissector = packet_dissector_rtp_new();
                break;
            case PACKET_PROTO_RTCP:
                dissector = packet_dissector_rtcp_new();
                break;
#ifdef USE_HEP
            case PACKET_PROTO_HEP:
                dissector = packet_dissector_hep_new();
                break;
#endif
#ifdef WITH_SSL
            case PACKET_PROTO_TLS:
                dissector = packet_dissector_tls_new();
                break;
#endif
            default:
                // Unsupported protocol id
                return NULL;
        }

        // Ignore not enabled dissectors
        if (dissector == NULL)
            return NULL;

        // Add to proto list
        g_ptr_array_set(dissectors, id, dissector);
    }

    return dissector;
}

gboolean
packet_dissector_enabled(PacketProtocolId id)
{
    switch (id) {
        case PACKET_PROTO_IP:
            return setting_enabled(SETTING_PACKET_IP);
        case PACKET_PROTO_UDP:
            return setting_enabled(SETTING_PACKET_UDP);
        case PACKET_PROTO_TCP:
            return setting_enabled(SETTING_PACKET_TCP);
        case PACKET_PROTO_SIP:
            return setting_enabled(SETTING_PACKET_SIP);
        case PACKET_PROTO_SDP:
            return setting_enabled(SETTING_PACKET_SDP);
        case PACKET_PROTO_RTP:
            return setting_enabled(SETTING_PACKET_RTP);
        case PACKET_PROTO_RTCP:
            return setting_enabled(SETTING_PACKET_RTCP);
#ifdef USE_HEP
        case PACKET_PROTO_HEP:
            return setting_enabled(SETTING_PACKET_HEP);
#endif
#ifdef WITH_SSL
        case PACKET_PROTO_TLS:
            return setting_enabled(SETTING_PACKET_TLS);
#endif
        default:
            return TRUE;
    }
}

void
packet_dissector_add_subdissector(PacketDissector *self, PacketProtocolId id)
{
    PacketDissectorPrivate *priv = packet_dissector_get_instance_private(self);
    if (packet_dissector_enabled(id)) {
        priv->subdissectors = g_slist_append(
            priv->subdissectors,
            packet_dissector_find_by_id(id)
        );
    }
}

GBytes *
packet_dissector_dissect(PacketDissector *self, Packet *packet, GBytes *data)
{
    g_return_val_if_fail (PACKET_IS_DISSECTOR(self), NULL);

    PacketDissectorClass *klass = PACKET_DISSECTOR_GET_CLASS(self);
    g_return_val_if_fail (klass->dissect != NULL, NULL);

    return klass->dissect(self, packet, data);
}

void
packet_dissector_free_data(PacketDissector *self, Packet *packet)
{
    g_return_if_fail (PACKET_IS_DISSECTOR(self));

    PacketDissectorClass *klass = PACKET_DISSECTOR_GET_CLASS(self);
    if (klass->free_data) {
        klass->free_data(packet);
    }
}

GBytes *
packet_dissector_next_proto(PacketProtocolId id, Packet *packet, GBytes *data)
{
    PacketDissector *dissector = packet_dissector_find_by_id(id);
    if (dissector != NULL) {
        return packet_dissector_dissect(dissector, packet, data);
    } else {
        return data;
    }
}

GBytes *
packet_dissector_next(PacketDissector *current, Packet *packet, GBytes *data)
{
    // No more dissection required
    if (data == NULL)
        return NULL;

    PacketDissectorPrivate *priv = packet_dissector_get_instance_private(current);
    // Call each sub-dissectors until data is parsed (and it returns NULL)
    for (GSList *l = priv->subdissectors; l != NULL; l = l->next) {
        data = packet_dissector_dissect(l->data, packet, data);
        // All data dissected, we're done
        if (data == NULL) {
            break;
        }
    }

    return data;
}

const gchar *
packet_dissector_get_name(PacketDissector *self)
{
    PacketDissectorPrivate *priv = packet_dissector_get_instance_private(self);
    g_return_val_if_fail(priv != NULL, NULL);
    return priv->name;
}

static void
packet_dissector_finalize(GObject *self)
{
    PacketDissectorPrivate *priv = packet_dissector_get_instance_private(PACKET_DISSECTOR(self));
    g_slist_free(priv->subdissectors);
    G_OBJECT_CLASS (packet_dissector_parent_class)->finalize(self);
}

static void
packet_dissector_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    PacketDissectorPrivate *priv = packet_dissector_get_instance_private(PACKET_DISSECTOR(self));

    switch (property_id) {
        case PROP_PROTOCOL_ID:
            priv->id = g_value_get_enum(value);
            break;
        case PROP_DISSECTOR_NAME:
            priv->name = g_value_dup_string(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
packet_dissector_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    PacketDissectorPrivate *priv = packet_dissector_get_instance_private(PACKET_DISSECTOR(self));

    switch (property_id) {
        case PROP_PROTOCOL_ID:
            g_value_set_enum(value, priv->id);
            break;
        case PROP_DISSECTOR_NAME:
            g_value_set_static_string(value, priv->name);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
packet_dissector_class_init(PacketDissectorClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = packet_dissector_finalize;
    object_class->set_property = packet_dissector_set_property;
    object_class->get_property = packet_dissector_get_property;

    obj_properties[PROP_PROTOCOL_ID] =
        g_param_spec_enum("id",
                          "ProtocolId",
                          "Protocol Identifier from PacketProtocolId enum",
                          PACKET_TYPE_PROTOCOL_ID,
                          0,
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    obj_properties[PROP_DISSECTOR_NAME] =
        g_param_spec_string("name",
                            "DissectorName",
                            "Dissector name",
                            NULL,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}

static void
packet_dissector_init(G_GNUC_UNUSED PacketDissector *self)
{
}
