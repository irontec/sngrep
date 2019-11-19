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
 * @file stats_win.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in stats.h
 */
/*
 * +---------------------------------------------------------+
 * |                    StorageStats Information                    |
 * +---------------------------------------------------------+
 * |  Dialogs: 725                  COMPLETED:  7 (22.1%)    |
 * |  Calls: 10                     CANCELLED:  2 (12.2%)    |
 * |  Messages: 200                 IN CALL:    10 (60.5%)   |
 * |                                REJECTED:   0 (0.0%)     |
 * |                                BUSY:       0 (0.0%)     |
 * |                                DIVERTED:   0 (0.0%)     |
 * |                                CALL SETUP: 0 (0.0%)     |
 * +---------------------------------------------------------+
 * |  INVITE:    10 (0.5%)          1XX: 123 (1.5%)          |
 * |  REGISTER:  200 (5.1%)         2XX: 231 (3.1%)          |
 * |  SUBSCRIBE: 20 (1.0%)          3XX: 0 (0.0%)            |
 * |  UPDATE:    30 (1.3%)          4XX: 12 (1.5%)           |
 * |  NOTIFY:    650 (22.7%)        5XX: 0 (0.0%)            |
 * |  OPTIONS:   750 (27.4%)        6XX: 3 (0.5%)            |
 * |  PUBLISH:   0 (0.0%)           7XX: 0 (0.0%)            |
 * |  MESSAGE:   0 (0.0%)           8XX: 0 (0.0%)            |
 * |  INFO:      0 (0.0%)                                    |
 * |  BYE:       10 (0.5%)                                   |
 * |  CANCEL:    0 (0.0%)                                    |
 * +---------------------------------------------------------+
 * |               Press any key to continue                 |
 * +---------------------------------------------------------+
 *
 */
#include "config.h"
#include <glib.h>
#include <storage/message.h>
#include "glib/glib-extra.h"
#include "storage/storage.h"
#include "storage/packet/packet_sip.h"
#include "stats_win.h"

