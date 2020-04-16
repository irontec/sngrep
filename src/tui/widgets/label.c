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
 * @file label.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#include "config.h"
#include "tui/widgets/label.h"
#include "tui/theme.h"

enum
{
    PROP_TEXT = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

// Menu label class definition
G_DEFINE_TYPE(Label, label, SNG_TYPE_WIDGET)

SngWidget *
label_new(const gchar *text)
{
    gint width = 0;
    if (text != NULL) {
        width = strlen(text);
    }

    return g_object_new(
        TUI_TYPE_LABEL,
        "text", text,
        "min-height", 1,
        "height", 1,
        "width", width,
        "hexpand", TRUE,
        "can-focus", FALSE,
        NULL
    );
}

void
label_free(Label *label)
{
    g_object_unref(label);
}

void
label_set_text(Label *label, const gchar *text)
{
    if (label->text != NULL) {
        g_free(label->text);
    }
    label->text = g_strdup(text);
}

const gchar *
label_get_text(Label *label)
{
    return label->text;
}

static gint
label_draw(SngWidget *widget)
{
    // Chain up parent draw
    SNG_WIDGET_CLASS(label_parent_class)->draw(widget);

    Label *label = TUI_LABEL(widget);
    if (label->text == NULL) {
        return 0;
    }

    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wattron(win, COLOR_PAIR(CP_DEFAULT));
    werase(win);

    g_auto(GStrv) tokens = g_strsplit_set(label->text, "<>", -1);
    gint color = 0;
    for (guint i = 0; i < g_strv_length(tokens); i++) {
        if (g_strcmp0(tokens[i], "red") == 0) {
            color = CP_RED_ON_DEF;
            continue;
        }
        if (g_strcmp0(tokens[i], "green") == 0) {
            color = CP_GREEN_ON_DEF;
            continue;
        }
        if (g_strcmp0(tokens[i], "yellow") == 0) {
            color = CP_YELLOW_ON_DEF;
            continue;
        }
        wattron(win, COLOR_PAIR(color));
        wprintw(win, tokens[i]);
        wattroff(win, COLOR_PAIR(color));
    }

    return 0;
}

static void
label_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    Label *label = TUI_LABEL(self);

    switch (property_id) {
        case PROP_TEXT:
            label->text = g_strdup(g_value_get_string(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
label_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    Label *label = TUI_LABEL(self);
    switch (property_id) {
        case PROP_TEXT:
            g_value_set_string(value, label->text);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
label_init(G_GNUC_UNUSED Label *self)
{
}

static void
label_class_init(LabelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = label_set_property;
    object_class->get_property = label_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = label_draw;

    obj_properties[PROP_TEXT] =
        g_param_spec_string("text",
                            "Menu label text",
                            "Menu label text",
                            "Untitled menu label",
                            G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}
