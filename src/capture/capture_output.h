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
 * @file capture_output.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to define capture output functions
 *
 */
#ifndef __SNGREP_CAPTURE_OUTPUT_H__
#define __SNGREP_CAPTURE_OUTPUT_H__

#include <glib.h>
#include <glib-object.h>
#include "capture/capture.h"

G_BEGIN_DECLS

#define CAPTURE_TYPE_OUTPUT capture_output_get_type()

G_DECLARE_DERIVABLE_TYPE(CaptureOutput, capture_output, CAPTURE, OUTPUT, GObject)

struct _CaptureOutputClass
{
    GObjectClass parent_class;

    //! Dump packet function
    void (*write)(CaptureOutput *self, Packet *packet);

    //! Close dump packet function
    void (*close)(CaptureOutput *self);
};

/*
 * Instance base methods
 */
void
capture_output_unref(CaptureOutput *self);

void
capture_output_write(CaptureOutput *self, Packet *packet);

void
capture_output_close(CaptureOutput *self);

/*
 * Setters/Getters
 */
void
capture_output_set_manager(CaptureOutput *self, CaptureManager *manager);

CaptureManager *
capture_output_manager(CaptureOutput *self);

void
capture_output_set_tech(CaptureOutput *self, CaptureTech tech);

CaptureTech
capture_output_tech(CaptureOutput *self);

void
capture_output_set_sink(CaptureOutput *self, const gchar *sink);

const gchar *
capture_output_sink(CaptureOutput *self);

G_END_DECLS

#endif /* __SNGREP_CAPTURE_OUTPUT_H__ */
