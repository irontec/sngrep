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
G_DEFINE_TYPE(SngTable, sng_table, SNG_TYPE_SCROLLABLE)

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
    height -= 1; // Remove header line

    // If horizontal scrolling is visible, we have one line less to draw data
    SngScrollbar *hscroll = sng_scrollable_get_hscroll(SNG_SCROLLABLE(table));
    if (sng_widget_is_visible(SNG_WIDGET(hscroll))) {
        height--;
    }

    // Move the first index if required (moving down)
    table->first_idx = MAX(
        table->first_idx,
        table->cur_idx - height + 1
    );

    // Update vertical scrollbar position
    SngScrollbar *vscroll = sng_scrollable_get_vscroll(SNG_SCROLLABLE(table));
    sng_scrollbar_set_position(vscroll, table->first_idx);
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
    SngScrollbar *hscroll = sng_scrollable_get_hscroll(SNG_SCROLLABLE(table));
    sng_scrollbar_set_position(
        hscroll,
        sng_scrollbar_get_position(hscroll) + times
    );
}

static gboolean
sng_table_handle_action(SngWidget *widget, SngAction action)
{

    SngTable *table = SNG_TABLE(widget);
    guint rnpag_steps = (guint) setting_get_intvalue(SETTING_TUI_CL_SCROLLSTEP);
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
            table->cur_idx = 0;
            // Reset scroll position
            sng_scrollbar_set_position(sng_scrollable_get_vscroll(SNG_SCROLLABLE(table)), 0);
            call_group_remove_all(table->group);
            break;
        case ACTION_CLEAR_CALLS_SOFT:
            // Remove stored calls, keeping the currently displayed calls
            storage_calls_clear_soft();
            // Clear List
            table->cur_idx = 0;
            // Reset scroll position
            sng_scrollbar_set_position(sng_scrollable_get_vscroll(SNG_SCROLLABLE(table)), 0);
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
            storage_sort_toggle_order();
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

void
sng_table_columns_update(SngTable *table)
{
    // Add configured columns
    table->columns = g_ptr_array_new();
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

static gint
sng_table_preferred_height(SngWidget *widget)
{
    SngTable *table = SNG_TABLE(widget);
    return g_ptr_array_len(table->dcalls) + 1 /* column line */;
}

static gint
sng_table_columns_width(SngTable *table, guint columns)
{
    // More requested columns that existing columns??
    if (columns > g_ptr_array_len(table->columns)) {
        columns = g_ptr_array_len(table->columns);
    }

    // If requested column is 0, count all columns
    guint column_cnt = (columns == 0) ? g_ptr_array_len(table->columns) : columns;

    // Add extra width for spaces between columns + selection box
    gint width = 4 + column_cnt;

    // Sum all column widths
    for (guint i = 0; i < column_cnt; i++) {
        Attribute *attr = g_ptr_array_index(table->columns, i);
        width += attr->length;
    }

    return width;
}

static gint
sng_table_preferred_width(SngWidget *widget)
{
    SngTable *table = SNG_TABLE(widget);
    return sng_table_columns_width(table, g_ptr_array_len(table->columns));
}

static void
sng_table_update(SngWidget *widget)
{
    SngTable *table = SNG_TABLE(widget);

    // Get the list of calls that are going to be displayed
    g_ptr_array_free(table->dcalls, TRUE);
    table->dcalls = g_ptr_array_copy_filtered(storage_calls(), (GEqualFunc) filter_check_call, NULL);

    // Add configured columns
    g_ptr_array_free(table->columns, TRUE);
    table->columns = g_ptr_array_new();
    g_auto(GStrv) columns = g_strsplit(setting_get_value(SETTING_TUI_CL_COLUMNS), ",", 0);
    for (guint i = 0; i < g_strv_length(columns); i++) {
        Attribute *attr = attribute_find_by_name(columns[i]);
        g_return_if_fail(attr != NULL);
        // Add column to the list
        g_ptr_array_add(table->columns, attr);
    }
}

static void
sng_table_draw(SngWidget *widget)
{
    SngTable *table = SNG_TABLE(widget);
    SngWidget *content = sng_scrollable_get_content(SNG_SCROLLABLE(table));
    WINDOW *win = sng_widget_get_ncurses_window(content);
    werase(win);

    // Get configured sorting options
    StorageSortOpts sort = storage_sort_options();

    // Draw columns titles
    wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));

    // Draw columns
    wprintw(win, "     ");
    for (guint i = 0; i < g_ptr_array_len(table->columns); i++) {
        Attribute *attr = g_ptr_array_index(table->columns, i);

        gint col_width = CLAMP(
            attribute_get_length(attr),
            0,
            getmaxx(win) - getcurx(win) - 2
        );

        if (col_width > 0) {
            if (attr == sort.by) {
                // Print sort column indicator
                wattron(win, A_BOLD | COLOR_PAIR(CP_YELLOW_ON_CYAN));
                wprintw(
                    win,
                    "%c%-*.*s ",
                    (sort.asc) ? '^' : 'v',
                    col_width - 1,
                    col_width - 1,
                    attribute_get_title(attr)
                );
            } else {
                wattron(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));
                wprintw(
                    win,
                    "%-*.*s ",
                    col_width,
                    col_width,
                    attribute_get_title(attr)
                );
            }
        }
    }

    // If autoscroll is enabled
    if (table->autoscroll) {
        // Print Autoscroll indicator
        mvwprintw(win, 0, 0, "A");
        // Select last dialog
        StorageSortOpts sort = storage_sort_options();
        if (sort.asc) {
            sng_table_move_vertical(table, g_ptr_array_len(table->dcalls));
        } else {
            sng_table_move_vertical(table, g_ptr_array_len(table->dcalls) * -1);
        }
    }

    wprintw(win, "%*s", getmaxx(win) - getcurx(win), " ");
    wattroff(win, A_BOLD | COLOR_PAIR(CP_DEF_ON_CYAN));

    // Fill the call list starting at current scroll position
    for (guint i = 0; i < g_ptr_array_len(table->dcalls); i++) {
        Call *call = g_ptr_array_index(table->dcalls, i);
        g_return_if_fail(call != NULL);

        // Get first call message attributes
        Message *msg = g_ptr_array_first(call->msgs);
        g_return_if_fail(msg != NULL);

        // Show bold selected rows
        if (call_group_exists(table->group, call))
            wattron(win, A_BOLD | COLOR_PAIR(CP_DEFAULT));

        // Highlight active call
        if (table->cur_idx == (gint) i) {
            wattron(win, COLOR_PAIR(CP_WHITE_ON_BLUE));
            if (!sng_widget_has_focus(SNG_WIDGET(table))) {
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

    // Chain-up parent draw function
    SNG_WIDGET_CLASS(sng_table_parent_class)->draw(widget);
}

static void
sng_table_map(SngWidget *widget)
{
    SngScrollable *scrollable = SNG_SCROLLABLE(widget);
    SngScrollbar *vscroll = sng_scrollable_get_vscroll(scrollable);
    SngScrollbar *hscroll = sng_scrollable_get_hscroll(scrollable);
    SngWidget *content = sng_scrollable_get_content(scrollable);

    // Copy lines based on current scrolls position from internal to current
    WINDOW *srcwin = sng_widget_get_ncurses_window(content);
    WINDOW *dstwin = sng_widget_get_ncurses_window(widget);
    gint sminrow = sng_scrollbar_get_position(vscroll);
    gint smincol = sng_scrollbar_get_position(hscroll);;
    gint dmaxrow = sng_widget_get_height(widget) - 1;
    gint dmaxcol = sng_widget_get_width(widget) - 1;

    // Copy the internal table to visible table
    copywin(srcwin, dstwin, sminrow, smincol, 0, 0, dmaxrow, dmaxcol, FALSE);

    // Always overwrite table header
    copywin(srcwin, dstwin, 0, 0, 0, 0, 0, dmaxcol, FALSE);

    // Respect fixed columns in horizontal scrolling
//    gint fixed_columns = setting_get_intvalue(SETTING_TUI_CL_FIXEDCOLS);
//    gint fixed_width = sng_table_columns_width(SNG_TABLE(widget), fixed_columns);
//    copywin(dstwin, dstwin, sminrow, smincol, 0, 0, dmaxrow, fixed_width, FALSE);

    // Chain-up parent map function
    SNG_WIDGET_CLASS(sng_table_parent_class)->map(widget);
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
sng_table_header_clicked(SngWidget *widget, MEVENT mevent)
{
    SngTable *table = SNG_TABLE(widget);

    gint column_xpos = 4;
    for (guint i = 0; i < g_ptr_array_len(table->columns); i++) {
        Attribute *attribute = g_ptr_array_index(table->columns, i);
        column_xpos += attribute->length + 1;
        if (column_xpos >= mevent.x) {
            // If already sorting by this attribute, just toggle sort order
            if (attribute == storage_sort_get_attribute()) {
                storage_sort_toggle_order();
            } else {
                storage_sort_set_attribute(attribute);
            }
            break;
        }
    }
}

static void
sng_table_clicked(SngWidget *widget, MEVENT mevent)
{
    SngTable *table = SNG_TABLE(widget);

    // Check if header was clicked
    if (mevent.y == sng_widget_get_ypos(widget)) {
        sng_table_header_clicked(widget, mevent);
        return;
    }

    // Select the clicked line
    table->cur_idx = table->first_idx + (mevent.y - sng_widget_get_ypos(widget) - 1);

    // Check if the checkbox was selected or Ctrl key is hold
    if ((mevent.x >= 1 && mevent.x <= 3)
        || mevent.bstate & BUTTON_CTRL) {
        // TODO Handle actions instead of keys
        sng_table_key_pressed(widget, KEY_SPACE);
    }

    // Activate selected rows on right click or double click
    if (mevent.bstate & BUTTON3_CLICKED
        || mevent.bstate & BUTTON1_DOUBLE_CLICKED) {
        sng_table_activate(SNG_TABLE(widget));
        return;
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

static void
sng_table_class_init(SngTableClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = sng_table_finalize;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->update = sng_table_update;
    widget_class->draw = sng_table_draw;
    widget_class->map = sng_table_map;
    widget_class->clicked = sng_table_clicked;
    widget_class->key_pressed = sng_table_key_pressed;
    widget_class->preferred_height = sng_table_preferred_height;
    widget_class->preferred_width = sng_table_preferred_width;

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

static void
sng_table_init(SngTable *table)
{
    table->autoscroll = setting_enabled(SETTING_TUI_CL_AUTOSCROLL);
    table->group = call_group_new();
    table->dcalls = g_ptr_array_new();
    table->columns = g_ptr_array_new();

    // Set scrollbar padding
    sng_scrollable_set_padding(SNG_SCROLLABLE(table), 1, 0, 0, 0);
}
