/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
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
 * @file ui_media_dashboard.h
 * @author Mohammad Bagher Amini <darwvin@hotmail.com>
 *
 * @brief Functions to manage the Media Inspector dashboard panel
 */
#ifndef SNGREP_UI_MEDIA_DASHBOARD_H
#define SNGREP_UI_MEDIA_DASHBOARD_H

#include <time.h>
#include "ui_manager.h"
#include "media_inspector.h"

//! Sorter declaration of struct media_dashboard_info
typedef struct media_dashboard_info media_dashboard_info_t;

/**
 * @brief Media dashboard panel status information
 */
struct media_dashboard_info
{
    //! Selected stream row
    size_t selected;
    //! First displayed stream row
    size_t offset;
    //! Sort by estimated loss instead of first seen time
    int order_by_loss;
    //! Last time the panel was redrawn
    time_t last_render;
};

/**
 * @brief Compare two flows for display ordering
 *
 * Flows are kept in first seen order so rows do not move between
 * refreshes. Loss ordering sorts by estimated loss first.
 *
 * @param left First flow to compare
 * @param right Second flow to compare
 * @return qsort compatible comparison result
 */
int
media_dashboard_cmp_flows(const void *left, const void *right);

/**
 * @brief Draw a text with the given attributes
 *
 * @param win Window to draw into
 * @param y Line of the text
 * @param x Column of the text
 * @param text Text to draw
 * @param attr Attributes to use
 */
void
media_dashboard_label(WINDOW *win, int y, int x, const char *text, int attr);

/**
 * @brief Return the display state of a flow
 *
 * @param flow Flow to check
 * @param now Current time
 * @return State label for the flow
 */
const char *
media_dashboard_state(const media_inspector_flow_t *flow, time_t now);

/**
 * @brief Create the media dashboard panel
 *
 * @param ui UI structure pointer
 */
void
media_dashboard_create(ui_t *ui);

/**
 * @brief Destroy the media dashboard panel
 *
 * @param ui UI structure pointer
 */
void
media_dashboard_destroy(ui_t *ui);

/**
 * @brief Resize the media dashboard panel to the terminal size
 *
 * @param ui UI structure pointer
 * @return 0 always
 */
int
media_dashboard_resize(ui_t *ui);

/**
 * @brief Check if the panel requires to be redrawn
 *
 * The panel is redrawn once per second.
 *
 * @param ui UI structure pointer
 * @return true if the panel must be redrawn, false otherwise
 */
bool
media_dashboard_redraw(ui_t *ui);

/**
 * @brief Draw the media dashboard panel
 *
 * @param ui UI structure pointer
 * @return 0 always
 */
int
media_dashboard_draw(ui_t *ui);

/**
 * @brief Handle media dashboard key strokes
 *
 * @param ui UI structure pointer
 * @param key Pressed keycode
 * @return enum @key_handler_ret
 */
int
media_dashboard_handle_key(ui_t *ui, int key);

#endif /* SNGREP_UI_MEDIA_DASHBOARD_H */
