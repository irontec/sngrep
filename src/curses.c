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
#include <pthread.h>
#include <string.h>
#include "curses.h"

extern struct sip_call *calls; //XXX
extern pthread_mutex_t calls_lock; //XXX
struct ui_config config;
PANEL *main_panel, *mhelp_panel, *details_panel, *details_ex_panel, *raw_panel;
struct sip_call *active_call;

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
    noecho(); // Dont write user input on screen
    curs_set(0); // Hide the cursor
    ESCDELAY = 25; // Only delay ESC Sequences 25 ms
    start_color();

    // FIXME We're a bit restrictive about this....
    if (COLS < UI_MIN_COLS) {
        endwin();
        return 127;
    }

    // Initialize colors
    config = uicfg;
    toggle_color(config.color);

    // Create all panels (Maybe this is not the best but is the easiest for now)
    main_panel = new_panel(newwin(LINES, COLS, 0, 0));
    mhelp_panel = new_panel(newwin(20, 50, LINES / 4, COLS / 4));
    details_panel = new_panel(newwin(LINES, COLS, 0, 0));
    details_ex_panel = new_panel(newwin(LINES, COLS, 0, 0));
    raw_panel = new_panel(newwin(LINES, COLS, 0, 0));

    // Add panel info structure
    struct panel_info main_info = {
            .type = MAIN_PANEL,
            .highlight = 1,
            .draw = &draw_main_panel };
    struct panel_info mhelp_info = {
            .type = MHELP_PANEL,
            .highlight = 0,
            .draw = &draw_main_help_panel };
    struct panel_info details_info = {
            .type = DETAILS_PANEL,
            .highlight = 1,
            .draw = &draw_details_panel };
    struct panel_info details_ex_info = {
            .type = DETAILS_EX_PANEL,
            .highlight = 0,
            .draw = &draw_details_ex_panel };
    struct panel_info raw_info = {
            .type = RAW_PANEL,
            .highlight = 0,
            .draw = &draw_raw_panel };

    // Add panel info to every panel
    set_panel_userptr(main_panel, &main_info);
    set_panel_userptr(mhelp_panel, &mhelp_info);
    set_panel_userptr(details_panel, &details_info);
    set_panel_userptr(details_ex_panel, &details_ex_info);
    set_panel_userptr(raw_panel, &raw_info);

    // Hide all panels (except the main one)
    hide_panel(mhelp_panel);
    hide_panel(details_panel);
    hide_panel(details_ex_panel);
    hide_panel(raw_panel);

    // Wait for user input
    wait_for_input(main_panel);

    // End ncurses mode 
    endwin();
    return 0;
}

/**
 * Wait for user input.
 * This function manages all user input in all panel types and
 * redraws the panel using its own draw function
 * 
 * @param panel the topmost panel
 */
