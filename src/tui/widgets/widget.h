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
 * @file widget.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Basic Widget interface component
 */

#ifndef __SNGREP_WIDGET_H__
#define __SNGREP_WIDGET_H__

#include <glib.h>
#include <glib-object.h>
#include <ncurses.h>

G_BEGIN_DECLS

#define TUI_TYPE_WIDGET widget_get_type()
G_DECLARE_DERIVABLE_TYPE(Widget, widget, TUI, WIDGET, GObject)

//! Possible key handler results
typedef enum
{
    KEY_HANDLED = 0,        // Panel has handled the key, dont'use default key handler
    KEY_NOT_HANDLED = -1,   // Panel has not handled the key, try default key handler
    KEY_PROPAGATED = -2,    // Panel destroys and requests previous panel to handle key
    KEY_DESTROY = -3,       // Panel request destroy
} WidgetKeyHandlerRet;

/**
 * @brief Panel information structure
 *
 * This struct contains the panel related data, including
 * a pointer to the function that manages its drawing
 */
struct _WidgetClass
{
    //! Parent class
    GObjectClass parent;
    //! Request the panel to redraw its data
    gint (*draw)(Widget *widget);
    //! Callback for focused event
    gboolean (*focus_gained)(Widget *widget);
    //! Callback for focused event
    void (*focus_lost)(Widget *widget);
    //! Handle a custom keybinding on this panel
    gint (*key_pressed)(Widget *widget, gint key);
    //! Handle a mouse event on this panel
    gint (*clicked)(Widget *widget, MEVENT event);
};

/**
 * @brief Create a ncurses panel for the given ui
 *
 * Create a panel and associated widget and store their
 * pointers in ui structure.
 * If height and width doesn't match the screen dimensions
 * the panel will be centered on the screen.
 *
 * @param height panel widget height
 * @param width panel windo width
 * @return widget instance pointer
 */
Widget *
widget_new(const Widget *parent);

/**
 * @brief Destroy a panel structure
 *
 * Removes the panel associated to the given ui and free
 * its memory. Most part of this task is done in the custom
 * destroy function of the panel.
 *
 * @param widget UI structure
 */
void
widget_destroy(Widget *widget);

void
widget_show(Widget *widget);

void
widget_hide(Widget *widget);

gboolean
widget_is_visible(Widget *widget);

Widget *
widget_get_toplevel(Widget *widget);

Widget *
widget_get_parent(Widget *widget);

GNode *
widget_get_node(Widget *widget);

/**
 * @brief Notifies current ui the screen size has changed
 *
 * This function acts as wrapper to custom ui resize functions
 * with some checks
 *
 * @param widget UI structure
 * @return 0 if ui has been resize, -1 otherwise
 */
gint
widget_draw(Widget *widget);

/**
 * @brief Callback when widget receives window focus
 * @param widget Widget to be focused
 * @return TRUE if the widget can be focused, FALSE otherwise
 */
gboolean
widget_focus_gain(Widget *widget);

/**
 * @brief Callback when widget loses window focus
 * @param widget Widget to remove focus from
 */
void
widget_focus_lost(Widget *widget);

/**
 * @brief Handle moves events on given widget
 *
 * This function will pass the mouse event
 * to the given widget function.
 *
 * @return enum @key_handler_ret*
 */
gint
widget_clicked(Widget *widget, MEVENT event);

/**
 * @brief Handle key inputs on given UI
 *
 * This function will pass the input key sequence
 * to the given UI.
 *
 * @param ui UI structure
 * @param key keycode sequence of the pressed keys and mods
 * @return enum @key_handler_ret*
 */
gint
widget_key_pressed(Widget *widget, gint key);

void
widget_set_ncurses_window(Widget *widget, WINDOW *win);

WINDOW *
widget_get_ncurses_window(Widget *widget);

void
widget_set_size(Widget *widget, gint width, gint height);

void
widget_set_width(Widget *widget, gint width);

gint
widget_get_width(Widget *widget);

void
widget_set_height(Widget *widget, gint height);

gint
widget_get_height(Widget *widget);

void
widget_set_position(Widget *widget, gint xpos, gint ypos);

gint
widget_get_xpos(Widget *widget);

gint
widget_get_ypos(Widget *widget);

void
widget_set_vexpand(Widget *widget, gboolean expand);

gboolean
widget_get_vexpand(Widget *widget);

void
widget_set_hexpand(Widget *widget, gboolean expand);

gboolean
widget_get_hexpand(Widget *widget);

G_END_DECLS

#endif /* __SNGREP_WIDGET_H__ */
