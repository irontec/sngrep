/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
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
 * @file capture_txt.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage saving packets to TXT files
 *
 */
#ifndef __SNGREP_CAPTURE_TXT_H__
#define __SNGREP_CAPTURE_TXT_H__

#include <glib.h>
#include <glib-object.h>
#include "capture/capture_output.h"

G_BEGIN_DECLS

#define CAPTURE_TYPE_OUTPUT_TXT capture_output_txt_get_type()

G_DECLARE_FINAL_TYPE(CaptureOutputTxt, capture_output_txt, CAPTURE, OUTPUT_TXT, CaptureOutput)

//! Error reporting
#define CAPTURE_TXT_ERROR (capture_txt_error_quark())

//! Error codes
typedef enum
{
    CAPTURE_TXT_ERROR_OPEN = 0,
} CaptureTxtErrors;

/**
 * @brief store all information related with packet capture
 *
 * Store capture required data from one packet source
 */
struct _CaptureOutputTxt
{
    //! Parent object attributes
    CaptureOutput parent;
    //! File where packets will be saved
    FILE *file;
};

/**
 * @brief Open a new txt for saving packets
 */
CaptureOutput *
capture_output_txt(const gchar *filename, GError **error);

G_END_DECLS

#endif /* __SNGREP_CAPTURE_TXT_H__ */
