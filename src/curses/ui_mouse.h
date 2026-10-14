/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ****************************************************************************/
/**
 * @file ui_mouse.h
 *
 * @brief Mouse support shared by every panel
 *
 * The input loop reads each mouse event once. Wheel events become arrow keys
 * and clicks on footer key hints become that key, so panels only need to
 * handle KEY_MOUSE for clicks inside their own content.
 */
#ifndef SNGREP_UI_MOUSE_H
#define SNGREP_UI_MOUSE_H

#include <stdbool.h>
#include <form.h>
#include "ui_panel.h"

/**
 * @brief Enable or disable mouse events for the given panel
 *
 * Mouse events are enabled according to the mouse setting, except
 * for panels without mouse support.
 *
 * @param ui UI structure pointer of the current panel
 */
void
ui_mouse_setup(ui_t *ui);

/**
 * @brief Read the pending mouse event
 *
 * Must be called once after wgetch returns KEY_MOUSE.
 * @return true if a mouse event was read
 */
bool
ui_mouse_read();

/**
 * @brief Translate the pending mouse event into a key for the given panel
 *
 * @return KEY_UP/KEY_DOWN for the wheel, the bound key for a footer hint,
 * KEY_MOUSE for other clicks or ERR if there was no event
 */
int
ui_mouse_translate(ui_t *ui);

/**
 * @brief Last mouse position relative to a window
 *
 * @return false if the last click was outside the window
 */
bool
ui_mouse_pos(WINDOW *win, int *y, int *x);

/**
 * @brief Check if the last mouse event was a double click
 */
bool
ui_mouse_double_click();

/**
 * @brief Find the active field under the last click
 *
 * Clicks on a field row outside any field select the closest field to their
 * left (or right), so clicking a label picks the field it describes.
 *
 * @param exact set to true when the click was inside the field (may be NULL)
 * @return the field or NULL if the click was not on a field row
 */
FIELD *
ui_mouse_form_field(FORM *form, bool *exact);

#endif /* SNGREP_UI_MOUSE_H */
