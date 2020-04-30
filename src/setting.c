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
 * @file setting.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in setting.h
 *
 */
#include "config.h"
#include <string.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include "tui/tui.h"
#include "glib-extra/glib.h"
#include "glib-extra/glib_enum_types.h"
#include "setting.h"

/**
 * @brief Storage settings
 *
 * This struct contains all the configurable options and can be updated
 * from resource files.
 *
 *  - Settings
 *  - Alias
 *  - Call List columns
 */
static SettingStorage *settings = NULL;

/**
 * @brief Convert Enum to String
 * @param src_value Enum type GValue
 * @param dst_value String type GValue
 */
static void
setting_enum_to_string(const GValue *src_value, GValue *dst_value)
{
    g_value_set_static_string(dst_value, g_value_get_enum_nick(src_value));
}

/**
 * @brief Convert String to Enum
 * @param src_value String type GValue
 * @param dst_value Enum type GValue
 */
static void
setting_string_to_enum(const GValue *src_value, GValue *dst_value)
{
    GType enum_type = G_VALUE_TYPE(dst_value);
    GEnumClass *enum_class = g_type_class_ref(enum_type);
    g_return_if_fail(enum_class != NULL);
    GEnumValue *enum_value = g_enum_get_value_by_nick(enum_class, g_value_get_string(src_value));
    g_return_if_fail(enum_value != NULL);
    g_value_set_enum(dst_value, enum_value->value);
}

/**
 * @brief Convert Boolean to String
 * @param src_value Boolean type GValue
 * @param dst_value String type GValue
 */
static void
setting_bool_to_string(const GValue *src_value, GValue *dst_value)
{
    g_value_set_static_string(
        dst_value,
        g_value_get_boolean(src_value) ? "on" : "off"
    );
}

/**
 * @brief Convert String to Boolean
 * @param src_value String type GValue
 * @param dst_value Boolean type GValue
 */
static void
setting_string_to_bool(const GValue *src_value, GValue *dst_value)
{
    const gchar *str_value = g_value_get_string(src_value);
    g_return_if_fail(str_value != NULL);
    g_value_set_boolean(
        dst_value,
        g_strcmp0(str_value, "yes") == 0 || g_strcmp0(str_value, "YES") == 0
        || g_strcmp0(str_value, "true") == 0 || g_strcmp0(str_value, "TRUE") == 0
        || g_strcmp0(str_value, "on") == 0 || g_strcmp0(str_value, "ON") == 0
    );
}

/**
 * @brief Create a new number type setting
 */
static GValue *
setting_number_new(gint value)
{
    GValue *setting = g_value_new(G_TYPE_INT);
    g_value_set_int(setting, value);
    return setting;
}

/**
 * @brief Create a new string type setting
 */
static GValue *
setting_string_new(const gchar *value)
{
    GValue *setting = g_value_new(G_TYPE_STRING);
    g_value_set_static_string(setting, value);
    return setting;
}

/**
 * @brief Create a new boolean type setting
 */
static GValue *
setting_bool_new(gboolean value)
{
    GValue *setting = g_value_new(G_TYPE_BOOLEAN);
    g_value_set_boolean(setting, value);
    return setting;
}

/**
 * @brief Create a new enum type setting
 */
static GValue *
setting_enum_new(gint value, GType enum_type)
{
    GValue *setting = g_value_new(enum_type);
    g_value_set_enum(setting, value);
    return setting;
}

Setting *
setting_by_name(const gchar *name)
{
    return g_datalist_get_data(&settings->values, name);
}

GType
setting_get_type(const gchar *name)
{
    const Setting *setting = setting_by_name(name);
    g_return_val_if_fail(setting != NULL, G_TYPE_INVALID);

    if (G_TYPE_IS_ENUM(G_VALUE_TYPE(&setting->value))) {
        return G_TYPE_ENUM;
    }
    return G_VALUE_TYPE(&setting->value);
}

const gchar **
setting_valid_values(const gchar *name)
{
    const Setting *setting = setting_by_name(name);
    g_return_val_if_fail(setting != NULL, NULL);
    GType enum_type = G_VALUE_TYPE(&setting->value);
    GEnumClass *enum_class = g_type_class_ref(enum_type);
    g_return_val_if_fail(enum_class != NULL, NULL);
    const gchar **values = g_malloc0(sizeof(gpointer) * (enum_class->n_values + 1));
    for (guint i = 0; i < enum_class->n_values; i++) {
        values[i] = enum_class->values[i].value_nick;
    }
    values[enum_class->n_values] = NULL;
    return values;
}

