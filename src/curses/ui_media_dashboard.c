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
 * @file ui_media_dashboard.c
 * @author Mohammad Bagher Amini <darwvin@hotmail.com>
 *
 * @brief Source of functions defined in ui_media_dashboard.h
 *
 * Redraws once per second while the UI dispatcher holds the capture lock.
 */
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ui_media_dashboard.h"
#include "capture.h"

/**
 * Ui Structure definition for Media Inspector panel
 */
ui_t ui_media_dashboard = {
    .type = PANEL_MEDIA_DASHBOARD,
    .panel = NULL,
    .create = media_dashboard_create,
    .destroy = media_dashboard_destroy,
    .redraw = media_dashboard_redraw,
    .draw = media_dashboard_draw,
    .resize = media_dashboard_resize,
    .handle_key = media_dashboard_handle_key,
    .help = NULL
};

static media_inspector_flow_t view[MEDIA_INSPECTOR_MAX_FLOWS];
static int order_by_loss;

int
media_dashboard_cmp_flows(const void *left, const void *right)
{
    const media_inspector_flow_t *a = left, *b = right;
    if (order_by_loss) {
        uint64_t la = media_inspector_estimated_loss(a);
        uint64_t lb = media_inspector_estimated_loss(b);
        if (la != lb)
            return la < lb ? 1 : -1;
    }
    if (a->first_us != b->first_us)
        return a->first_us < b->first_us ? -1 : 1;
    if (a->ssrc != b->ssrc)
        return a->ssrc < b->ssrc ? -1 : 1;
    return 0;
}

void
media_dashboard_label(WINDOW *win, int y, int x, const char *text, int attr)
{
    wattron(win, attr);
    mvwaddstr(win, y, x, text);
    wattroff(win, attr);
}

const char *
media_dashboard_state(const media_inspector_flow_t *f, time_t now)
{
    uint64_t lost = media_inspector_estimated_loss(f);
    uint64_t expected = (uint64_t)f->max_seq - f->first_seq + 1;
    if (f->last_us && (uint64_t)now > f->last_us / 1000000u + 3)
        return "STALE";
    if (expected >= 50 && lost * 100 > expected * 3)
        return "LOSS";
    if (f->jitter_available && f->jitter_ms > 30.0)
        return "JITTER";
    return "OBSERVED";
}

void
media_dashboard_create(ui_t *ui)
{
    media_dashboard_info_t *info = calloc(1, sizeof(*info));
    ui_panel_create(ui, LINES, COLS);
    set_panel_userptr(ui->panel, info);
}

void
media_dashboard_destroy(ui_t *ui)
{
    free((void *)panel_userptr(ui->panel));
    ui_panel_destroy(ui);
}

int
media_dashboard_resize(ui_t *ui)
{
    ui->height = LINES;
    ui->width = COLS;
    wresize(ui->win, LINES, COLS);
    return 0;
}

bool
media_dashboard_redraw(ui_t *ui)
{
    media_dashboard_info_t *info = (media_dashboard_info_t *)panel_userptr(ui->panel);
    time_t now = time(NULL);
    if (info && now != info->last_render) {
        info->last_render = now;
        return true;
    }
    return false;
}

