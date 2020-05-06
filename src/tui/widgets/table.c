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
 * @file table.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include "glib-extra/glib.h"
#include "setting.h"
#include "storage/filter.h"
#include "tui/theme.h"
#include "tui/keybinding.h"
#include "tui/widgets/table.h"

enum
{
    SIG_ACTIVATE,
    SIGS
};

static guint signals[SIGS] = { 0 };

// Menu table class definition
G_DEFINE_TYPE(SngTable, sng_table, SNG_TYPE_WIDGET)

static void
sng_table_activate(SngTable *table)
{
    g_signal_emit(SNG_WIDGET(table), signals[SIG_ACTIVATE], 0);
}

/**
 * @brief Move selection cursor N times vertically
 *
 * @param self CallListWindow pointer
 * @param times number of lines, positive for down, negative for up
 */
static void
sng_table_move_vertical(SngTable *table, gint times)
{
    // Set the new current selected index
    table->cur_idx = CLAMP(
        table->cur_idx + times,
        0,
        (gint) g_ptr_array_len(table->dcalls) - 1
    );

    // Move the first index if required (moving up)
    table->first_idx = MIN(table->first_idx, table->cur_idx);

    // Calculate Call List height
    gint height = sng_widget_get_height(SNG_WIDGET(table));
    height -= 1;                                        // Remove header line
    height -= scrollbar_visible(table->hscroll) ? 1 : 0; // Remove Horizontal scrollbar

    // Move the first index if required (moving down)
    table->first_idx = MAX(
        table->first_idx,
        table->cur_idx - height + 1
    );

    // Update vertical scrollbar position
    table->vscroll.pos = table->first_idx;
}

/**
 * @brief Move selection cursor N times horizontal
 *
 * @param self CallListWindow pointer
 * @param times number of lines, positive for right, negative for left
 */
static void
sng_table_move_horizontal(SngTable *table, gint times)
{
    // Move horizontal scroll N times
    table->hscroll.pos = CLAMP(
        table->hscroll.pos + times,
        0,
        table->hscroll.max - getmaxx(table->hscroll.win)
    );
}

static gboolean
sng_table_handle_action(SngWidget *widget, SngAction action)
{

    SngTable *table = SNG_TABLE(widget);
    guint rnpag_steps = (guint) setting_get_intvalue(SETTING_TUI_CL_SCROLLSTEP);
    StorageSortOpts sort;
    Call *call = NULL;

    // Check if we handle this action
    switch (action) {
        case ACTION_RIGHT:
            sng_table_move_horizontal(table, 3);
            break;
        case ACTION_LEFT:
            sng_table_move_horizontal(table, -3);
            break;
        case ACTION_DOWN:
            sng_table_move_vertical(table, 1);
            break;
        case ACTION_UP:
            sng_table_move_vertical(table, -1);
            break;
        case ACTION_HNPAGE:
            sng_table_move_vertical(table, rnpag_steps / 2);
            break;
        case ACTION_NPAGE:
            sng_table_move_vertical(table, rnpag_steps);
            break;
        case ACTION_HPPAGE:
            sng_table_move_vertical(table, -1 * rnpag_steps / 2);
            break;
        case ACTION_PPAGE:
            sng_table_move_vertical(table, -1 * rnpag_steps);
            break;
        case ACTION_BEGIN:
            sng_table_move_vertical(table, g_ptr_array_len(table->dcalls) * -1);
            break;
        case ACTION_END:
            sng_table_move_vertical(table, g_ptr_array_len(table->dcalls));
            break;
        case ACTION_CLEAR:
            // Clear group calls
            call_group_remove_all(table->group);
            break;
        case ACTION_CLEAR_CALLS:
            // Remove all stored calls
            storage_calls_clear();
            // Clear List
            table->vscroll.pos = table->cur_idx = 0;
            call_group_remove_all(table->group);
            break;
        case ACTION_CLEAR_CALLS_SOFT:
            // Remove stored calls, keeping the currently displayed calls
            storage_calls_clear_soft();
            // Clear List
            table->vscroll.pos = table->cur_idx = 0;
            call_group_remove_all(table->group);
            break;
        case ACTION_AUTOSCROLL:
            table->autoscroll = (table->autoscroll) ? 0 : 1;
            break;
        case ACTION_SELECT:
            // Ignore on empty list
            if (g_ptr_array_len(table->dcalls) == 0) {
                break;
            }

            call = g_ptr_array_index(table->dcalls, table->cur_idx);
            if (call_group_exists(table->group, call)) {
                call_group_remove(table->group, call);
            } else {
                call_group_add(table->group, call);
            }
            break;
        case ACTION_SORT_SWAP:
            // Change sort order
            sort = storage_sort_options();
            sort.asc = (sort.asc) ? FALSE : TRUE;
            storage_set_sort_options(sort);
            break;
        case ACTION_CONFIRM:
            sng_table_activate(table);
            break;
        default:
            // Parse next action
            return FALSE;
    }

    return TRUE;
}

