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
#include "tui/theme.h"
#include "tui/widgets/window.h"
#include "tui/widgets/entry.h"

enum
{
    SIG_ACTIVATE,
    SIGS
};

enum
{
    PROP_TEXT = 1,
    N_PROPERTIES
};

static guint signals[SIGS] = { 0 };

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

// Menu entry class definition
G_DEFINE_TYPE(SngEntry, sng_entry, SNG_TYPE_WIDGET)

SngWidget *
sng_entry_new(const gchar *text)
{
    return g_object_new(
        SNG_TYPE_ENTRY,
        "text", text,
        "height", 1,
        "hexpand", TRUE,
        NULL
    );
}

const gchar *
sng_entry_get_text(SngEntry *entry)
{
    return g_strchomp(field_buffer(entry->fields[0], 0));
}

static void
sng_entry_activate(SngEntry *entry)
{
    g_signal_emit(SNG_WIDGET(entry), signals[SIG_ACTIVATE], 0);
}

static void
sng_entry_realize(SngWidget *widget)
{
    SngEntry *entry = SNG_ENTRY(widget);

    // Chain up parent realize
    SNG_WIDGET_CLASS(sng_entry_parent_class)->realize(widget);

    // Create entry field
    if (entry->form == NULL) {
        entry->fields = g_malloc0_n(sizeof(FIELD *), 2);
        entry->fields[0] = new_field(
            sng_widget_get_height(widget),
            sng_widget_get_width(widget),
            0, 0,
            0, 0
        );
        set_field_back(entry->fields[0], A_UNDERLINE);

        if (entry->text != NULL) {
            set_field_buffer(entry->fields[0], 0, entry->text);
        }

        entry->form = new_form(entry->fields);
        set_form_sub(entry->form, sng_widget_get_ncurses_window(widget));
        set_current_field(entry->form, entry->fields[0]);
    }
}

static void
sng_entry_draw(SngWidget *widget)
{
    SngEntry *entry = SNG_ENTRY(widget);
    post_form(entry->form);

    // Move the cursor in main screen
    if (sng_widget_has_focus(widget)) {
        gint x, y;
        // Get subwindow current cursor position
        getyx(sng_widget_get_ncurses_window(widget), y, x);

        // Position cursor in toplevel window
        SngWidget *toplevel = sng_widget_get_toplevel(widget);
        wmove(
            sng_widget_get_ncurses_window(toplevel),
            y + (sng_widget_get_ypos(widget) - sng_widget_get_ypos(toplevel)),
            x + (sng_widget_get_xpos(widget) - sng_widget_get_xpos(toplevel))
        );
    }

     SNG_WIDGET_CLASS(sng_entry_parent_class)->draw(widget);
}

static void
sng_entry_focus_gained(SngWidget *widget)
{
    SngEntry *entry = SNG_ENTRY(widget);
    // Enable cursor
    curs_set(1);
    // Change field background
    set_field_back(entry->fields[0], A_UNDERLINE | A_REVERSE);
    // Move to the last character
    form_driver(entry->form, REQ_END_LINE);
    // Update field form
    post_form(entry->form);
    // Chain up parent focus gained
     SNG_WIDGET_CLASS(sng_entry_parent_class)->focus_gained(widget);
}

static void
sng_entry_focus_lost(SngWidget *widget)
{
    SngEntry *entry = SNG_ENTRY(widget);
    // Disable cursor
    curs_set(0);
    // Change field background
    set_field_back(entry->fields[0], A_UNDERLINE | A_NORMAL);
    // Chain up parent focus lost
    SNG_WIDGET_CLASS(sng_entry_parent_class)->focus_lost(widget);
}

static void
sng_entry_key_pressed(SngWidget *widget, gint key)
{
    SngEntry *entry = SNG_ENTRY(widget);

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
            case ACTION_UP:
                sng_window_focus_prev(SNG_WINDOW(sng_widget_get_toplevel(widget)));
                break;
            case ACTION_DOWN:
                sng_window_focus_next(SNG_WINDOW(sng_widget_get_toplevel(widget)));
                return;
            case ACTION_CONFIRM:
                sng_entry_activate(entry);
                sng_widget_lose_focus(widget);
                break;
            case ACTION_CANCEL:
                sng_widget_lose_focus(widget);
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
}

static void
sng_entry_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngEntry *entry = SNG_ENTRY(object);

    switch (property_id) {
        case PROP_TEXT:
            entry->text = g_strdup(g_value_get_string(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_entry_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngEntry *entry = SNG_ENTRY(object);
    switch (property_id) {
        case PROP_TEXT:
            g_value_set_string(value, entry->text);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
sng_entry_init(G_GNUC_UNUSED SngEntry *self)
{
}

static void
sng_entry_class_init(SngEntryClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = sng_entry_set_property;
    object_class->get_property = sng_entry_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->realize = sng_entry_realize;
    widget_class->draw = sng_entry_draw;
    widget_class->focus_gained = sng_entry_focus_gained;
    widget_class->focus_lost = sng_entry_focus_lost;
    widget_class->key_pressed = sng_entry_key_pressed;

    obj_properties[PROP_TEXT] =
        g_param_spec_string("text",
                            "Entry default text",
                            "Entry default text",
                            NULL,
                            G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );

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