void wait_for_input(PANEL *panel)
{

    // Get window of main panel
    WINDOW *win = panel_window(panel);
    keypad(win, TRUE);

    // Get panel info
    struct panel_info *pinfo;
    pinfo = (struct panel_info*) panel_userptr(panel);
    pinfo->highlight = 1;

    for (;;) {
        // Put this panel on top
        top_panel(panel);

        pthread_mutex_lock(&calls_lock);
        // Paint the panel 
        if ((pinfo->draw)(panel) != 0) {
            pthread_mutex_unlock(&calls_lock);
            return;
        }
        pthread_mutex_unlock(&calls_lock);

        update_panels(); // Update the stacking order
        doupdate(); // Refresh screen

        int c = wgetch(win);
        switch (c) {
        case 'C':
        case 'c':
            config.color = (config.color) ? 0 : 1;
            toggle_color(config.color);
            break;
        case 'H':
        case 'h':
        case 265: /* KEY_F1 */
            if (pinfo->type == MAIN_PANEL) wait_for_input(mhelp_panel);
            break;
        case KEY_PPAGE:
            if (pinfo->type == RAW_PANEL) pinfo->highlight -= 10;
        case KEY_UP:
            pinfo->highlight--;
            if (pinfo->highlight < 1) pinfo->highlight = 1;
            break;
        case KEY_NPAGE:
            if (pinfo->type == RAW_PANEL) pinfo->highlight += 10;
        case KEY_DOWN:
            pinfo->highlight++;
            if (pinfo->highlight > pinfo->entries) pinfo->highlight = pinfo->entries;
            break;
        case 10: /* KEY_ENTER */
            if (pinfo->type == MAIN_PANEL) wait_for_input(details_panel);
            break;
        case 'F':
        case 'f':
            if (pinfo->type == MAIN_PANEL || pinfo->type == DETAILS_PANEL || pinfo->type
                    == DETAILS_EX_PANEL) wait_for_input(raw_panel);
            break;
        case 'X':
        case 'x':
            if (active_call) {
                if (pinfo->type == MAIN_PANEL && get_ex_call(active_call)) {
                    wait_for_input(details_ex_panel);
                } else if (pinfo->type == DETAILS_PANEL && get_ex_call(active_call)) {
                    wait_for_input(details_ex_panel);
                    return;
                } else if (pinfo->type == DETAILS_EX_PANEL) {
                    wait_for_input(details_panel);
                    return;
                }
            }
            break;
        case 'Q':
        case 'q':
        case 27: /* KEY_ESC */
            pinfo->highlight = 0;
            pinfo->padpos = 0;
            hide_panel(panel);
            return;
            break;
        }
    }
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
    // Get the topmost panel
    PANEL *panel = panel_below(NULL);

    if (panel) {
        // Get panel info
        struct panel_info *pinfo;
        pinfo = (struct panel_info*) panel_userptr(panel);
        if (!pinfo) return;

        /** FIXME
         * If panel type is DETAILS or DETAILS_EX, it should
         * be only be refreshed if active_call Call-ID or its
         * ex_call Call-ID are the one being updated 
         */

        // Paint the panel
        if ((pinfo->draw)(panel) != 0) return;

        update_panels(); // Update the stacking order
        doupdate(); // Refresh screen
    }
}

/****************************************************************************
 * Draw functions for each type of panel 
 * A pointer to one of this function is stored into each panel  information 
 * structure @panel_info. Draw function is invoked during the user input loop 
 * or async when the exec thread request UI update.
 ****************************************************************************/
/**
 * Main Calls panel draw function
 *
 * This panel contains a list of the parsed calls, one line per found
 * Call-ID.
 *
 * At the top of the panel a brief bar will show the current running 
 * status: pcap file (Offline) or ngrep parsing (Online).
 *
 */