int
media_dashboard_draw(ui_t *ui)
{
    media_dashboard_info_t *info = (media_dashboard_info_t *)panel_userptr(ui->panel);
    size_t n = media_inspector_snapshot(view, MEDIA_INSPECTOR_MAX_FLOWS);
    size_t i, visible;
    uint64_t packets = 0, losses = 0, duplicates = 0, ordered = 0;
    size_t stale = 0;
    time_t now = time(NULL);
    WINDOW *w = ui->win;
    const char *keybindings[] = {
        "F6", "Switch", "Up/Down", "Select", "S", "Sort", "P", "Pause", "Esc", "Back"
    };
    int rows, y, width = ui->width, height = ui->height;
    int compact = width < 110;

    werase(w);
    if (width < 76 || height < 20) {
        mvwprintw(w, 1, 2, "Media Inspector requires at least 76 columns x 20 rows.");
        mvwprintw(w, 3, 2, "F6 switch view  |  ESC return");
        return 0;
    }

    order_by_loss = info ? info->order_by_loss : 0;
    qsort(view, n, sizeof(view[0]), media_dashboard_cmp_flows);

    for (i = 0; i < n; i++) {
        const char *state = media_dashboard_state(&view[i], now);
        packets += view[i].packets;
        losses += media_inspector_estimated_loss(&view[i]);
        duplicates += view[i].duplicates;
        ordered += view[i].out_of_order;
        if (!strcmp(state, "STALE"))
            stale++;
        /* Quality alerts are calculated per observed stream, not per call. */
    }

    ui_set_title(ui, "sngrep - Media Inspector");
    mvwprintw(w, 2, 3, "Live RTP  /  passive observation  /  no SIP required");
    mvwhline(w, 3, 2, ACS_HLINE, width - 4);

    wattron(w, A_BOLD | COLOR_PAIR(CP_WHITE_ON_DEF));
    mvwprintw(w, 5, 3, "FLOWS %-7zu", n);
    mvwprintw(w, 5, 24, "RTP PACKETS %-12llu", (unsigned long long)packets);
    mvwprintw(w, 5, 55, "STALE %-5zu", stale);
    wattroff(w, A_BOLD | COLOR_PAIR(CP_WHITE_ON_DEF));
    mvwprintw(w, 6, 3, "Estimated capture gaps: %-12llu  Reordered: %-10llu  Duplicates: %llu",
              (unsigned long long)losses, (unsigned long long)ordered,
              (unsigned long long)duplicates);
    mvwhline(w, 7, 2, ACS_HLINE, width - 4);

    media_dashboard_label(w, 9, 3, "LIVE RTP STREAMS", A_BOLD | COLOR_PAIR(CP_CYAN_ON_DEF));
    if (!has_colors())
        wattron(w, A_REVERSE);
    wattron(w, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
    ui_clear_line(ui, 11);
    if (compact)
        mvwprintw(w, 11, 3, "%-3s %-19s %-19s %-8s %8s %6s",
                  "#", "SOURCE", "DESTINATION", "STATE", "PACKETS", "LOSS");
    else
        mvwprintw(w, 11, 3, "%-3s %-23s %-23s %-8s %8s %6s %9s",
                  "#", "SOURCE", "DESTINATION", "STATE", "PACKETS", "LOSS", "JITTER");
    wattroff(w, A_BOLD | A_REVERSE | COLOR_PAIR(CP_DEF_ON_CYAN));

    rows = height - 19;
    if (rows < 1) rows = 1;
    if (info && info->selected >= n)
        info->selected = n ? n - 1 : 0;
    if (info && info->selected < info->offset)
        info->offset = info->selected;
    if (info && info->selected >= info->offset + (size_t)rows)
        info->offset = info->selected - (size_t)rows + 1;
    visible = n;
    if (info && visible > info->offset + (size_t)rows)
        visible = info->offset + (size_t)rows;

    for (i = info ? info->offset : 0; i < visible; i++) {
        const media_inspector_flow_t *f = &view[i];
        uint64_t lost = media_inspector_estimated_loss(f);
        char source[96], dest[96], jitter[16];
        const char *state = media_dashboard_state(f, now);
        int attr = i == (info ? info->selected : 0) ? A_REVERSE | A_BOLD : A_NORMAL;
        int color = !strcmp(state, "STALE") || !strcmp(state, "LOSS") ?
                    CP_YELLOW_ON_DEF : CP_GREEN_ON_DEF;
        y = 12 + (int)(i - (info ? info->offset : 0));
        snprintf(source, sizeof(source), "%s:%u", f->src.ip, f->src.port);
        snprintf(dest, sizeof(dest), "%s:%u", f->dst.ip, f->dst.port);
        if (f->jitter_available)
            snprintf(jitter, sizeof(jitter), "%.1fms", f->jitter_ms);
        else
            snprintf(jitter, sizeof(jitter), "N/A");
        wattron(w, attr);
        if (compact)
            mvwprintw(w, y, 3, "%-3zu %-19.19s %-19.19s", i + 1, source, dest);
        else
            mvwprintw(w, y, 3, "%-3zu %-23.23s %-23.23s", i + 1, source, dest);
        wattroff(w, attr);
        media_dashboard_label(w, y, compact ? 48 : 55, state, COLOR_PAIR(color));
        if (compact)
            mvwprintw(w, y, 57, "%8llu %6llu",
                      (unsigned long long)f->packets, (unsigned long long)lost);
        else
            mvwprintw(w, y, 64, "%8llu %6llu %9s",
                      (unsigned long long)f->packets,
                      (unsigned long long)lost, jitter);
    }

    if (!n)
        mvwprintw(w, 13, 4, "Waiting for RTP. Select the media interface or open an RTP-only PCAP.");

    y = height - 6;
    mvwhline(w, y, 2, ACS_HLINE, width - 4);
    media_dashboard_label(w, y + 1, 3, "SELECTED STREAM", A_BOLD | COLOR_PAIR(CP_CYAN_ON_DEF));
    if (info && n) {
        const media_inspector_flow_t *f = &view[info->selected];
        mvwprintw(w, y + 2, 3, "SSRC 0x%08x  PT %u  Bytes %llu  Estimated gaps %llu",
                  f->ssrc, f->payload_type, (unsigned long long)f->bytes,
                  (unsigned long long)media_inspector_estimated_loss(f));
    } else {
        mvwprintw(w, y + 2, 3, "No observed RTP stream selected.");
    }
    if (compact)
        mvwprintw(w, y + 3, 3, "RTPengine leg pairing unavailable; no one-way diagnosis.");
    else
        mvwprintw(w, y + 3, 3,
                  "Direction pairing: unavailable without RTPengine metadata. No one-way diagnosis.");
    ui_draw_bindings(ui, keybindings, 10);
    return 0;
}

int
media_dashboard_handle_key(ui_t *ui, int key)
{
    media_dashboard_info_t *info = (media_dashboard_info_t *)panel_userptr(ui->panel);
    int action = -1;
    if (!info)
        return KEY_NOT_HANDLED;
    if (key == 's' || key == 'S') {
        info->order_by_loss = !info->order_by_loss;
        info->selected = info->offset = 0;
        return KEY_HANDLED;
    }
    while ((action = key_find_action(key, action)) != ERR) {
        if (action == ACTION_UP) {
            if (info->selected) info->selected--;
            return KEY_HANDLED;
        }
        if (action == ACTION_DOWN) {
            if (info->selected + 1 < media_inspector_flow_count())
                info->selected++;
            return KEY_HANDLED;
        }
        if (action == ACTION_NPAGE) {
            info->selected += 10;
            return KEY_HANDLED;
        }
        if (action == ACTION_PPAGE) {
            info->selected = info->selected > 10 ? info->selected - 10 : 0;
            return KEY_HANDLED;
        }
    }
    return KEY_NOT_HANDLED;
}
