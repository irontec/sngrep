/******************************************************************************
 **
 ** Copyright (C) 2011-2012 Irontec SL. All rights reserved.
 **
 ** This file may be used under the terms of the GNU General Public
 ** License version 3.0 as published by the Free Software Foundation
 ** and appearing in the file LICENSE.GPL included in the packaging of
 ** this file.  Please review the following information to ensure GNU
 ** General Public Licensing requirements will be met:
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ******************************************************************************/
#include "ui_manager.h"
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_flow_ex.h"
#include "ui_call_raw.h"

/**
 * @brief Interface status data
 * XXX I think this should be in the applicaton configuration structure
 *     avaliable everywhere in the program
 */
static struct ui_status
{
    int color;
} status;

/**
 * @brief Available panel windows list
 * This list contein the available list of windows
 * and pointer to their main functions.
 * XXX If the panel count increase a lot, it will be required to 
 *     load panels as modules and provide a way to register
 *     themselfs into the panel pool dynamically.
 */
static ui_t panel_pool[] = {
        {
                .type = MAIN_PANEL,
                .panel = NULL,
                .create = call_list_create,
                .redraw_required = call_list_redraw_required,
                .draw = call_list_draw,
                .handle_key = call_list_handle_key,
                .help = call_list_help,
                .destroy = call_list_destroy, },
        {
                .type = DETAILS_PANEL,
                .panel = NULL,
                .create = call_flow_create,
                .redraw_required = call_flow_redraw_required,
                .draw = call_flow_draw,
                .handle_key = call_flow_handle_key,
                .help = call_flow_help,
                .destroy = call_flow_destroy, },
        {
                .type = DETAILS_EX_PANEL,
                .panel = NULL,
                .create = call_flow_ex_create,
                .redraw_required = call_flow_ex_redraw_required,
                .draw = call_flow_ex_draw,
                .handle_key = call_flow_ex_handle_key,
                .help = call_flow_ex_help },
        {
                .type = RAW_PANEL,
                .panel = NULL,
                .create = call_raw_create,
                .redraw_required = call_raw_redraw_required,
                .draw = call_raw_draw,
                .handle_key = call_raw_handle_key,
                .help = call_raw_help } };

/**
 * Initialize ncurses mode and create a main window
 * 
 * @param ui_config UI configuration structure
 * @returns 0 on ncurses initialization success, 1 otherwise 
 */
int
init_interface(const struct ui_config uicfg)
{
    // Initialize curses 
    if (!initscr()) {
        fprintf(stderr, "Unable to initialize ncurses mode.\n");
        return -1;
    }
    cbreak();

    // Dont write user input on screen
    noecho();
    // Hide the cursor
    curs_set(0);
    // Only delay ESC Sequences 25 ms (we dont want Escape sequences)
    ESCDELAY = 25;
    start_color();
    toggle_color((status.color = 1));

    // Start showing call list 
    wait_for_input(ui_create(ui_find_by_type(MAIN_PANEL)));

    // End ncurses mode
    endwin();
    return 0;
}

ui_t *
ui_create(ui_t *ui)
{
    // If already has a panel, just return it
    if (ui_get_panel(ui)) return ui;

    // Otherwise, request the ui to create the panel
    if (ui->create) {
        ui->panel = ui->create();
    }

    // And return it
    return ui;
}

void
ui_destroy(ui_t *ui)
{
    PANEL *panel;
    // If thre is no ui panel, we're done
    if (!(panel = ui_get_panel(ui))) return;

    // If panel has a destructor function use it
    if (ui->destroy) ui->destroy(panel);

    // Initialize panel pointer
    ui->panel = NULL;
}

PANEL *
ui_get_panel(ui_t *ui)
{
    // Return panel pointer of ui struct
    return (ui) ? ui->panel : NULL;
}

int
ui_redraw_required(ui_t *ui, sip_msg_t *msg)
{
    //! Sanity check, this should not happen
    if (!ui) return -1;
    // Request the panel to draw on the scren
    if (ui->redraw_required) {
        return ui->redraw_required(ui_get_panel(ui), msg);
    }
    // If no redraw capabilities, never redraw
    return -1;
}

int
ui_draw_panel(ui_t *ui)
{
    //! Sanity check, this should not happen
    if (!ui) return -1;

    // Create the panel if it does not exist
    if (!(ui_create(ui))) return -1;

    // Make this panel the topmost panel
    top_panel(ui_get_panel(ui));

    // Request the panel to draw on the scren
    if (ui->draw) {
        if (ui->draw(ui_get_panel(ui)) == 0) {
            // Update panel stack
            update_panels();
            doupdate();
            return 0;
        }
    }
    return -1;
}