int draw_main_panel(PANEL *panel)
{

    int y = 1, x = 5;
    int w, h, ph;

    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get panel info
    struct panel_info *pinfo = (struct panel_info*) panel_userptr(panel);
    pinfo->entries = get_n_calls();

    // Get window size
    getmaxyx(win, h, w);

    title_foot_box(win);
    mvwprintw(win, y, (w - 45) / 2, "sngrep - SIP message interface for ngrep");
    if (config.online) {
        mvwprintw(win, y + 2, 2, "Current Mode: %s", "Online");
    } else {
        mvwprintw(win, y + 2, 2, "Current Mode: %s", "Offline");
        mvwprintw(win, y + 3, 2, "Filename: %s", config.fname);
    }
    mvwaddch(win, y+4, 0, ACS_LTEE);
    mvwhline(win, y+4, 1, ACS_HLINE, w - 2);
    mvwaddch(win, y+4, w-1, ACS_RTEE);
    mvwprintw(win, y + 5, x + 2, "From SIP");
    mvwprintw(win, y + 5, x + 51, "To SIP");
    mvwprintw(win, y + 5, x + 109, "Msg");
    mvwprintw(win, y + 5, x + 116, "From");
    mvwprintw(win, y + 5, x + 136, "To");
    mvwprintw(win, y + 5, x + 155, "Starting");
    mvwaddch(win, y+6, 0, ACS_LTEE);
    mvwhline(win, y+6, 1, ACS_HLINE, w - 2);
    mvwaddch(win, y+6, w-1, ACS_RTEE);

    WINDOW *main_pad = newpad(get_n_calls() + h, w);

    struct sip_call *call = calls;
    int cline = 0, callcnt = 1;
    while (call) {
        if (callcnt == pinfo->highlight) {
            active_call = call;
            wattron(main_pad,COLOR_PAIR(HIGHLIGHT_COLOR));
        }

        mvwprintw(main_pad, cline, x + 2, "%*s", w - x * 2 - 4, ""); /* Highlight all the line */
        mvwprintw(main_pad, cline, x, "%.50s", call->messages->sip_from);
        mvwprintw(main_pad, cline, x + 50, "%.50s", call->messages->sip_to);
        mvwprintw(main_pad, cline, x + 109, "%d", get_n_msgs(call));
        mvwprintw(main_pad, cline, x + 115, "%s", call->messages->ip_from);
        mvwprintw(main_pad, cline, x + 135, "%s", call->messages->ip_to);
        mvwprintw(main_pad, cline, x + 155, "%s", call->messages->type);
        wattroff(main_pad, COLOR_PAIR(HIGHLIGHT_COLOR));
        cline++;
        callcnt++;
        call = call->next;
    }

    /* Calculate the space the pad will be covering in the screen */
    ph = h - 2 /* Title */- 5 /* Header */- 2 /* Footer */;

    /* Calculate the highlight position */
    // The highlight position is below the last displayed position?
    if (pinfo->highlight > ph + pinfo->padpos - 2) {
        // Scrolling down 
        pinfo->padpos++;
        // The highlight position is above the first displayed position?
    } else if (pinfo->highlight <= pinfo->padpos) {
        // Scroll up
        pinfo->padpos--;
    }

    /* Draw some fancy arrow to indicate scrolling */
    if (pinfo->padpos > 0) {
        mvwaddch(main_pad, pinfo->padpos, 3, ACS_UARROW);
    }
    if (get_n_calls() > ph + pinfo->padpos) {
        mvwaddch(main_pad, ph+pinfo->padpos-3, 3, ACS_DARROW);
    }

    // Copy the rawmessage into the screen
    copywin(main_pad, win, pinfo->padpos, 1, 2 + 5 + 1, 1, 5 + ph, w - 2, false);
    delwin(main_pad);

    mvwprintw(
            win,
            h - 2,
            2,
            "Q: Quit    C: Toggle color    F: Show raw messages     H: Help    ENTER: Show Call-flow    X: Show Extended Call-Flow");

    return 0;

}

/**
 * Main Help panel draw function
 * 
 * This panel contains information about common keybindings, but it's a 
 * bit deprecated. XXX
 *
 */
int draw_main_help_panel(PANEL *panel)
{
    int cline = 1;
    int width, height;

    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get window size
    getmaxyx(win, height, width);

    box(win, 0, 0);
    mvwprintw(win, cline++, 15, "Help - Dialogs Window ");
    mvwaddch(win, cline, 0, ACS_LTEE);
    mvwhline(win, cline, 1, ACS_HLINE, width - 2);
    mvwaddch(win, cline++, width-1, ACS_RTEE);
    wattron(win,COLOR_PAIR(HELP_COLOR));
    mvwprintw(win, cline, 3, "F1/h:");
    mvwprintw(win, cline + 1, 3, "ESC/q:");
    mvwprintw(win, cline + 2, 3, "Up:");
    mvwprintw(win, cline + 3, 3, "Down:");
    mvwprintw(win, cline + 4, 3, "Enter:");
    wattroff(win,COLOR_PAIR(HELP_COLOR));
    mvwprintw(win, cline, 15, "Show this screen :)");
    mvwprintw(win, cline + 1, 15, "Exit sngrep");
    mvwprintw(win, cline + 2, 15, "Select Previous dialog");
    mvwprintw(win, cline + 3, 15, "Select Next dialog");
    mvwprintw(win, cline + 4, 15, "Show dialog details");

    return 0;
}