static gint
sng_table_columns_width(SngTable *table, guint columns)
{
    // More requested columns that existing columns??
    if (columns > g_ptr_array_len(table->columns)) {
        columns = g_ptr_array_len(table->columns);
    }

    // If requested column is 0, count all columns
    guint columncnt = (columns == 0) ? g_ptr_array_len(table->columns) : columns;

    // Add extra width for spaces between columns + selection box
    gint width = 5 + columncnt;

    // Sum all column widths
    for (guint i = 0; i < columncnt; i++) {
        Attribute *attr = g_ptr_array_index(table->columns, i);
        width += attr->length;
    }

    return width;
}

void
sng_table_columns_update(SngTable *table)
{
    // Add configured columns
    table->columns = g_ptr_array_new_with_free_func(g_free);
    g_auto(GStrv) columns = g_strsplit(setting_get_value(SETTING_TUI_CL_COLUMNS), ",", 0);
    for (guint i = 0; i < g_strv_length(columns); i++) {
        Attribute *attr = attribute_find_by_name(columns[i]);
        g_return_if_fail(attr != NULL);
        // Add column to the list
        g_ptr_array_add(table->columns, attr);
    }
}

CallGroup *
sng_table_get_call_group(SngTable *table)
{
    return table->group;
}

void
sng_table_select_current(SngTable *table)
{
    sng_table_handle_action(SNG_WIDGET(table), ACTION_SELECT);
}

Call *
sng_table_get_current(SngTable *table)
{
    if (g_ptr_array_len(table->dcalls) == 0) {
        return NULL;
    }
    return g_ptr_array_index(table->dcalls, table->cur_idx);
}

void
sng_table_clear(SngTable *table)
{
    sng_table_handle_action(SNG_WIDGET(table), ACTION_CLEAR_CALLS);
}

const gchar *
sng_table_get_line_for_call(SngTable *table, Call *call)
{
    // Get first call message
    Message *msg = g_ptr_array_first(call->msgs);
    g_return_val_if_fail(msg != NULL, NULL);

    // Print requested columns
    GString *out = g_string_new(NULL);
    for (guint i = 0; i < g_ptr_array_len(table->columns); i++) {
        Attribute *attr = g_ptr_array_index(table->columns, i);

        // Get call attribute for current column
        const gchar *call_attr = NULL;
        if ((call_attr = msg_get_attribute(msg, attr)) != NULL) {
            g_string_append(out, call_attr);
        }
    }

    return out->str;
}

static void
sng_table_realize(SngWidget *widget)
{
    SngTable *table = SNG_TABLE(widget);

    if (!sng_widget_is_realized(widget)) {
        // Create a new pad for configured columns
        SNG_WIDGET_CLASS(sng_table_parent_class)->realize(widget);
        WINDOW *win = sng_widget_get_ncurses_window(widget);
        // Set window srollbars
        table->vscroll = window_set_scrollbar(win, SB_VERTICAL, SB_LEFT);
        table->hscroll = window_set_scrollbar(win, SB_HORIZONTAL, SB_BOTTOM);
    }

    SNG_WIDGET_CLASS(sng_table_parent_class)->realize(widget);
}

