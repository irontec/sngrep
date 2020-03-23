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
#include "storage/storage.h"
#include "dissector.h"

enum
{
    PROP_PROTOCOL_ID = 1,
    PROP_DISSECTOR_NAME,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

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

void
packet_dissector_add_subdissector(PacketDissector *self, PacketProtocolId id)
{
    PacketDissectorPrivate *priv = packet_dissector_get_instance_private(self);
    priv->subdissectors = g_slist_append(priv->subdissectors, GINT_TO_POINTER(id));
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
    PacketDissector *dissector = storage_find_dissector(id);
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

        data = packet_dissector_next_proto(GPOINTER_TO_INT(l->data), packet, data);

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
    PacketDissectorPrivate *priv = packet_dissector_get_instance_private(PACKET_DISSECTOR(self));
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
            priv->name = g_value_get_string(value);
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