/**
 * Call Details panel
 *
 * This panel show the call-flow of active_call. The panel will be divided
 * into two pads: one containing the call flow and one showing the payload
 * of the selected SIP message.
 *
 * XXX By now, scrolling is only available in Call flow pad, but it would be
 * great to scroll SIP message pad using and mod key (like SHIFT)
 *
 */
int draw_details_panel(PANEL *panel)
{
    int h, w, fw, fh, rw, rh, ph;
    int msgcnt = 0;

    // This panel only makes sense with a selected call
    if (!active_call) return 1;

    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get panel info
    struct panel_info *pinfo = (struct panel_info*) panel_userptr(panel);
    pinfo->entries = get_n_msgs(active_call);

    // Get data from first message
    const char *from = active_call->messages->ip_from;

    // Get window size
    getmaxyx(win, h, w);
    // Get flow size
    fw = 65;
    fh = h - 3 - 3;
    // Get the raw size
    rw = w - fw - 2;
    rh = h - 3 - 3;

    // Window borders
    wattron(win,COLOR_PAIR(DETAIL_BORDER_COLOR));
    title_foot_box(win);
    mvwaddch(win, 2, fw, ACS_TTEE);
    mvwvline(win, 3, fw, ACS_VLINE, fh);
    mvwaddch(win, 4, 0, ACS_LTEE);
    mvwhline(win, 4, 1, ACS_HLINE, fw);
    mvwaddch(win, 4, fw, ACS_RTEE);
    mvwaddch(win, 3+fh, fw, ACS_BTEE);
    wattroff(win,COLOR_PAIR(DETAIL_BORDER_COLOR));

    // Window title
    mvwprintw(win, 1, (w - 45) / 2, "Call Details for %s", active_call->callid);
    // Callflow box title
    mvwprintw(win, 3, 30, "Call Flow");

    // Hosts and lines in callflow
    mvwprintw(win, 5, 13, "%-22s", active_call->messages->ip_from);
    mvwprintw(win, 5, 42, "%-22s", active_call->messages->ip_to);
    mvwhline(win, 6, 11, ACS_HLINE, 20);
    mvwhline(win, 6, 40, ACS_HLINE, 20);
    mvwaddch(win, 6, 20, ACS_TTEE);
    mvwaddch(win, 6, 50, ACS_TTEE);

    mvwprintw(win, h - 2, 2,
            "Q: Quit    C: Toggle color    F: Show raw messages     X: Show Extended Call-Flow");

    // Make the pad long enough to contain all messages and some extra space
    WINDOW *flow_pad = newpad(fh + get_n_msgs(active_call) * 2, fw);
    mvwvline(flow_pad, 0, 20, ACS_VLINE, fh+get_n_msgs(active_call)*2);
    mvwvline(flow_pad, 0, 50, ACS_VLINE, fh+get_n_msgs(active_call)*2);

    // Make a pad for sip message
    WINDOW *raw_pad = newpad(rh, rw);

    int cline = 0;
    struct sip_msg *msg = NULL;
    while ((msg = get_next_msg(active_call, msg))) {
        msgcnt++;

        if (msgcnt == pinfo->highlight) {
            int raw_line = 0, pcount = 0;
            for (raw_line = 0; raw_line < msg->plines; raw_line++, pcount++) {
                mvwprintw(raw_pad, pcount, 0, "%.*s", rw, msg->payload[raw_line]);
            }
        }

        // Print timestamp
        mvwprintw(flow_pad, cline, 2, "%s", msg->time);

        if (msgcnt == pinfo->highlight) wattron(flow_pad,A_REVERSE);

        // Determine the message direction
        if (!strcmp(msg->ip_from, from)) {
            wattron(flow_pad,COLOR_PAIR(OUTGOING_COLOR));
            mvwhline(flow_pad, cline+1, 22, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline+1, 47, ACS_RARROW);
        } else {
            wattron(flow_pad,COLOR_PAIR(INCOMING_COLOR));
            mvwhline(flow_pad, cline+1, 22, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline+1, 22, ACS_LARROW);
        }

        // Draw message type or status and line
        mvwprintw(flow_pad, cline, 22, "%26s", "");
        int msglen = strlen(msg->type);
        if (msglen > 24) msglen = 24;
        mvwprintw(flow_pad, cline, 22 + (24 - msglen) / 2, "%.24s", msg->type);

        // Turn off colors
        wattroff(flow_pad,COLOR_PAIR(OUTGOING_COLOR));
        wattroff(flow_pad,COLOR_PAIR(INCOMING_COLOR));
        wattroff(flow_pad, A_REVERSE);

        cline++;
        cline++;
    }

    /* Calculate the space the pad will be covering in the screen */
    ph = fh - 3 /* CF header */- 2 /* Addresses */;
    /* Make it even */
    ph -= ph % 2;

    /* Calculate the highlight position */
    // The highlight position is below the last displayed position?
    if (pinfo->highlight * 2 > ph + pinfo->padpos) {
        // Scrolling down 
        pinfo->padpos += 2;
        // The highlight position is above the first displayed position?
    } else if (pinfo->highlight * 2 <= pinfo->padpos) {
        // Scroll up
        pinfo->padpos -= 2;
    }

    /* Draw some fancy arrow to indicate scrolling */
    if (pinfo->padpos > 0) {
        mvwaddch(flow_pad, pinfo->padpos, 20, ACS_UARROW);
        mvwaddch(flow_pad, pinfo->padpos, 50, ACS_UARROW);
    }
    if (get_n_msgs(active_call) * 2 > ph + pinfo->padpos) {
        mvwaddch(flow_pad, ph+pinfo->padpos-1, 20, ACS_DARROW);
        mvwaddch(flow_pad, ph+pinfo->padpos-1, 50, ACS_DARROW);
    }

    // Copy the callflow into the screen
    copywin(flow_pad, win, pinfo->padpos, 1, 3 + 2 + 2, 1, 6 + ph, fw - 1, false);
    delwin(flow_pad);
    // Copy the rawmessage into the screen
    copywin(raw_pad, win, 0, 0, 3, fw + 1, rh, fw + rw, false);
    delwin(raw_pad);

    return 0;

}