const gchar *
setting_get_value(const gchar *name)
{
    const Setting *setting = setting_by_name(name);
    g_return_val_if_fail(setting != NULL, NULL);
    GValue string_value = G_VALUE_INIT;
    g_value_init(&string_value, G_TYPE_STRING);
    g_value_transform(&setting->value, &string_value);
    return g_value_get_string(&string_value);
}

gint
setting_get_intvalue(const gchar *name)
{
    const Setting *setting = setting_by_name(name);
    g_return_val_if_fail(setting != NULL, -1);
    return g_value_get_int(&setting->value);
}

gint
setting_get_enum(const gchar *name)
{
    const Setting *setting = setting_by_name(name);
    g_return_val_if_fail(setting != NULL, -1);
    return g_value_get_enum(&setting->value);
}

void
setting_set_value(const gchar *name, const gchar *value)
{
    Setting *setting = setting_by_name(name);
    g_return_if_fail(setting != NULL);

    GValue new_value = G_VALUE_INIT;
    g_value_init(&new_value, G_TYPE_STRING);
    g_value_set_static_string(&new_value, value);
    g_value_transform(&new_value, &setting->value);
}

void
setting_set_intvalue(const gchar *id, gint value)
{
    char strvalue[80];
    sprintf(strvalue, "%d", value);
    setting_set_value(id, strvalue);
}

gint
setting_enabled(const gchar *id)
{
    const Setting *setting = setting_by_name(id);
    g_return_val_if_fail(setting != NULL, -1);
    return g_value_get_boolean(&setting->value);
}

gint
setting_disabled(const gchar *id)
{
    const Setting *setting = setting_by_name(id);
    g_return_val_if_fail(setting != NULL, -1);
    return !g_value_get_boolean(&setting->value);
}

void
setting_toggle(const gchar *id)
{
    Setting *setting = setting_by_name(id);
    g_return_if_fail(setting != NULL);

    switch (setting_get_type(id)) {
        case G_TYPE_BOOLEAN:
            g_value_set_boolean(&setting->value, !g_value_get_boolean(&setting->value));
            break;
        case G_TYPE_ENUM:
            g_value_set_enum(&setting->value, setting_enum_next(id));
            break;
        default:
            break;
    }
}

gint
setting_enum_next(const gchar *id)
{
    const Setting *setting = setting_by_name(id);
    g_return_val_if_fail(setting != NULL, -1);
    GType enum_type = G_VALUE_TYPE(&setting->value);
    GEnumClass *enum_class = g_type_class_ref(enum_type);
    g_return_val_if_fail(enum_class != NULL, -1);

    gint next_value = g_value_get_enum(&setting->value) + 1;
    if (next_value > enum_class->maximum)
        next_value = enum_class->minimum;

    return next_value;
}