void
ui_help(ui_t *ui)
{
    // If current ui has help function
    if (ui->help) ui->help(ui_get_panel(ui));

    // Update the stacking order
    update_panels();
    doupdate();
    // Press any key to continue
    wgetch(panel_window(ui_get_panel(ui)));
}

int
ui_handle_key(ui_t *ui, int key)
{
    if (ui->handle_key) return ui->handle_key(ui_get_panel(ui), key);
    return -1;
}

ui_t *
ui_find_by_panel(PANEL *panel)
{
    int i;
    int panelcnt = sizeof(panel_pool) / sizeof(ui_t);
    for (i = 0; i < panelcnt; i++) {
        if (panel_pool[i].panel == panel) return &panel_pool[i];
    }
    return NULL;
}

ui_t *
ui_find_by_type(int type)
{
    int i;
    int panelcnt = sizeof(panel_pool) / sizeof(ui_t);
    for (i = 0; i < panelcnt; i++) {
        if (panel_pool[i].type == type) return &panel_pool[i];
    }
    return NULL;
}

int
wait_for_input(ui_t *ui)
{
    ui_t *replace;
    WINDOW *win;

    // Keep getting keys until panel is destroyed
    while (ui_get_panel(ui)) {
        // If ui requested replacement
        if (ui->replace) {
            replace = ui->replace;
            ui->replace = NULL;
            ui_destroy(ui);
            ui = replace;
        }

        if (ui_draw_panel(ui) != 0) return -1;

        // Enable key input on current panel
        win = panel_window(ui_get_panel(ui));
        keypad(win, TRUE);

        // Get pressed key
        int c = wgetch(win);

        // Check if current panel has custom bindings for that key
        if (ui_handle_key(ui, c) == 0) continue;

        // Otherwise, use standard keybindings
        switch (c) {
        case 'C':
        case 'c':
            // TODO general application config structure
            status.color = (status.color) ? 0 : 1;
            toggle_color(status.color);
            break;
        case 'H':
        case 'h':
        case 265: /* KEY_F1 */
            ui_help(ui);
            break;
        case 'Q':
        case 'q':
        case 27: /* KEY_ESC */
            ui_destroy(ui);
            return 0;
            break;
        }
    }

    return -1;
}

void
toggle_color(int on)
{
    if (on) {
        // Initialize some colors
        init_pair(HIGHLIGHT_COLOR, COLOR_WHITE, COLOR_BLUE);
        init_pair(HELP_COLOR, COLOR_CYAN, COLOR_BLACK);
        init_pair(OUTGOING_COLOR, COLOR_RED, COLOR_BLACK);
        init_pair(INCOMING_COLOR, COLOR_GREEN, COLOR_BLACK);
        init_pair(DETAIL_BORDER_COLOR, COLOR_BLUE, COLOR_BLACK);
    } else {
        init_pair(HIGHLIGHT_COLOR, COLOR_BLACK, COLOR_WHITE);
        init_pair(HELP_COLOR, COLOR_WHITE, COLOR_BLACK);
        init_pair(OUTGOING_COLOR, COLOR_WHITE, COLOR_BLACK);
        init_pair(INCOMING_COLOR, COLOR_WHITE, COLOR_BLACK);
        init_pair(DETAIL_BORDER_COLOR, COLOR_WHITE, COLOR_BLACK);
    }
}

void
ui_new_msg_refresh(sip_msg_t *msg)
{
    PANEL *panel;

    // Get the topmost panel
    if ((panel = panel_below(NULL))) {
        // Get ui information for that panel
        if (ui_redraw_required(ui_find_by_panel(panel), msg) == 0) {
            // If ui needs to be update, redraw it
            ui_draw_panel(ui_find_by_panel(panel));
        }
    }
}

void
title_foot_box(WINDOW *win)
{
    int height, width;

    // Get window size
    getmaxyx(win, height, width);
    box(win, 0, 0);
    mvwaddch(win, 2, 0, ACS_LTEE);
    mvwhline(win, 2, 1, ACS_HLINE, width - 2);
    mvwaddch(win, 2, width - 1, ACS_RTEE);
    mvwaddch(win, height - 3, 0, ACS_LTEE);
    mvwhline(win, height - 3, 1, ACS_HLINE, width - 2);
    mvwaddch(win, height - 3, width - 1, ACS_RTEE);

}

int
ui_set_replace(ui_t *original, ui_t *replace)
{
    if (!original || !replace) return -1;
    original->replace = replace;
    return 0;
}
