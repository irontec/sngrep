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
 * @file ui_mouse.c
 *
 * @brief Mouse support shared by every panel
 */
#include "config.h"
#include <limits.h>
#include <string.h>
#include "ui_mouse.h"
#include "setting.h"

static MEVENT last;

void
ui_mouse_setup(ui_t *ui)
{
#ifdef NCURSES_MOUSE_VERSION
    static mmask_t current = 0;
    mmask_t mask = BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | BUTTON4_PRESSED;
#if NCURSES_MOUSE_VERSION > 1
    mask |= BUTTON5_PRESSED;
#endif
    // Without mouse events the terminal keeps its own text selection
    if (!setting_enabled(SETTING_MOUSE))
        mask = 0;
    // Raw panel has no mouse support, so its messages can be selected
    if (ui && ui->type == PANEL_CALL_RAW)
        mask = 0;
    // Only notify the terminal when the mask changes
    if (mask != current) {
        mousemask(mask, NULL);
        current = mask;
    }
#endif
}

bool
ui_mouse_read()
{
    memset(&last, 0, sizeof(last));
#ifdef NCURSES_MOUSE_VERSION
    return getmouse(&last) == OK;
#else
    return false;
#endif
}

int
ui_mouse_translate(ui_t *ui)
{
    int i, y, x;

    if (!ui_mouse_read())
        return ERR;

#ifdef NCURSES_MOUSE_VERSION
    if (last.bstate & BUTTON4_PRESSED)
        return KEY_UP;
#if NCURSES_MOUSE_VERSION > 1
    if (last.bstate & BUTTON5_PRESSED)
        return KEY_DOWN;
#endif
#endif

    // Footer key hints behave like pressing the key
    if (ui_mouse_pos(ui->win, &y, &x) && y == ui->height - 1) {
        for (i = 0; i < ui->bindingcnt; i++) {
            if (x >= ui->bindings[i].start && x < ui->bindings[i].end)
                return ui->bindings[i].key;
        }
    }

    return KEY_MOUSE;
}

bool
ui_mouse_pos(WINDOW *win, int *y, int *x)
{
    *y = last.y;
    *x = last.x;
#ifdef NCURSES_MOUSE_VERSION
    return win && wmouse_trafo(win, y, x, FALSE);
#else
    return false;
#endif
}

bool
ui_mouse_double_click()
{
#ifdef NCURSES_MOUSE_VERSION
    return (last.bstate & BUTTON1_DOUBLE_CLICKED) != 0;
#else
    return false;
#endif
}

FIELD *
ui_mouse_form_field(FORM *form, bool *exact)
{
    FIELD **fields = form_fields(form), *left = NULL, *right = NULL;
    int i, y, x, page = 0, rows, cols, frow, fcol, nrow, nbuf;
    int lcol = -1, rcol = INT_MAX;

    if (exact)
        *exact = false;

    if (!fields || !ui_mouse_pos(form_sub(form), &y, &x))
        return NULL;

    for (i = 0; i < field_count(form); i++) {
        // Only fields of the displayed page can be clicked
        if (i && new_page(fields[i]))
            page++;
        if (page != form_page(form))
            continue;
        if ((field_opts(fields[i]) & (O_ACTIVE | O_VISIBLE)) != (O_ACTIVE | O_VISIBLE))
            continue;

        field_info(fields[i], &rows, &cols, &frow, &fcol, &nrow, &nbuf);
        if (y < frow || y >= frow + rows)
            continue;

        if (x >= fcol && x < fcol + cols) {
            if (exact)
                *exact = true;
            return fields[i];
        }
        if (fcol < x && fcol > lcol) {
            left = fields[i];
            lcol = fcol;
        }
        if (fcol > x && fcol < rcol) {
            right = fields[i];
            rcol = fcol;
        }
    }

    return left ? left : right;
}
