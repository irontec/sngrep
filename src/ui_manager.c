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
#include "curses.h"
#include "ui_manager.h"
#include "ui_call_list.h"
#include "ui_call_flow.h"
#include "ui_call_flow_ex.h"

/**
 * @brief Interface status data
 */
static struct ui_status {
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
static ui_panel_t panel_pool[] = {
{
    .type = MAIN_PANEL,
    .panel = NULL,
    .create = call_list_create,
    .draw = call_list_draw,
    .handle_key = call_list_handle_key,
    .help = call_list_help
},
{
    .type = DETAILS_PANEL,
    .panel = NULL,
    .create = call_flow_create,
    .draw = call_flow_draw,
    .handle_key = call_flow_handle_key,
    .help = call_flow_help
},
{
    .type = DETAILS_PANEL_EX,
    .panel = NULL,
    .create = call_flow_ex_create,
    .draw = call_flow_ex_draw,
    .handle_key = call_flow_ex_handle_key,
    .help = call_flow_ex_help
}};

/**
 * Initialize ncurses mode and create a main window
 * 
 * @param ui_config UI configuration structure
 * @returns 0 on ncurses initialization success, 1 otherwise 
 */
int init_interface(const struct ui_config uicfg)
{
    // Initialize curses 
    initscr();
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
    // Fixme for a wrapper ui_panel_show(ui_panel_t*);
    panel_pool->panel = panel_pool->create();
    wait_for_input(panel_pool);
    
    // End ncurses mode
    endwin();
}

/**
 * Toggle color mode on and off
 * @param on Pass 0 to turn all black&white
 */
void toggle_color(int on)
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


/**
 * Wait for user input.
 * This function manages all user input in all panel types and
 * redraws the panel using its own draw function
 * 
 * @param panel the topmost panel
 */
void wait_for_input(ui_panel_t *ui_panel)
{

    PANEL *panel = ui_panel->panel;
    // Get window of main panel
    WINDOW *win = panel_window(panel);
    keypad(win, TRUE);

    for (;;) {
        // Put this panel on top
        top_panel(panel);

        // Paint the panel 
        ui_panel->draw(panel);

        update_panels(); // Update the stacking order
        doupdate(); // Refresh screen

        int c = wgetch(win);
        switch (c) {
        case 'C':
        case 'c':
            status.color = (status.color) ? 0 : 1;
            toggle_color(status.color);
            break;
        case 'H':
        case 'h':
        case 265: /* KEY_F1 */
            /* wrapper this shit */
            ui_panel->help(panel);
            break;
        case 'Q':
        case 'q':
        case 27: /* KEY_ESC */
            hide_panel(panel);
            return;
            break;
        default:
            ui_panel->handle_key(panel, c);
            break;
        }
    }
}
 
/**
 * Draw a box around passed windows with two bars (top and bottom)
 * of one line each.
 *
 * @param win Window to draw borders on
 */
void title_foot_box(WINDOW *win)
{
    int height, width;

    // Get window size
    getmaxyx(win, height, width);
    box(win, 0, 0);
    mvwaddch(win, 2, 0, ACS_LTEE);
    mvwhline(win, 2, 1, ACS_HLINE, width - 2);
    mvwaddch(win, 2, width-1, ACS_RTEE);
    mvwaddch(win, height-3, 0, ACS_LTEE);
    mvwhline(win, height-3, 1, ACS_HLINE, width - 2);
    mvwaddch(win, height-3, width-1, ACS_RTEE);

}

/**
 * This function is invocked asynchronously from the
 * ngrep exec thread to notify a new message of the giving
 * callid. If the UI is displaying this call or it's 
 * extended one, the topmost panel will be redraw again 
 *
 * @param callid Call-ID from the last received message
 */
void refresh_call_ui(const char *callid)
{
    ui_panel_t *ui_panel;
    PANEL *panel;

    // Get the topmost panel
    if ((panel = panel_below(NULL))) {
        // Get ui information for that panel
        if ((ui_panel = ui_get_panel(panel))){
            // Request the panel to be drawn again
            ui_panel->draw(panel);
            //! Refresh panel stack
            update_panels(); 
            doupdate(); 
        }
    }
}


ui_panel_t *ui_get_panel(PANEL *panel)
{
	int i;
	int panelcnt = sizeof(panel_pool)/sizeof(ui_panel_t);
	for (i=0; i <panelcnt; i++){
		if (panel_pool[i].panel == panel)
			return &panel_pool[i];
	}
}
