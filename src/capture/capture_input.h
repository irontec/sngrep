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
 * @file capture_input.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to define capture input functions
 *
 */
#ifndef __SNGREP_CAPTURE_INPUT_H__
#define __SNGREP_CAPTURE_INPUT_H__

#include <glib.h>
#include <glib-object.h>
#include "capture.h"
#include "packet/dissector.h"
#include "packet/packet.h"

G_BEGIN_DECLS

#define CAPTURE_TYPE_INPUT capture_input_get_type()

G_DECLARE_DERIVABLE_TYPE(CaptureInput, capture_input, CAPTURE, INPUT, GObject)

struct _CaptureInputClass
{
    GObjectClass parent_class;

    //! Start capturing packets function
    gpointer (*start)(CaptureInput *self);

    //! Stop capturing packets function
    void (*stop)(CaptureInput *);

    //! Capture filtering function
    gint (*filter)(CaptureInput *, const gchar *, GError **);
};

/*
 * Instance base methods
 */
void
capture_input_unref(CaptureInput *self);

gpointer
capture_input_start(CaptureInput *self);

void
capture_input_stop(CaptureInput *self);

gint
capture_input_filter(CaptureInput *self, const gchar *filter, GError **error);

/*
 * Setters/Getters
 */
void
capture_input_set_source(CaptureInput *self, GSource *source);

GSource *
capture_input_source(CaptureInput *self);

void
capture_input_set_mode(CaptureInput *self, CaptureMode mode);

CaptureMode
capture_input_mode(CaptureInput *self);

void
capture_input_set_tech(CaptureInput *self, CaptureTech tech);

CaptureTech
capture_input_tech(CaptureInput *self);

void
capture_input_set_source_str(CaptureInput *self, const gchar *source_str);

const gchar *
capture_input_source_str(CaptureInput *self);

void
capture_input_set_total_size(CaptureInput *self, guint64 size);

guint64
capture_input_total_size(CaptureInput *self);

void
capture_input_set_loaded_size(CaptureInput *self, guint64 loaded);

guint64
capture_input_loaded_size(CaptureInput *self);

void
capture_input_set_initial_protocol(CaptureInput *self, PacketDissector *dissector);

PacketDissector *
capture_input_initial_protocol(CaptureInput *self);

G_END_DECLS

#endif /* __SNGREP_CAPTURE_INPUT_H__ */