static void
sng_table_draw(SngWidget *widget)
{
    SngTable *table = SNG_TABLE(widget);

    // Get the list of calls that are going to be displayed
//    g_ptr_array_free(table->dcalls, TRUE);
    table->dcalls = g_ptr_array_copy_filtered(storage_calls(), (GEqualFunc) filter_check_call, NULL);

    // If autoscroll is enabled, select the last dialog
    if (table->autoscroll) {
        StorageSortOpts sort = storage_sort_options();
        if (sort.asc) {
            sng_table_move_vertical(table, g_ptr_array_len(table->dcalls));
        } else {
            sng_table_move_vertical(table, g_ptr_array_len(table->dcalls) * -1);
        }
    }

    WINDOW *win = sng_widget_get_ncurses_window(widget);
    werase(win);

    // Get configured sorting options
    StorageSortOpts sort = storage_sort_options();

    // Draw columns titles
    wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));

    // Draw columns
    wprintw(win, "     ");
    for (guint i = 0; i < g_ptr_array_len(table->columns); i++) {
        Attribute *attr = g_ptr_array_index(table->columns, i);

        // Print sort column indicator
        if (attr == sort.by) {
            wattron(win, A_BOLD | COLOR_PAIR(CP_YELLOW_ON_CYAN));
            wprintw(win, "%c", ((sort.asc) ? '^' : 'v'));
            wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
        }

        gint col_width = CLAMP(
            attribute_get_length(attr),
            0,
            getmaxx(win) - getcurx(win) - 2
        );

        if (col_width > 0) {
            wprintw(
                win,
                "%-*.*s ",
                col_width,
                col_width,
                attribute_get_title(attr)
            );
        }
    }
    wprintw(win, "%*s", getmaxx(win) - getcurx(win), " ");
    wattroff(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));

    // Fill the call list
    for (guint i = (guint) table->vscroll.pos; i < g_ptr_array_len(table->dcalls); i++) {
        Call *call = g_ptr_array_index(table->dcalls, i);
        g_return_if_fail(call != NULL);

        // Get first call message attributes
        Message *msg = g_ptr_array_first(call->msgs);
        g_return_if_fail(msg != NULL);

        // Stop if we have reached the bottom of the list
        if (getcury(win) == getmaxy(win))
            break;

        // Show bold selected rows
        if (call_group_exists(table->group, call))
            wattron(win, A_BOLD | COLOR_PAIR(CP_DEFAULT));

        // Highlight active call
        if (table->cur_idx == (gint) i) {
            wattron(win, COLOR_PAIR(CP_WHITE_ON_BLUE));
            if (!sng_widget_has_focus(widget)) {
                wattron(win, A_DIM);
            }
        }

        // Set current line selection box
        wprintw(win, call_group_exists(table->group, call) ? " [*] " : " [ ] ");

        // Print requested columns
        for (guint j = 0; j < g_ptr_array_len(table->columns); j++) {
            Attribute *attr = g_ptr_array_index(table->columns, j);
            // Get current column data
            const gchar *col_text = msg_get_attribute(msg, attr);

            // Enable attribute color (if not current one)
            gint color = 0;
            if (table->cur_idx != (gint) i) {
                if ((color = attribute_get_color(attr, col_text)) > 0) {
                    wattron(win, color);
                }
            }

            // Add the column text to the existing columns
            gint col_width = CLAMP(
                attribute_get_length(attr),
                0,
                getmaxx(win) - getcurx(win) - 2
            );

            if (col_width > 0) {
                wprintw(
                    win,
                    "%-*.*s ",
                    col_width,
                    col_width,
                    (col_text != NULL) ? col_text : " "
                );
            }

            // Disable attribute color
            if (color > 0)
                wattroff(win, color);
        }

        wprintw(win, "%*s", getmaxx(win) - getcurx(win), " ");
        wattroff(win, COLOR_PAIR(CP_DEFAULT));
        wattroff(win, COLOR_PAIR(CP_DEF_ON_BLUE));
        wattroff(win, A_BOLD | A_REVERSE | A_DIM);
    }

    // Copy fixed columns
