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
 * @file table.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_TABLE_H__
#define __SNGREP_TABLE_H__

#include <glib.h>
#include <glib-object.h>
#include "storage/group.h"
#include "tui/widgets/widget.h"
#include "tui/widgets/scrollbar.h"

G_BEGIN_DECLS

// Class declaration
#define TUI_TYPE_TABLE table_get_type()
G_DECLARE_FINAL_TYPE(Table, table, TUI, TABLE, SngWidget)

struct _Table
{
    //! Parent object attributes
    SngWidget parent;
    //! Table Columns (Pointers to Attributes)
    GPtrArray *columns;
    //! Table displayed rows
    GPtrArray *dcalls;
    //! Selected call in the list
    gint cur_idx;
    //! First displayed call in the list
    gint first_idx;
    //! Selected table rows
    CallGroup *group;
    //! Move to last list entry if autoscroll is enabled
    gboolean autoscroll;
    //! List vertical scrollbar
    Scrollbar vscroll;
    //! List horizontal scrollbar
    Scrollbar hscroll;
};

SngWidget *
table_new();

void
table_set_rows(Table *table, GPtrArray *rows);

GPtrArray *
table_get_rows(Table *table);

void
table_columns_update(Table *table);

CallGroup *
table_get_call_group(Table *table);

Call *
table_get_current_call(Table *table);

/**
 * @brief Get List line from the given call
 *
 * Get the list line of the given call to display in the list
 * This line is built using the configured columns and sizes
 *
 * @param table Call List table
 * @param call Call to get data from
 * @return A pointer to text
 */
const gchar *
table_get_line_for_call(Table *table, Call *call);

G_END_DECLS

#endif    /* __SNGREP_TABLE_H__ */