gint
setting_column_pos(Attribute *attr)
{
    g_auto(GStrv) columns = g_strsplit(setting_get_value(SETTING_TUI_CL_COLUMNS), ",", 0);
    for (gint i = 0; i < (gint) g_strv_length(columns); i++) {
        if (g_strcmp0(columns[i], attr->name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Sets an alias for a given address
 *
 * @param address IP Address
 * @param string representing the alias
 */
static SettingAlias *
setting_alias_new(const gchar *address, const gchar *alias)
{
    SettingAlias *setting = g_malloc0(sizeof(SettingAlias));
    setting->address = g_strdup(address);
    setting->alias = g_strdup(alias);
    return setting;
}

static void
setting_alias_free(SettingAlias *setting, G_GNUC_UNUSED gpointer user_data)
{
    g_return_if_fail(setting != NULL);
    g_free(setting->alias);
    g_free(setting->address);
    g_free(setting);
}

const gchar *
setting_get_alias(const gchar *address)
{
    g_return_val_if_fail(address != NULL, NULL);

    for (GList *l = settings->alias; l != NULL; l = l->next) {
        SettingAlias *alias = l->data;
        if (g_strcmp0(alias->address, address) == 0) {
            return alias->alias;
        }
    }

    return address;
}

/**
 * @brief Sets an externip for a given address
 *
 * @param address IP Address
 * @param string representing the externip
 */
static SettingExtenIp *
setting_externip_new(const gchar *address, const gchar *externip)
{
    SettingExtenIp *setting = g_malloc0(sizeof(SettingExtenIp));
    setting->address = g_strdup(address);
    setting->externip = g_strdup(externip);
    return setting;
}

static void
setting_externip_free(SettingExtenIp *setting, G_GNUC_UNUSED gpointer user_data)
{
    g_return_if_fail(setting != NULL);
    g_free(setting->address);
    g_free(setting->externip);
    g_free(setting);
}

const gchar *
setting_get_externip(const gchar *address)
{
    g_return_val_if_fail(address != NULL, NULL);

    for (GList *l = settings->externips; l != NULL; l = l->next) {
        SettingExtenIp *externip = l->data;
        if (g_strcmp0(externip->address, address) == 0) {
            return externip->externip;
        }
        if (g_strcmp0(externip->externip, address) == 0) {
            return externip->address;
        }
    }

    return NULL;
}

gint
setting_read_file(const gchar *fname)
{
    FILE *fh;
    char line[1024], type[20], option[50], value[50];

    if (!(fh = fopen(fname, "rt")))
        return -1;

    while (fgets(line, 1024, fh) != NULL) {
        // Check if this line is a commentary or empty line
        if (!strlen(line) || *line == '#')
            continue;

        // Get configuration option from setting line
        if (sscanf(line, "%s %s %[^\t\n]", type, option, value) == 3) {
            if (!strcasecmp(type, "set")) {
                Setting *sett = setting_by_name(option);
                if (sett == NULL) {
                    g_printerr("error: Unknown setting: %s\n", option);
                    continue;
                }
                setting_set_value(option, value);
            } else if (!strcasecmp(type, "alias")) {
                settings->alias = g_list_append(settings->alias, setting_alias_new(option, value));
            } else if (!strcasecmp(type, "externip")) {
                settings->externips = g_list_append(settings->externips, setting_externip_new(option, value));
            } else if (!strcasecmp(type, "bind")) {
                key_bind_action(key_action_id(option), key_from_str(value));
            } else if (!strcasecmp(type, "unbind")) {
                key_unbind_action(key_action_id(option), key_from_str(value));
            } else if (!strcasecmp(type, "attr")) {
                attribute_from_setting(option, g_strdup(value));
            }
        }
    }
    fclose(fh);
    return 0;
}

void
settings_add_setting(const gchar *name, GValue *value)
{
    g_datalist_set_data_full(
        &settings->values,
        name,
        value,
        (GDestroyNotify) g_value_free
    );
}

int
settings_init(SettingOpts options)
{
    // Custom user conf file
    const gchar *homedir = g_get_home_dir();

    // Custom transform functions
    g_value_register_transform_func(G_TYPE_ENUM, G_TYPE_STRING, setting_enum_to_string);
    g_value_register_transform_func(G_TYPE_STRING, G_TYPE_ENUM, setting_string_to_enum);
    g_value_register_transform_func(G_TYPE_BOOLEAN, G_TYPE_STRING, setting_bool_to_string);
    g_value_register_transform_func(G_TYPE_STRING, G_TYPE_BOOLEAN, setting_string_to_bool);

    // Allocate memory for settings storage
    settings = g_malloc0(sizeof(SettingStorage));
    g_datalist_init(&settings->values);

    // Default settings values
    settings_add_setting(SETTING_CAPTURE_LIMIT, setting_number_new(20000));
    settings_add_setting(SETTING_CAPTURE_PCAP_DEVICE, setting_string_new("any"));
    settings_add_setting(SETTING_CAPTURE_PCAP_OUTFILE, setting_string_new(NULL));
    settings_add_setting(SETTING_CAPTURE_PCAP_BUFSIZE, setting_number_new(10 * G_BYTES_PER_MEGABYTE));
#ifdef USE_HEP
    settings_add_setting(SETTING_CAPTURE_HEP_SEND, setting_bool_new(FALSE));
    settings_add_setting(SETTING_CAPTURE_HEP_SEND_VER, setting_number_new(3));
    settings_add_setting(SETTING_CAPTURE_HEP_SEND_ADDR, setting_string_new("127.0.0.1"));
    settings_add_setting(SETTING_CAPTURE_HEP_SEND_PORT, setting_number_new(9060));
    settings_add_setting(SETTING_CAPTURE_HEP_SEND_PASS, setting_string_new(""));
    settings_add_setting(SETTING_CAPTURE_HEP_SEND_ID, setting_number_new(2000));
    settings_add_setting(SETTING_CAPTURE_HEP_LISTEN, setting_bool_new(FALSE));
    settings_add_setting(SETTING_CAPTURE_HEP_LISTEN_VER, setting_string_new("3"));
    settings_add_setting(SETTING_CAPTURE_HEP_LISTEN_ADDR, setting_string_new("0.0.0.0"));
    settings_add_setting(SETTING_CAPTURE_HEP_LISTEN_PORT, setting_number_new(9060));
    settings_add_setting(SETTING_CAPTURE_HEP_LISTEN_PASS, setting_string_new(""));
    settings_add_setting(SETTING_CAPTURE_HEP_LISTEN_UUID, setting_bool_new(FALSE));
#endif
    settings_add_setting(SETTING_PACKET_IP, setting_bool_new(TRUE));
    settings_add_setting(SETTING_PACKET_UDP, setting_bool_new(TRUE));
    settings_add_setting(SETTING_PACKET_TCP, setting_bool_new(TRUE));
    settings_add_setting(SETTING_PACKET_TLS, setting_bool_new(FALSE));
#ifdef WITH_SSL
    settings_add_setting(SETTING_PACKET_TLS_KEYFILE, setting_string_new(NULL));
    settings_add_setting(SETTING_PACKET_TLS_SERVER, setting_string_new(NULL));
#endif
    settings_add_setting(SETTING_PACKET_HEP, setting_bool_new(FALSE));
    settings_add_setting(SETTING_PACKET_WS, setting_bool_new(FALSE));
    settings_add_setting(SETTING_PACKET_SIP, setting_bool_new(TRUE));
    settings_add_setting(SETTING_PACKET_SDP, setting_bool_new(TRUE));
    settings_add_setting(SETTING_PACKET_RTP, setting_bool_new(TRUE));
    settings_add_setting(SETTING_PACKET_RTCP, setting_bool_new(TRUE));
    settings_add_setting(SETTING_STORAGE_MEMORY_LIMIT, setting_string_new("250M"));
    settings_add_setting(SETTING_STORAGE_RTP, setting_bool_new(FALSE));
    settings_add_setting(SETTING_STORAGE_MODE,
                         setting_enum_new(SETTING_STORAGE_MODE_MEMORY, SETTING_TYPE_STORAGE_MODE));
    settings_add_setting(SETTING_STORAGE_ROTATE, setting_bool_new(FALSE));
    settings_add_setting(SETTING_STORAGE_INCOMPLETE_DLG, setting_bool_new(FALSE));
    settings_add_setting(SETTING_STORAGE_CALLS, setting_bool_new(FALSE));
    settings_add_setting(SETTING_STORAGE_SAVEPATH, setting_string_new(g_get_current_dir()));
    settings_add_setting(SETTING_STORAGE_FILTER_METHODS, setting_string_new(NULL));
    settings_add_setting(SETTING_STORAGE_FILTER_PAYLOAD, setting_string_new(NULL));
    settings_add_setting(SETTING_TUI_BACKGROUND, setting_enum_new(SETTING_BACKGROUND_DARK, SETTING_TYPE_BACKGROUND));
    settings_add_setting(SETTING_TUI_COLORMODE, setting_enum_new(SETTING_COLORMODE_REQUEST, SETTING_TYPE_COLOR_MODE));
    settings_add_setting(SETTING_TUI_SYNTAX, setting_bool_new(TRUE));
    settings_add_setting(SETTING_TUI_SYNTAX_TAG, setting_bool_new(FALSE));
    settings_add_setting(SETTING_TUI_SYNTAX_BRANCH, setting_bool_new(FALSE));
    settings_add_setting(SETTING_TUI_ALTKEY_HINT, setting_bool_new(FALSE));
    settings_add_setting(SETTING_TUI_EXITPROMPT, setting_bool_new(TRUE));
    settings_add_setting(SETTING_TUI_DISPLAY_ALIAS, setting_bool_new(FALSE));
    settings_add_setting(SETTING_TUI_CL_COLUMNS, setting_string_new("index,method,sipfrom,sipto,msgcnt,src,dst,state"));
    settings_add_setting(SETTING_TUI_CL_SCROLLSTEP, setting_number_new(4));
    settings_add_setting(SETTING_TUI_CL_COLORATTR, setting_bool_new(TRUE));
    settings_add_setting(SETTING_TUI_CL_AUTOSCROLL, setting_bool_new(FALSE));
    settings_add_setting(SETTING_TUI_CL_SORTFIELD, setting_string_new(ATTR_CALLINDEX));
    settings_add_setting(SETTING_TUI_CL_SORTORDER, setting_string_new("asc"));
    settings_add_setting(SETTING_TUI_CL_FIXEDCOLS, setting_number_new(2));
    settings_add_setting(SETTING_TUI_CF_FORCERAW, setting_bool_new(TRUE));
    settings_add_setting(SETTING_TUI_CF_RAWMINWIDTH, setting_number_new(40));
    settings_add_setting(SETTING_TUI_CF_RAWFIXEDWIDTH, setting_number_new(0));
    settings_add_setting(SETTING_TUI_CF_SPLITCALLID, setting_bool_new(FALSE));
    settings_add_setting(SETTING_TUI_CF_HIGHTLIGHT,
                         setting_enum_new(SETTING_ARROW_HIGHLIGH_BOLD, SETTING_TYPE_ARROW_HIGHLIGHT));
    settings_add_setting(SETTING_TUI_CF_SCROLLSTEP, setting_number_new(4));
    settings_add_setting(SETTING_TUI_CF_LOCALHIGHLIGHT, setting_bool_new(TRUE));
    settings_add_setting(SETTING_TUI_CF_SDP_INFO, setting_enum_new(SETTING_SDP_OFF, SETTING_TYPE_SDP_MODE));
    settings_add_setting(SETTING_TUI_CF_MEDIA, setting_bool_new(TRUE));
    settings_add_setting(SETTING_TUI_CF_ONLYMEDIA, setting_bool_new(FALSE));
    settings_add_setting(SETTING_TUI_CF_DELTA, setting_bool_new(TRUE));
    settings_add_setting(SETTING_TUI_CF_HIDEDUPLICATE, setting_bool_new(FALSE));
    settings_add_setting(SETTING_TUI_CR_SCROLLSTEP, setting_number_new(10));
    settings_add_setting(SETTING_TUI_CR_NON_ASCII, setting_string_new("."));

    // Done if config file should not be read
    if (!options.use_defaults) {

        // Read options from configuration files
        setting_read_file("/etc/sngreprc");
        setting_read_file("/usr/local/etc/sngreprc");

        // Get user configuration
        if (getenv("SNGREPRC") != NULL) {
            setting_read_file(getenv("SNGREPRC"));
        } else if (homedir != NULL) {
            GString *userconf = g_string_new(homedir);
            g_string_append(userconf, "/.sngreprc");
            setting_read_file(userconf->str);
            g_string_free(userconf, TRUE);
        }
    }

    // Override settings if requested
    if (options.file != NULL) {
        setting_read_file(options.file);
    }

    return 0;
}

void
settings_deinit()
{
    g_list_foreach(settings->alias, (GFunc) setting_alias_free, NULL);
    g_list_free(settings->alias);
    g_list_foreach(settings->externips, (GFunc) setting_externip_free, NULL);
    g_list_free(settings->externips);
    g_datalist_clear(&settings->values);
    g_free(settings);
}

void
settings_dump_setting(GQuark key_id, G_GNUC_UNUSED gpointer data, G_GNUC_UNUSED gpointer user_data)
{
    g_print("SettingName: %-30s Value: %s\n",
            g_quark_to_string(key_id),
            setting_get_value(g_quark_to_string(key_id))
    );
}

void
settings_dump_alias(SettingAlias *alias, G_GNUC_UNUSED gpointer user_data)
{
    g_print("Address: %s\t Alias: %s\n", alias->address, alias->alias);
}

void
settings_dump_externip(SettingExtenIp *externip, G_GNUC_UNUSED gpointer user_data)
{
    g_print("Address: %s\t ExternIP: %s\n", externip->address, externip->externip);
}

void
settings_dump()
{
    g_print("\nSettings List\n===============\n");
    g_datalist_foreach(&settings->values, settings_dump_setting, NULL);

    if (settings->alias) {
        g_print("\nAddress Alias\n===============\n");
        g_list_foreach(settings->alias, (GFunc) settings_dump_alias, NULL);
    }

    if (settings->externips) {
        g_print("\nAddress ExternIP\n===============\n");
        g_list_foreach(settings->externips, (GFunc) settings_dump_externip, NULL);
    }
}