Window *
stats_win_new()
{
    Window *window = g_malloc0(sizeof(Window));
    window->type = WINDOW_STATS;
    window->destroy = window_deinit;

    // Counters!
    struct
    {
        int dtotal, dcalls, completed, cancelled, incall, rejected, setup, busy, diverted;
        int mtotal, invite, regist, subscribe, update, notify, options;
        int publish, message, info, ack, bye, cancel;
        int r100, r200, r300, r400, r500, r600, r700, r800;
    } stats;
    memset(&stats, 0, sizeof(stats));

    // Calculate window dimensions
    window_init(window, 25, 60);

    // Set the window title and boxes
    mvwprintw(window->win, 1, window->width / 2 - 9, "StorageStats Information");
    wattron(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));
    title_foot_box(window->panel);
    mvwhline(window->win, 10, 1, ACS_HLINE, window->width - 1);
    mvwaddch(window->win, 10, 0, ACS_LTEE);
    mvwaddch(window->win, 10, window->width - 1, ACS_RTEE);
    mvwprintw(window->win, window->height - 2, window->width / 2 - 9, "Press ESC to leave");
    wattroff(window->win, COLOR_PAIR(CP_BLUE_ON_DEF));

    // Parse the data
    GPtrArray *calls = storage_calls();
    stats.dtotal = g_ptr_array_len(calls);

    // Ignore this screen when no dialog exists
    if (!stats.dtotal) {
        mvwprintw(window->win, 3, 3, "No information to display");
        return window;
    }

    for (guint i = 0; i < g_ptr_array_len(calls); i++) {
        Call *call = g_ptr_array_index(calls, i);

        // If this dialog is a call
        if (call->state) {
            // Increase call counter
            stats.dcalls++;
            // Increase call status counter
            switch (call->state) {
                case CALL_STATE_CALLSETUP:
                    stats.setup++;
                    break;
                case CALL_STATE_INCALL:
                    stats.incall++;
                    break;
                case CALL_STATE_CANCELLED:
                    stats.cancelled++;
                    break;
                case CALL_STATE_REJECTED:
                    stats.rejected++;
                    break;
                case CALL_STATE_BUSY:
                    stats.busy++;
                    break;
                case CALL_STATE_DIVERTED:
                    stats.diverted++;
                    break;
                case CALL_STATE_COMPLETED:
                    stats.completed++;
                    break;
            }
        }

        // For each message in call
        for (guint i = 0; i < g_ptr_array_len(call->msgs); i++) {
            Message *msg = g_ptr_array_index(call->msgs, i);
            guint method = msg->request.id;

            // Increase message counter
            stats.mtotal++;
            // Check message type
            switch (method) {
                case SIP_METHOD_REGISTER:
                    stats.regist++;
                    break;
                case SIP_METHOD_INVITE:
                    stats.invite++;
                    break;
                case SIP_METHOD_SUBSCRIBE:
                    stats.subscribe++;
                    break;
                case SIP_METHOD_NOTIFY:
                    stats.notify++;
                    break;
                case SIP_METHOD_OPTIONS:
                    stats.options++;
                    break;
                case SIP_METHOD_PUBLISH:
                    stats.publish++;
                    break;
                case SIP_METHOD_MESSAGE:
                    stats.message++;
                    break;
                case SIP_METHOD_CANCEL:
                    stats.cancel++;
                    break;
                case SIP_METHOD_BYE:
                    stats.bye++;
                    break;
                case SIP_METHOD_ACK:
                    stats.ack++;
                    break;
                    // case SIP_METHOD_PRACK:
                case SIP_METHOD_INFO:
                    stats.info++;
                    break;
                    // case SIP_METHOD_REFER:
                case SIP_METHOD_UPDATE:
                    stats.update++;
                    break;
                default:
                    if (method >= 800) stats.r800++;
                    else if (method >= 700) stats.r700++;
                    else if (method >= 600) stats.r600++;
                    else if (method >= 500) stats.r500++;
                    else if (method >= 400) stats.r400++;
                    else if (method >= 300) stats.r300++;
                    else if (method >= 200) stats.r200++;
                    else if (method >= 100) stats.r100++;
            }
        }


    }

    // Print parses data
    mvwprintw(window->win, 3, 3, "Dialogs: %d", stats.dtotal);
    mvwprintw(window->win, 4, 3, "Calls: %d (%.1f%%)", stats.dcalls, (float) stats.dcalls * 100 / stats.dtotal);
    mvwprintw(window->win, 5, 3, "Messages: %d", stats.mtotal);
    // Print status of calls if any
    if (stats.dcalls) {
        mvwprintw(window->win, 3, 33, "COMPLETED:  %d (%.1f%%)", stats.completed,
                  (float) stats.completed * 100 / stats.dcalls);
        mvwprintw(window->win, 4, 33, "CANCELLED:  %d (%.1f%%)", stats.cancelled,
                  (float) stats.cancelled * 100 / stats.dcalls);
        mvwprintw(window->win, 5, 33, "IN CALL:    %d (%.1f%%)", stats.incall,
                  (float) stats.incall * 100 / stats.dcalls);
        mvwprintw(window->win, 6, 33, "REJECTED:   %d (%.1f%%)", stats.rejected,
                  (float) stats.rejected * 100 / stats.dcalls);
        mvwprintw(window->win, 7, 33, "BUSY:       %d (%.1f%%)", stats.busy, (float) stats.busy * 100 / stats.dcalls);
        mvwprintw(window->win, 8, 33, "DIVERTED:   %d (%.1f%%)", stats.diverted,
                  (float) stats.diverted * 100 / stats.dcalls);
        mvwprintw(window->win, 9, 33, "CALL SETUP: %d (%.1f%%)", stats.setup, (float) stats.setup * 100 / stats.dcalls);
    }

    mvwprintw(window->win, 11, 3, "INVITE:    %d (%.1f%%)", stats.invite, (float) stats.invite * 100 / stats.mtotal);
    mvwprintw(window->win, 12, 3, "REGISTER:  %d (%.1f%%)", stats.regist, (float) stats.regist * 100 / stats.mtotal);
    mvwprintw(window->win, 13, 3, "SUBSCRIBE: %d (%.1f%%)", stats.subscribe,
              (float) stats.subscribe * 100 / stats.mtotal);
    mvwprintw(window->win, 14, 3, "UPDATE:    %d (%.1f%%)", stats.update, (float) stats.update * 100 / stats.mtotal);
    mvwprintw(window->win, 15, 3, "NOTIFY:    %d (%.1f%%)", stats.notify, (float) stats.notify * 100 / stats.mtotal);
    mvwprintw(window->win, 16, 3, "OPTIONS:   %d (%.1f%%)", stats.options, (float) stats.options * 100 / stats.mtotal);
    mvwprintw(window->win, 17, 3, "PUBLISH:   %d (%.1f%%)", stats.publish, (float) stats.publish * 100 / stats.mtotal);
    mvwprintw(window->win, 18, 3, "MESSAGE:   %d (%.1f%%)", stats.message, (float) stats.message * 100 / stats.mtotal);
    mvwprintw(window->win, 19, 3, "INFO:      %d (%.1f%%)", stats.info, (float) stats.info * 100 / stats.mtotal);
    mvwprintw(window->win, 20, 3, "BYE:       %d (%.1f%%)", stats.bye, (float) stats.bye * 100 / stats.mtotal);
    mvwprintw(window->win, 21, 3, "CANCEL:    %d (%.1f%%)", stats.cancel, (float) stats.cancel * 100 / stats.mtotal);

    mvwprintw(window->win, 11, 33, "1XX: %d (%.1f%%)", stats.r100, (float) stats.r100 * 100 / stats.mtotal);
    mvwprintw(window->win, 12, 33, "2XX: %d (%.1f%%)", stats.r200, (float) stats.r200 * 100 / stats.mtotal);
    mvwprintw(window->win, 13, 33, "3XX: %d (%.1f%%)", stats.r300, (float) stats.r300 * 100 / stats.mtotal);
    mvwprintw(window->win, 14, 33, "4XX: %d (%.1f%%)", stats.r400, (float) stats.r400 * 100 / stats.mtotal);
    mvwprintw(window->win, 15, 33, "5XX: %d (%.1f%%)", stats.r500, (float) stats.r500 * 100 / stats.mtotal);
    mvwprintw(window->win, 16, 33, "6XX: %d (%.1f%%)", stats.r600, (float) stats.r600 * 100 / stats.mtotal);
    mvwprintw(window->win, 17, 33, "7XX: %d (%.1f%%)", stats.r700, (float) stats.r700 * 100 / stats.mtotal);
    mvwprintw(window->win, 18, 33, "8XX: %d (%.1f%%)", stats.r800, (float) stats.r800 * 100 / stats.mtotal);

    return window;
}
