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
 * @file radio_button.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 */

#include "config.h"
#include "tui/theme.h"
#include "tui/keybinding.h"
#include "tui/widgets/window.h"
#include "tui/widgets/radio_button.h"

enum
{
    PROP_ACTIVE = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

// Class definition
G_DEFINE_TYPE(SngRadioButton, sng_radio_button, SNG_TYPE_BUTTON)

SngWidget *
sng_radio_button_new(const gchar *text)
{
    return g_object_new(
        SNG_TYPE_RADIO_BUTTON,
        "text", text,
        "height", 1,
        "hexpand", TRUE,
        NULL
    );
}

GSList *
sng_radio_button_group_add(GSList *group, SngRadioButton *radio_button)
{
    // Add radio button to the group
    radio_button->group = g_slist_append(group, radio_button);
    return radio_button->group;
}

GSList *
sng_radio_button_get_group(SngRadioButton *radio_button)
{
    return radio_button->group;
}

gboolean
sng_radio_button_is_active(SngRadioButton *radio_button)
{
    return radio_button->active;
}

static void
sng_radio_button_update_group(SngRadioButton *radio_button)
{
    // Current radio button has changed
    for (GSList *l = radio_button->group; l != NULL; l = l->next) {
        SngRadioButton *member = l->data;
        if (member == radio_button) {
            continue;
        }

        // Deactivate the other group radio buttons
        member->active = FALSE;
    }
}

static void
sng_radio_button_draw(SngWidget *widget)
{
    SngRadioButton *radio_button = SNG_RADIO_BUTTON(widget);

    // Update label text based on current status
    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wattron(win, COLOR_PAIR(CP_DEFAULT));
    werase(win);

    if (radio_button->active) {
        wprintw(win, "(*) %s", sng_label_get_text(SNG_LABEL(widget)));
    } else {
        wprintw(win, "( ) %s", sng_label_get_text(SNG_LABEL(widget)));
    }
}

static void
sng_radio_button_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngRadioButton *radio_button = SNG_RADIO_BUTTON(self);
    switch (property_id) {
        case PROP_ACTIVE:
            radio_button->active = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_radio_button_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngRadioButton *radio_button = SNG_RADIO_BUTTON(self);
    switch (property_id) {
        case PROP_ACTIVE:
            g_value_set_boolean(value, radio_button->active);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_radio_button_constructed(GObject *object)
{
    // Add callback to disable other buttons when this one activates
    g_signal_connect(object, "activate",
                     G_CALLBACK(sng_radio_button_update_group), NULL);
}

static void
sng_radio_button_activate(SngButton *button)
{
    SngRadioButton *radio_button = SNG_RADIO_BUTTON(button);
    radio_button->active = TRUE;
}

static void
sng_radio_button_init(G_GNUC_UNUSED SngRadioButton *radio_button)
{
}

static void
sng_radio_button_class_init(SngRadioButtonClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = sng_radio_button_constructed;
    object_class->set_property = sng_radio_button_set_property;
    object_class->get_property = sng_radio_button_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_radio_button_draw;

    SngButtonClass *button_class = SNG_BUTTON_CLASS(klass);
    button_class->activate = sng_radio_button_activate;

    obj_properties[PROP_ACTIVE] =
        g_param_spec_boolean("active",
                             "Radio button active",
                             "Radio button active",
                             FALSE,
                             G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}
