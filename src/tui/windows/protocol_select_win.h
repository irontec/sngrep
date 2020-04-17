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
 * @file protocol_select_win.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage protocol select panel
 */

#ifndef __SNGREP_PROTOCOL_SELECT_WIN_H__
#define __SNGREP_PROTOCOL_SELECT_WIN_H__

#include <menu.h>
#include <form.h>
#include "tui/widgets/window.h"
#include "tui/tui.h"
#include "tui/widgets/scrollbar.h"
#include "storage/attribute.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_PROTOCOL_SELECT protocol_select_win_get_type()
G_DECLARE_FINAL_TYPE(ProtocolSelectWindow, protocol_select_win, TUI, PROTOCOL_SELECT, SngAppWindow)

/**
 * @brief Enum of available fields
 */
typedef enum
{
    FLD_PROTOCOLS_SAVE = 0,
    FLD_PROTOCOLS_CANCEL,
    FLD_PROTOCOLS_COUNT
} ProtocolSelectFields;

/**
 * @brief Protocol selector panel private information
 *
 * This structure contains the durable data of protocol selection panel.
 */
struct _ProtocolSelectWindow
{
    //! Parent object attributes
    SngAppWindow parent;
    //! Section of panel where menu is being displayed
    WINDOW *menu_win;
    //! Columns menu
    MENU *menu;
    // Columns Items
    ITEM *items[PACKET_PROTO_COUNT + 1];
    //! Current selected protocol
    GPtrArray *selected;
    //! Form that contains the save fields
    FORM *form;
    //! An array of window form fields
    FIELD *fields[FLD_PROTOCOLS_COUNT + 1];
    //! Flag to handle key inputs
    gboolean form_active;
    //! Scrollbar for the menu window
    Scrollbar scroll;
};

/**
 * @brief Creates a new protocol selection window
 *
 * This function allocates all required memory for
 * displaying the protocol selection window. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 *
 * @return Window UI structure pointer
 */
SngAppWindow *
protocol_select_win_new();

G_END_DECLS

#endif /* __SNGREP_PROTOCOL_SELECT_WIN_H__ */
