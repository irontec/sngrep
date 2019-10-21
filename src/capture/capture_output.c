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
 * @file capture_output.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in capture_output.h
 *
 */

#include "capture_output.h"

typedef struct
{
    //! Manager owner of this capture input
    CaptureManager *manager;
    //! Capture Output type
    CaptureTech tech;
    //! Sink string
    gchar *sink;
} CaptureOutputPrivate;

// CaptureOutput class definition
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(CaptureOutput, capture_output, G_TYPE_OBJECT)

void
capture_output_unref(CaptureOutput *self)
{
    g_object_unref(self);
}

void
capture_output_write(CaptureOutput *self, Packet *packet)
{
    g_return_if_fail(CAPTURE_IS_OUTPUT(self));

    CaptureOutputClass *klass = CAPTURE_OUTPUT_GET_CLASS(self);
    g_return_if_fail(klass->write != NULL);
    klass->write(self, packet);
}

void
capture_output_close(CaptureOutput *self)
{
    g_return_if_fail (CAPTURE_IS_OUTPUT(self));

    CaptureOutputClass *klass = CAPTURE_OUTPUT_GET_CLASS(self);
    g_return_if_fail (klass->close != NULL);
    klass->close(self);
}

void
capture_output_set_manager(CaptureOutput *self, CaptureManager *manager)
{
    CaptureOutputPrivate *priv = capture_output_get_instance_private(self);
    g_return_if_fail(priv != NULL);
    priv->manager = manager;
}

CaptureManager *
capture_output_manager(CaptureOutput *self)
{
    CaptureOutputPrivate *priv = capture_output_get_instance_private(self);
    g_return_val_if_fail(priv != NULL, NULL);
    return priv->manager;
}

void
capture_output_set_tech(CaptureOutput *self, CaptureTech tech)
{
    CaptureOutputPrivate *priv = capture_output_get_instance_private(self);
    g_return_if_fail(priv != NULL);
    priv->tech = tech;
}

CaptureTech
capture_output_tech(CaptureOutput *self)
{
    CaptureOutputPrivate *priv = capture_output_get_instance_private(self);
    g_return_val_if_fail(priv != NULL, CAPTURE_TECH_INVALID);
    return priv->tech;
}

void
capture_output_set_sink(CaptureOutput *self, const gchar *sourcestr)
{
    CaptureOutputPrivate *priv = capture_output_get_instance_private(self);
    g_return_if_fail(priv != NULL);
    priv->sink = g_strdup(sourcestr);
}

const gchar *
capture_output_sink(CaptureOutput *self)
{
    CaptureOutputPrivate *priv = capture_output_get_instance_private(self);
    g_return_val_if_fail(priv != NULL, NULL);
    return priv->sink;
}

static void
capture_output_finalize(GObject *object)
{
    CaptureOutputPrivate *priv = capture_output_get_instance_private(CAPTURE_OUTPUT(object));
    g_free(priv->sink);
    G_OBJECT_CLASS (capture_output_parent_class)->finalize(object);
}

static void
capture_output_class_init(CaptureOutputClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = capture_output_finalize;
}

static void
capture_output_init(G_GNUC_UNUSED CaptureOutput *self)
{
}
