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
 * @file dialog.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Common process for modal dialogs
 *
 */

#ifndef __SNGREP_DIALOG_H
#define __SNGREP_DIALOG_H

#include <glib.h>
#include "tui/widgets/window.h"

//! Default dialog dimensions
#define DIALOG_MAX_WIDTH 100
#define DIALOG_MIN_WIDTH 40

/**
 * @brief Draw a centered dialog with a message
 *
 * Create a centered dialog with a message.
 * @param msg Message to be drawn
 */
int
dialog_run(const char *fmt, ...);

/**
 * @brief Create a new progress bar dialog
 *
 * Create a new progress bar dialog with the given text. The returned
 * pointer should be used as parameter for @dialog_progress_set_value
 * in order to move the progress bar percentage.
 *
 * @param fmt, vaarg Text to be displayed above the progress bar
 * @return a pointer to the created window.
 */
WINDOW *
dialog_progress_run(const char *fmt, ...);

/**
 * @brief Set current percentage of dialog progress bar
 *
 * @param win Window pointer created with @dialog_progress_run
 * @param perc 0-100 percentage of progress bar
 */
void
dialog_progress_set_value(WINDOW *win, int perc);

/**
 * @brief Destroy a dialog created by @dialog_progress_run
 *
 * This function will deallocate all memory and close the
 * given window pointer.
 *
 * @param win Window pointer created with @dialog_progress_run
 */
void
dialog_progress_destroy(WINDOW *win);

/**
 * @brief Create a new confirmation dialog with multiple buttons
 *
 * This function can be used to create dialogs with multiple buttons to
 * request user confirmation. By default, the first given option will
 * be selected.
 *
 * @param title Title displayed in the top of the dialog
 * @param text Text displayed inside the dialog
 * @param options Comma separated labels for the different buttons
 * @return the index of the button pressed
 */
int
dialog_confirm(const char *title, const char *text, const char *options);


#endif // __SNGREP_DIALOG_H