//    guint fixed_width = call_list_columns_width(self, (guint) setting_get_intvalue(SETTING_TUI_CL_FIXEDCOLS));
//    copywin(pad, self->list_win, 0, 0, 0, 0, listh - 1, fixed_width, 0);

    // Setup horizontal scrollbar
    table->hscroll.max = sng_table_columns_width(table, 0);
    table->hscroll.preoffset = 1;    // Leave first column for vscroll

    // Setup vertical scrollbar
    table->vscroll.max = g_ptr_array_len(table->dcalls) - 1;
    table->vscroll.preoffset = 1;    // Leave first row for titles
    if (scrollbar_visible(table->hscroll)) {
        table->vscroll.postoffset = 1; // Leave last row for hscroll
    }

    // Draw scrollbars if required
    scrollbar_draw(table->hscroll);
    scrollbar_draw(table->vscroll);

    // Print Autoscroll indicator
    if (table->autoscroll) {
        mvwprintw(win, 0, 0, "A");
    }

    SNG_WIDGET_CLASS(sng_table_parent_class)->draw(widget);
}

static void
sng_table_focus_gained(SngWidget *widget)
{
    // Chain up parent focus gained
    SNG_WIDGET_CLASS(sng_table_parent_class)->focus_gained(widget);
}

static void
sng_table_focus_lost(SngWidget *widget)
{
    // Chain up parent focus lost
    SNG_WIDGET_CLASS(sng_table_parent_class)->focus_lost(widget);
}

static void
sng_table_key_pressed(SngWidget *widget, gint key)
{
    SngTable *table = SNG_TABLE(widget);

    // Check actions for this key
    SngAction action = ACTION_NONE;
    while ((action = key_find_action(key, action)) != ACTION_NONE) {
        // This panel has handled the key successfully
        if (sng_table_handle_action(widget, action)) {
            break;
        }
    }

    // Disable autoscroll on some key pressed
    switch (action) {
        case ACTION_DOWN:
        case ACTION_UP:
        case ACTION_HNPAGE:
        case ACTION_HPPAGE:
        case ACTION_NPAGE:
        case ACTION_PPAGE:
        case ACTION_BEGIN:
        case ACTION_END:
            table->autoscroll = 0;
            break;
        default:
            break;
    }

    if (action == ACTION_NONE) {
         SNG_WIDGET_CLASS(sng_table_parent_class)->key_pressed(widget, key);
    }
}

static void
sng_table_clicked(SngWidget *widget, MEVENT mevent)
{
    SngTable *table = SNG_TABLE(widget);

    // Check if the header line was clicked
    if (mevent.y == sng_widget_get_ypos(widget)) {
        return;
    }

    // Select the clicked line
    table->cur_idx = table->first_idx + (mevent.y - sng_widget_get_ypos(widget) - 1);

    // Check if the checkbox was selected
    if (mevent.x >= 1 && mevent.x <= 3) {
        // TODO Handle actions instead of keys
        sng_table_key_pressed(widget, KEY_SPACE);
    }
}

static void
sng_table_finalize(GObject *object)
{
    SngTable *table = SNG_TABLE(object);

    // Deallocate window private data
    call_group_free(table->group);
    g_ptr_array_free(table->columns, TRUE);
    g_ptr_array_free(table->dcalls, TRUE);

    // Chain-up parent finalize function
    G_OBJECT_CLASS(sng_table_parent_class)->finalize(object);
}


SngWidget *
sng_table_new()
{
    return g_object_new(
        SNG_TYPE_TABLE,
        "hexpand", TRUE,
        "vexpand", TRUE,
        NULL
    );
}

void
sng_table_free(SngTable *table)
{
    g_object_unref(table);
}

static void
sng_table_init(SngTable *table)
{
    table->autoscroll = setting_enabled(SETTING_TUI_CL_AUTOSCROLL);
    table->group = call_group_new();
}

static void
sng_table_class_init(SngTableClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = sng_table_finalize;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->realize = sng_table_realize;
    widget_class->draw = sng_table_draw;
    widget_class->focus_gained = sng_table_focus_gained;
    widget_class->focus_lost = sng_table_focus_lost;
    widget_class->key_pressed = sng_table_key_pressed;
    widget_class->clicked = sng_table_clicked;

    signals[SIG_ACTIVATE] =
        g_signal_newv("activate",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_LAST,
                      NULL,
                      NULL, NULL,
                      NULL,
                      G_TYPE_NONE, 0, NULL
        );
}
