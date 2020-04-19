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
#include "glib-extra/glib.h"
#include "glib-extra/glib_enum_types.h"
#include "tui/widgets/label.h"
#include "tui/theme.h"

enum
{
    PROP_TEXT = 1,
    PROP_ALIGNMENT,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

// Menu label class definition
G_DEFINE_TYPE(SngLabel, sng_label, SNG_TYPE_WIDGET)

SngWidget *
sng_label_new(const gchar *text)
{
    gint width = 0;
    if (text != NULL) {
        width = sng_label_get_text_len(text);
    }

    return g_object_new(
        SNG_TYPE_LABEL,
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
sng_label_free(SngLabel *label)
{
    g_object_unref(label);
}

void
sng_label_set_text(SngLabel *label, const gchar *text)
{
    if (label->text != NULL) {
        g_free(label->text);
    }
    label->text = g_strdup(text);
}

const gchar *
sng_label_get_text(SngLabel *label)
{
    return label->text;
}

gint
sng_label_get_text_len(const gchar *text)
{
    gint length = 0;
    g_auto(GStrv) tokens = g_strsplit_set(text, ">", -1);
    for (guint i = 0; i < g_strv_length(tokens); i++) {
        const gchar *open_tag = g_strstr_len(tokens[i], strlen(tokens[i]), "<");
        if (open_tag == NULL) {
            length += strlen(tokens[i]);
        } else {
            length += open_tag - tokens[i];
        }
    }
    return length;
}

void
sng_label_set_align(SngLabel *label, SngAlignment align)
{
    label->alignment = align;
}

SngAlignment
sng_label_get_align(SngLabel *label)
{
    return label->alignment;
}

static gint
sng_label_draw(SngWidget *widget)
{
    // Chain up parent draw
    SNG_WIDGET_CLASS(sng_label_parent_class)->draw(widget);

    SngLabel *label = SNG_LABEL(widget);
    if (label->text == NULL) {
        return 0;
    }

    WINDOW *win = sng_widget_get_ncurses_window(widget);
    wattron(win, COLOR_PAIR(CP_DEFAULT));
    werase(win);

    // Position text inside widget
    gint width = sng_widget_get_width(widget);
    gint text_len = sng_label_get_text_len(label->text);
    switch (label->alignment) {
        case SNG_ALIGN_CENTER:
            wmove(win, 0, (width - text_len) / 2);
            break;
        case SNG_ALIGN_RIGHT:
            wmove(win, 0, width - text_len);
            break;
        case SNG_ALIGN_LEFT:
        default:
            break;
    }

    g_auto(GStrv) tokens = g_strsplit_set(label->text, "<>", -1);
    gint attr = 0;
    for (guint i = 0; i < g_strv_length(tokens); i++) {
        if (g_strcmp0(tokens[i], "red") == 0) {
            attr = COLOR_PAIR(CP_RED_ON_DEF);
            continue;
        }
        if (g_strcmp0(tokens[i], "green") == 0) {
            attr = COLOR_PAIR(CP_GREEN_ON_DEF);
            continue;
        }
        if (g_strcmp0(tokens[i], "yellow") == 0) {
            attr = COLOR_PAIR(CP_YELLOW_ON_DEF);
            continue;
        }
        if (g_strcmp0(tokens[i], "cyan") == 0) {
            attr = COLOR_PAIR(CP_CYAN_ON_DEF);
            continue;
        }
        if (g_atoi(tokens[i])) {
            attr = g_atoi(tokens[i]);
            continue;
        }
        wattron(win, attr);
        wprintw(win, tokens[i]);
        wattroff(win, attr);
    }

    return 0;
}

static void
sng_label_set_property(GObject *self, guint property_id, const GValue *value, GParamSpec *pspec)
{
    SngLabel *label = SNG_LABEL(self);

    switch (property_id) {
        case PROP_TEXT:
            label->text = g_strdup(g_value_get_string(value));
            break;
        case PROP_ALIGNMENT:
            label->alignment = g_value_get_enum(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_label_get_property(GObject *self, guint property_id, GValue *value, GParamSpec *pspec)
{
    SngLabel *label = SNG_LABEL(self);
    switch (property_id) {
        case PROP_TEXT:
            g_value_set_string(value, label->text);
            break;
        case PROP_ALIGNMENT:
            g_value_set_enum(value, label->alignment);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
            break;
    }
}

static void
sng_label_init(G_GNUC_UNUSED SngLabel *self)
{
}

static void
sng_label_class_init(SngLabelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = sng_label_set_property;
    object_class->get_property = sng_label_get_property;

    SngWidgetClass *widget_class = SNG_WIDGET_CLASS(klass);
    widget_class->draw = sng_label_draw;

    obj_properties[PROP_TEXT] =
        g_param_spec_string("text",
                            "Label text",
                            "Label text",
                            NULL,
                            G_PARAM_READWRITE
        );

    obj_properties[PROP_ALIGNMENT] =
        g_param_spec_enum("alignment",
                          "Label text alignment",
                          "Label text alignment",
                          SNG_TYPE_ALIGNMENT,
                          SNG_ALIGN_LEFT,
                          G_PARAM_READWRITE
        );

    g_object_class_install_properties(
        object_class,
        N_PROPERTIES,
        obj_properties
    );
}
