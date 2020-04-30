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
 * @file flow_column.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_FLOW_COLUMN_H__
#define __SNGREP_FLOW_COLUMN_H__

#include <glib.h>
#include <glib-object.h>
#include "storage/address.h"
#include "tui/widgets/widget.h"

G_BEGIN_DECLS

//! Configurable some day
#define CF_COLUMN_WIDTH 30

#define SNG_TYPE_FLOW_COLUMN sng_flow_column_get_type()
G_DECLARE_FINAL_TYPE(SngFlowColumn, sng_flow_column, SNG, FLOW_COLUMN, SngWidget)

/**
 * @brief Structure to hold one column information
 */
struct _SngFlowColumn
{
    //! Parent object attributes
    SngWidget parent;
    //! Address header for this column
    Address addr;
    //! Alias for the given address
    const gchar *alias;
    //! Twin column for externip setting
    SngFlowColumn *twin;
};

SngWidget *
sng_flow_column_new(Address address);

G_END_DECLS

#endif    /* __SNGREP_FLOW_COLUMN_H__ */
