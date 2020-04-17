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
 * @file button.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include "tui/keybinding.h"
#include "tui/widgets/button.h"

enum
{
    SIG_ACTIVATE,
    SIGS
};


static guint signals[SIGS] = { 0 };

// Menu button class definition
G_DEFINE_TYPE(SngButton, sng_button, SNG_TYPE_LABEL)

SngWidget *
sng_button_new(const gchar *text)
{
        return g_object_new(
        SNG_TYPE_BUTTON,
        "text", text,
        "min-height", 1,
        "height", 1,
        "width", (text != NULL) ? sng_label_get_text_len(text) : 0,
        "hexpand", TRUE,
        NULL
    );
}

void
sng_button_free(SngButton *button)
{
    g_object_unref(button);
}

static void
sng_button_activate(SngButton *button)
{
    g_signal_emit(SNG_WIDGET(button), signals[SIG_ACTIVATE], 0);
}

static gint
sng_button_key_pressed(SngWidget *widget, gint key)
{
    SngButton *button = SNG_BUTTON(widget);

    // Check actions for this key
    KeybindingAction action = ACTION_UNKNOWN;
    while ((action = key_find_action(key, action)) != ACTION_UNKNOWN) {
        // Check if we handle this action
        switch (action) {
            case ACTION_CONFIRM:
                sng_button_activate(button);
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

    // Return if this panel has handled or not the key
    return (action == ERR) ? KEY_NOT_HANDLED : KEY_HANDLED;
}

static gint
sng_button_clicked(SngWidget *widget, G_GNUC_UNUSED MEVENT mevent)
{
    sng_button_activate(SNG_BUTTON(widget));
    return 0;
}

static void
sng_button_init(G_GNUC_UNUSED SngButton *self)
{
}

static void
sng_button_class_init(SngButtonClass *klass)
{
    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->key_pressed = sng_button_key_pressed;
    widget_class->clicked = sng_button_clicked;

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
