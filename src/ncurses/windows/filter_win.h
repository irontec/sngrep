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
 * @file ui_filter.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ui window for filtering options
 *
 * This file contains the functions and structures to manage the filter
 * dialog, that can be used to filter the lines in call list window.
 */

#ifndef __SNGREP_FILTER_WIN_H
#define __SNGREP_FILTER_WIN_H

#include "config.h"
#include <form.h>
#include "ncurses/manager.h"

/**
 * @brief Enum of available dialog fields
 *
 * Dialog form has a field array. Following enum represents the
 * order this fields are stored in panel info structure.
 *
 */
enum FilterWinFields {
    FLD_FILTER_SIPFROM = 0,
    FLD_FILTER_SIPTO,
    FLD_FILTER_SRC,
    FLD_FILTER_DST,
    FLD_FILTER_PAYLOAD,
    FLD_FILTER_REGISTER,
    FLD_FILTER_INVITE,
    FLD_FILTER_SUBSCRIBE,
    FLD_FILTER_NOTIFY,
    FLD_FILTER_INFO,
    FLD_FILTER_OPTIONS,
    FLD_FILTER_PUBLISH,
    FLD_FILTER_MESSAGE,
    FLD_FILTER_REFER,
    FLD_FILTER_UPDATE,
    FLD_FILTER_FILTER,
    FLD_FILTER_CANCEL,
    FLD_FILTER_COUNT
};

//! Sorter declaration of struct _FilterWinInfo
typedef struct _FilterWinInfo FilterWinInfo;

/**
 * @brief Filter panel private information
 *
 * This structure contains the durable data of filter panel.
 */
struct _FilterWinInfo {
    //! Form that contains the filter fields
    FORM *form;
    //! An array of fields
    FIELD *fields[FLD_FILTER_COUNT + 1];
};

/**
 * @brief Creates a new filter panel
 *
 * This function allocates all required memory for
 * displaying the filter panel. It also draws all the
 * static information of the panel that will never be
 * redrawn.
 */
Window *
filter_win_new();

/**
 * @brief Destroy filter panel
 *
 * This function do the final cleanups for this panel
 * @param ui UI structure pointer
 */
void
filter_win_free(Window *ui);

#endif /* __SNGREP_FILTER_WIN_H */