/**
 * Call Details Extended panel
 *
 * This panel show the call-flow of active_call and the second leg of the call
 * if any is found in calls list (@see get_ex_call function)
 *
 * The panel will be divided
 * into two pads: one containing the call flow and one showing the payload
 * of the selected SIP message.
 *
 * XXX By now, scrolling is only available in Call flow pad, but it would be
 * great to scroll SIP message pad using and mod key (like SHIFT)
 *
 */
int draw_details_ex_panel(PANEL *panel)
{
    int w, h, fw, fh, rw, rh, ph;
    int msgcnt = 0;

    // This panel only makes sense with a selected call
    if (!active_call) return 1;

    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get other call data
    struct sip_call *call = active_call;
    struct sip_call *call2 = get_ex_call(call);

    if (!call2) return 1;

    // Get panel info
    struct panel_info *pinfo = (struct panel_info*) panel_userptr(panel);
    pinfo->entries = get_n_msgs(call) + get_n_msgs(call2);

    const char *from, *to, *via;
    const char *callid1, *callid2;

    const struct sip_msg *first = get_next_msg_ex(call, NULL);
    if (!strcmp(call->callid, first->call->callid)) {
        from = call->messages->ip_from;
        via = call->messages->ip_to;
        to = call2->messages->ip_to;
        callid1 = call->callid;
        callid2 = call2->callid;
    } else {
        from = call2->messages->ip_from;
        via = call2->messages->ip_to;
        to = call->messages->ip_to;
        callid1 = call2->callid;
        callid2 = call->callid;
    }

    // Get window size
    getmaxyx(win, h, w);
    // Get flow size
    fw = 95;
    fh = h - 3 - 3;
    // Get the raw size
    rw = w - fw - 2;
    rh = h - 3 - 3;

    // Window borders
    wattron(win,COLOR_PAIR(DETAIL_BORDER_COLOR));
    title_foot_box(win);
    mvwaddch(win, 2, fw, ACS_TTEE);
    mvwvline(win, 3, fw, ACS_VLINE, fh);
    mvwaddch(win, 4, 0, ACS_LTEE);
    mvwhline(win, 4, 1, ACS_HLINE, fw);
    mvwaddch(win, 4, fw, ACS_RTEE);
    mvwaddch(win, 3+fh, fw, ACS_BTEE);
    wattroff(win,COLOR_PAIR(DETAIL_BORDER_COLOR));

    // Window title
    mvwprintw(win, 1, (w - 45) / 2, "Call Details for %s", call->callid);
    // Callflow box title
    mvwprintw(win, 3, 40, "Call Flow Extended");

    // Hosts and lines in callflow
    mvwprintw(win, 5, 13, "%-22s", from);
    mvwprintw(win, 5, 42, "%-22s", via);
    mvwprintw(win, 5, 72, "%-22s", to);
    mvwhline(win, 6, 11, ACS_HLINE, 20);
    mvwhline(win, 6, 40, ACS_HLINE, 20);
    mvwhline(win, 6, 70, ACS_HLINE, 20);
    mvwaddch(win, 6, 20, ACS_TTEE);
    mvwaddch(win, 6, 50, ACS_TTEE);
    mvwaddch(win, 6, 80, ACS_TTEE);
    mvwprintw(win, h - 2, 2,
            "Q: Quit    C: Toggle color    F: Show raw messages     X: Show Call-Flow");

    int msgcntex = get_n_msgs(call) + get_n_msgs(call2);

    // Make the pad long enough to contain all messages and some extra space
    WINDOW *flow_pad = newpad(fh + msgcntex * 2, fw);
    mvwvline(flow_pad, 0, 20, ACS_VLINE, fh+msgcntex*2);
    mvwvline(flow_pad, 0, 50, ACS_VLINE, fh+msgcntex*2);
    mvwvline(flow_pad, 0, 80, ACS_VLINE, fh+msgcntex*2);

    // Make a pad for sip message
    WINDOW *raw_pad = newpad(rh, rw);

    struct sip_msg *msg = NULL;

    int cline = 0;
    while ((msg = get_next_msg_ex(call, msg))) {
        msgcnt++;

        // Set raw content for selected text
        if (msgcnt == pinfo->highlight) {
            int raw_line = 0, pcount = 0;
            for (raw_line = 0; raw_line < msg->plines; raw_line++, pcount++) {
                mvwprintw(raw_pad, pcount, 0, "%.*s", rw, msg->payload[raw_line]);
            }
        }

        // Print timestamp
        mvwprintw(flow_pad, cline, 2, "%s", msg->time);

        if (msgcnt == pinfo->highlight) wattron(flow_pad,A_REVERSE);

        int msglen = strlen(msg->type);
        if (msglen > 24) msglen = 24;

        // Determine the message direction
        if (!strcmp(msg->call->callid, callid1) && !strcmp(msg->ip_from, from)) {
            wattron(flow_pad,COLOR_PAIR(OUTGOING_COLOR));
            mvwprintw(flow_pad, cline, 22, "%26s", "");
            mvwprintw(flow_pad, cline++, 22 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(flow_pad, cline, 22, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline++, 47, ACS_RARROW);
        } else if (!strcmp(msg->call->callid, callid1) && !strcmp(msg->ip_to, from)) {
            wattron(flow_pad,COLOR_PAIR(INCOMING_COLOR));
            mvwprintw(flow_pad, cline, 22, "%26s", "");
            mvwprintw(flow_pad, cline++, 22 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(flow_pad, cline, 22, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline++, 22, ACS_LARROW);
        } else if (!strcmp(msg->call->callid, callid2) && !strcmp(msg->ip_from, via)) {
            wattron(flow_pad,COLOR_PAIR(OUTGOING_COLOR));
            mvwprintw(flow_pad, cline, 52, "%26s", "");
            mvwprintw(flow_pad, cline++, 54 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(flow_pad, cline, 52, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline++, 77, ACS_RARROW);
        } else {
            wattron(flow_pad,COLOR_PAIR(INCOMING_COLOR));
            mvwprintw(flow_pad, cline, 52, "%26s", "");
            mvwprintw(flow_pad, cline++, 54 + (24 - msglen) / 2, "%.24s", msg->type);
            mvwhline(flow_pad, cline, 52, ACS_HLINE, 26);
            mvwaddch(flow_pad, cline++, 52, ACS_LARROW);
        }

        wattroff(flow_pad,COLOR_PAIR(OUTGOING_COLOR));
        wattroff(flow_pad,COLOR_PAIR(INCOMING_COLOR));
        wattroff(flow_pad, A_REVERSE);
    }

    /* Calculate the space the pad will be covering in the screen */
    ph = fh - 3 /* CF header */- 2 /* Addresses */;
    /* Make it even */
    ph -= ph % 2;

    /* Calculate the highlight position */
    // The highlight position is below the last displayed position?
    if (pinfo->highlight * 2 > ph + pinfo->padpos) {
        // Scrolling down 
        pinfo->padpos += 2;
        // The highlight position is above the first displayed position?
    } else if (pinfo->highlight * 2 <= pinfo->padpos) {
        // Scroll up
        pinfo->padpos -= 2;
    }

    /* Draw some fancy arrow to indicate scrolling */
    if (pinfo->padpos > 0) {
        mvwaddch(flow_pad, pinfo->padpos, 20, ACS_UARROW);
        mvwaddch(flow_pad, pinfo->padpos, 50, ACS_UARROW);
        mvwaddch(flow_pad, pinfo->padpos, 80, ACS_UARROW);
    }
    if (msgcntex * 2 > ph + pinfo->padpos) {
        mvwaddch(flow_pad, ph+pinfo->padpos-1, 20, ACS_DARROW);
        mvwaddch(flow_pad, ph+pinfo->padpos-1, 50, ACS_DARROW);
        mvwaddch(flow_pad, ph+pinfo->padpos-1, 80, ACS_DARROW);
    }

    // Copy the callflow into the screen
    copywin(flow_pad, win, pinfo->padpos, 1, 3 + 2 + 2, 1, 6 + ph, fw - 1, false);
    delwin(flow_pad);
    // Copy the rawmessage into the screen
    copywin(raw_pad, win, 0, 0, 3, fw + 1, rh, fw + rw, false);
    delwin(raw_pad);

    return 0;

}

/**
 * Show the SIPs messages in a raw full screen panel
 *
 * This panel was designed with the intention of making the SIP Messages easier
 * to copy :)
 */
int draw_raw_panel(PANEL *panel)
{

    // This panel only makes sense with a selected call
    if (!active_call) return 1;

    // Get window of main panel
    WINDOW *win = panel_window(panel);

    // Get panel info
    struct panel_info *pinfo = (struct panel_info*) panel_userptr(panel);

    // Make a pad for sip message
    WINDOW *raw_pad = newpad(get_n_msgs(active_call) * 100, COLS);

    int pline = 0, raw_line;
    struct sip_msg *msg = NULL;
    while ((msg = get_next_msg(active_call, msg))) {
        for (raw_line = 0; raw_line < msg->plines; raw_line++) {
            mvwprintw(raw_pad, pline, 0, "%s", msg->payload[raw_line]);
            pline++;
        }
        pline++;
        pline++;
    }
    copywin(raw_pad, win, pinfo->highlight - 1, 0, 0, 0, LINES - 1, COLS - 1, false);
    delwin(raw_pad);

    // Update the last line
    pinfo->entries = pline - LINES;
    if (pinfo->entries < 1) pinfo->entries = 1; // Disable scrolling if not enough lines

    return 0;

}

