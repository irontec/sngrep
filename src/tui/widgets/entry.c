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
 * @file entry.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include "tui/keybinding.h"
#include "tui/widgets/entry.h"

enum
{
    SIG_ACTIVATE,
    SIGS
};

static guint signals[SIGS] = { 0 };

// Menu entry class definition
G_DEFINE_TYPE(Entry, entry, TUI_TYPE_WIDGET)

Widget *
entry_new()
{
    return g_object_new(
        TUI_TYPE_ENTRY,
        "min-height", 1,
        "height", 1,
        "hexpand", TRUE,
        NULL
    );
}

void
entry_free(Entry *entry)
{
    g_object_unref(entry);
}

void
entry_set_text(Entry *entry, const gchar *text)
{
    set_field_buffer(entry->fields[0], 0, text);
}

const gchar *
entry_get_text(Entry *entry)
{
    return g_strchomp(field_buffer(entry->fields[0], 0));
}

static void
entry_activate(Entry *entry)
{
    g_signal_emit(TUI_WIDGET(entry), signals[SIG_ACTIVATE], 0);
}

static void
entry_realize(Widget *widget)
{
    Entry *entry = TUI_ENTRY(widget);

    // Chain up parent realize
    TUI_WIDGET_CLASS(entry_parent_class)->realize(widget);

    // Create entry field
    if (entry->form == NULL) {
        entry->fields = g_malloc0_n(sizeof(FIELD *), 2);
        entry->fields[0] = new_field(
            widget_get_height(widget),
            widget_get_width(widget),
            0, 0,
            0, 0
        );
        set_field_back(entry->fields[0], A_UNDERLINE);

        entry->form = new_form(entry->fields);
        set_form_sub(entry->form, widget_get_ncurses_window(widget));
        set_current_field(entry->form, entry->fields[0]);
    }
}

static gint
entry_draw(Widget *widget)
{
    Entry *entry = TUI_ENTRY(widget);
    post_form(entry->form);

    // Move the cursor in main screen
    if (widget_has_focus(widget)) {
        gint x, y;
        // Get subwindow current cursor position
        getyx(widget_get_ncurses_window(widget), y, x);
        // Position cursor in toplevel window
        wmove(
            widget_get_ncurses_window(widget_get_toplevel(widget)),
            y + widget_get_ypos(widget),
            x + widget_get_xpos(widget)
        );
    }

    return TUI_WIDGET_CLASS(entry_parent_class)->draw(widget);
}

static gboolean
entry_focus_gained(Widget *widget)
{
    Entry *entry = TUI_ENTRY(widget);
    // Enable cursor
    curs_set(1);
    // Change field background
    set_field_back(entry->fields[0], A_REVERSE);
    // Move to the last character
    form_driver(entry->form, REQ_END_LINE);
    // Update field form
    post_form(entry->form);
    // Chain up parent focus gained
    return TUI_WIDGET_CLASS(entry_parent_class)->focus_gained(widget);
}

static void
entry_focus_lost(Widget *widget)
{
    Entry *entry = TUI_ENTRY(widget);
    // Disable cursor
    curs_set(0);
    // Change field background
    set_field_back(entry->fields[0], A_NORMAL);
    // Chain up parent focus lost
    TUI_WIDGET_CLASS(entry_parent_class)->focus_lost(widget);
}

static gint
entry_key_pressed(Widget *widget, gint key)
{
    Entry *entry = TUI_ENTRY(widget);

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_PRINTABLE:
                // If this is a normal character on input field, print it
                form_driver(entry->form, key);
                break;
            case ACTION_RIGHT:
                form_driver(entry->form, REQ_RIGHT_CHAR);
                break;
            case ACTION_LEFT:
                form_driver(entry->form, REQ_LEFT_CHAR);
                break;
            case ACTION_BEGIN:
                form_driver(entry->form, REQ_BEG_LINE);
                break;
            case ACTION_END:
                form_driver(entry->form, REQ_END_LINE);
                break;
            case ACTION_CLEAR:
                form_driver(entry->form, REQ_BEG_LINE);
                form_driver(entry->form, REQ_CLR_EOL);
                break;
            case ACTION_DELETE:
                form_driver(entry->form, REQ_DEL_CHAR);
                break;
            case ACTION_BACKSPACE:
                form_driver(entry->form, REQ_DEL_PREV);
                break;
            case ACTION_CONFIRM:
                widget_focus_lost(widget);
                entry_activate(entry);
                break;
            default:
                // Parse next action
                continue;
        }

        // We've handled this key, stop checking actions
        break;
    }

    // Validate all input data
    form_driver(entry->form, REQ_VALIDATION);

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

static void
entry_init(G_GNUC_UNUSED Entry *self)
{
}

static void
entry_class_init(EntryClass *klass)
{
    WidgetClass *widget_class = TUI_WIDGET_CLASS(klass);
    widget_class->realize = entry_realize;
    widget_class->draw = entry_draw;
    widget_class->focus_gained = entry_focus_gained;
    widget_class->focus_lost = entry_focus_lost;
    widget_class->key_pressed = entry_key_pressed;

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
