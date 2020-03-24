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
#include "tui/keybinding.h"
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
static Setting *
setting_number_new(const gchar *name, gint value)
{
    Setting *setting = g_malloc0(sizeof(Setting));
    setting->name = name;
    g_value_init(&setting->value, G_TYPE_INT);
    g_value_set_int(&setting->value, value);
    return setting;
}

/**
 * @brief Create a new string type setting
 */
static Setting *
setting_string_new(const gchar *name, const gchar *value)
{
    Setting *setting = g_malloc0(sizeof(Setting));
    setting->name = name;
    g_value_init(&setting->value, G_TYPE_STRING);
    g_value_set_static_string(&setting->value, value);
    return setting;
}

/**
 * @brief Create a new boolean type setting
 */
static Setting *
setting_bool_new(const gchar *name, gboolean value)
{
    Setting *setting = g_malloc0(sizeof(Setting));
    setting->name = name;
    g_value_init(&setting->value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&setting->value, value);
    return setting;
}

/**
 * @brief Create a new enum type setting
 */
static Setting *
setting_enum_new(const gchar *name, gint value, GType enum_type)
{
    Setting *setting = g_malloc0(sizeof(Setting));
    setting->name = name;
    g_value_init(&setting->value, enum_type);
    g_value_set_enum(&setting->value, value);
    return setting;
}

static void
setting_free(Setting *setting)
{
    g_return_if_fail(setting != NULL);
    g_value_unset(&setting->value);
    g_free(setting);
}

Setting *
setting_by_id(SettingId id)
{
    return g_ptr_array_index(settings->values, id);
}

Setting *
setting_by_name(const gchar *name)
{
    for (guint i = 0; i < g_ptr_array_len(settings->values); i++) {
        Setting *setting = g_ptr_array_index(settings->values, i);
        if (g_strcmp0(setting->name, name) == 0) {
            return setting;
        }
    }
    return NULL;
}

SettingId
setting_id(const gchar *name)
{
    const Setting *sett = setting_by_name(name);
    return (sett) ? g_ptr_array_data_index(settings->values, sett) : SETTING_UNKNOWN;
}

const gchar *
setting_name(SettingId id)
{
    const Setting *setting = setting_by_id(id);
    g_return_val_if_fail(setting != NULL, NULL);
    return setting->name;
}

GType
setting_get_type(SettingId id)
{
    const Setting *setting = setting_by_id(id);
    g_return_val_if_fail(setting != NULL, G_TYPE_INVALID);

    if (G_TYPE_IS_ENUM(G_VALUE_TYPE(&setting->value))) {
        return G_TYPE_ENUM;
    }
    return G_VALUE_TYPE(&setting->value);
}

const gchar **
setting_valid_values(SettingId id)
{
    const Setting *setting = setting_by_id(id);
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
setting_get_value(SettingId id)
{
    const Setting *setting = setting_by_id(id);
    g_return_val_if_fail(setting != NULL, NULL);
    GValue string_value = G_VALUE_INIT;
    g_value_init(&string_value, G_TYPE_STRING);
    g_value_transform(&setting->value, &string_value);
    return g_value_get_string(&string_value);
}

gint
setting_get_intvalue(SettingId id)
{
    const Setting *setting = setting_by_id(id);
    g_return_val_if_fail(setting != NULL, -1);
    return g_value_get_int(&setting->value);
}

gint
setting_get_enum(SettingId id)
{
    const Setting *setting = setting_by_id(id);
    g_return_val_if_fail(setting != NULL, -1);
    return g_value_get_enum(&setting->value);
}

void
setting_set_value(SettingId id, const gchar *value)
{
    Setting *setting = setting_by_id(id);
    g_return_if_fail(setting != NULL);

    GValue new_value = G_VALUE_INIT;
    g_value_init(&new_value, G_TYPE_STRING);
    g_value_set_static_string(&new_value, value);
    g_value_transform(&new_value, &setting->value);
}

void
setting_set_intvalue(SettingId id, gint value)
{
    char strvalue[80];
    sprintf(strvalue, "%d", value);
    setting_set_value(id, strvalue);
}

gint
setting_enabled(SettingId id)
{
    const Setting *setting = setting_by_id(id);
    g_return_val_if_fail(setting != NULL, -1);
    return g_value_get_boolean(&setting->value);
}

gint
setting_disabled(SettingId id)
{
    const Setting *setting = setting_by_id(id);
    g_return_val_if_fail(setting != NULL, -1);
    return !g_value_get_boolean(&setting->value);
}

void
setting_toggle(SettingId id)
{
    Setting *setting = setting_by_id(id);
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
setting_enum_next(SettingId id)
{
    const Setting *setting = setting_by_id(id);
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
    g_auto(GStrv) columns = g_strsplit(setting_get_value(SETTING_CL_COLUMNS), ",", 0);
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
                SettingId id = setting_id(option);
                if (id == SETTING_UNKNOWN) {
                    g_printerr("error: Unknown setting: %s\n", option);
                    continue;
                }
                setting_set_value(id, value);
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

int
settings_init(SettingOpts options)
{
    // Custom user conf file
    const gchar *homedir = g_get_home_dir();
    gchar *curdir = g_get_current_dir();

    // Custom transform functions
    g_value_register_transform_func(G_TYPE_ENUM, G_TYPE_STRING, setting_enum_to_string);
    g_value_register_transform_func(G_TYPE_BOOLEAN, G_TYPE_STRING, setting_bool_to_string);
    g_value_register_transform_func(G_TYPE_STRING, G_TYPE_BOOLEAN, setting_string_to_bool);

    // Allocate memory for settings storage
    settings = g_malloc0(sizeof(SettingStorage));
    settings->values = g_ptr_array_new_with_free_func((GDestroyNotify) setting_free);
    g_ptr_array_set_size(settings->values, SETTING_COUNT);

    // Default settings values
    g_ptr_array_set(settings->values, SETTING_BACKGROUND,
                    setting_enum_new("background", SETTING_BACKGROUND_DARK, SETTING_TYPE_BACKGROUND));
    g_ptr_array_set(settings->values, SETTING_COLORMODE,
                    setting_enum_new("colormode", SETTING_COLORMODE_REQUEST, SETTING_TYPE_COLOR_MODE));
    g_ptr_array_set(settings->values, SETTING_SYNTAX,
                    setting_bool_new("syntax", TRUE));
    g_ptr_array_set(settings->values, SETTING_SYNTAX_TAG,
                    setting_bool_new("syntax.tag", FALSE));
    g_ptr_array_set(settings->values, SETTING_SYNTAX_BRANCH,
                    setting_bool_new("syntax.branch", FALSE));
    g_ptr_array_set(settings->values, SETTING_ALTKEY_HINT,
                    setting_bool_new("hintkeyalt", FALSE));
    g_ptr_array_set(settings->values, SETTING_EXITPROMPT,
                    setting_bool_new("exitprompt", TRUE));
    g_ptr_array_set(settings->values, SETTING_MEMORY_LIMIT,
                    setting_string_new("memory_limit", "250M"));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_LIMIT,
                    setting_number_new("capture.limit", 20000));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_DEVICE,
                    setting_string_new("capture.device", "any"));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_OUTFILE,
                    setting_string_new("capture.outfile", NULL));
#ifdef WITH_SSL
    g_ptr_array_set(settings->values, SETTING_CAPTURE_KEYFILE,
                    setting_string_new("capture.keyfile", NULL));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_TLSSERVER,
                    setting_string_new("capture.tlsserver", NULL));
#endif
    g_ptr_array_set(settings->values, SETTING_CAPTURE_RTP,
                    setting_bool_new("capture.rtp", FALSE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_IP,
                    setting_bool_new("packet.ip.enabled", TRUE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_UDP,
                    setting_bool_new("packet.udp.enabled", TRUE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_TCP,
                    setting_bool_new("packet.tcp.enabled", TRUE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_TLS,
                    setting_bool_new("packet.tls.enabled", FALSE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_HEP,
                    setting_bool_new("packet.hep.enabled", FALSE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_WS,
                    setting_bool_new("packet.ws.enabled", FALSE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_SIP,
                    setting_bool_new("packet.sip.enabled", TRUE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_SDP,
                    setting_bool_new("packet.sdp.enabled", TRUE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_RTP,
                    setting_bool_new("packet.rtp.enabled", TRUE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_PACKET_RTCP,
                    setting_bool_new("packet.rtcp.enabled", TRUE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_STORAGE,
                    setting_enum_new("capture.storage", SETTING_STORAGE_MODE_MEMORY, SETTING_TYPE_STORAGE_MODE));
    g_ptr_array_set(settings->values, SETTING_CAPTURE_ROTATE,
                    setting_bool_new("capture.rotate", FALSE));
    g_ptr_array_set(settings->values, SETTING_SIP_NOINCOMPLETE,
                    setting_bool_new("sip.noincomplete", TRUE));
    g_ptr_array_set(settings->values, SETTING_STORAGE_MAX_QUEUE,
                    setting_number_new("storage.max_queue_size", 1000));
    g_ptr_array_set(settings->values, SETTING_SIP_CALLS,
                    setting_bool_new("sip.calls", FALSE));
    g_ptr_array_set(settings->values, SETTING_SAVEPATH,
                    setting_string_new("savepath", curdir));
    g_ptr_array_set(settings->values, SETTING_DISPLAY_ALIAS,
                    setting_bool_new("displayalias", FALSE));
    g_ptr_array_set(settings->values, SETTING_CL_COLUMNS,
                    setting_string_new("cl.columns", "index,method,sipfrom,sipto,msgcnt,src,dst,state"));
    g_ptr_array_set(settings->values, SETTING_CL_SCROLLSTEP,
                    setting_number_new("cl.scrollstep", 4));
    g_ptr_array_set(settings->values, SETTING_CL_COLORATTR,
                    setting_bool_new("cl.colorattr", TRUE));
    g_ptr_array_set(settings->values, SETTING_CL_AUTOSCROLL,
                    setting_bool_new("cl.autoscroll", FALSE));
    g_ptr_array_set(settings->values, SETTING_CL_SORTFIELD,
                    setting_string_new("cl.sortfield", ATTR_CALLINDEX));
    g_ptr_array_set(settings->values, SETTING_CL_SORTORDER,
                    setting_string_new("cl.sortorder", "asc"));
    g_ptr_array_set(settings->values, SETTING_CL_FIXEDCOLS,
                    setting_number_new("cl.fixedcols", 2));
    g_ptr_array_set(settings->values, SETTING_CF_FORCERAW,
                    setting_bool_new("cf.forceraw", TRUE));
    g_ptr_array_set(settings->values, SETTING_CF_RAWMINWIDTH,
                    setting_number_new("cf.rawminwidth", 40));
    g_ptr_array_set(settings->values, SETTING_CF_RAWFIXEDWIDTH,
                    setting_number_new("cf.rawfixedwidth", 0));
    g_ptr_array_set(settings->values, SETTING_CF_SPLITCALLID,
                    setting_bool_new("cf.splitcallid", FALSE));
    g_ptr_array_set(settings->values, SETTING_CF_HIGHTLIGHT,
                    setting_enum_new("cf.highlight", SETTING_ARROW_HIGHLIGH_BOLD, SETTING_TYPE_ARROW_HIGHLIGHT));
    g_ptr_array_set(settings->values, SETTING_CF_SCROLLSTEP,
                    setting_number_new("cf.scrollstep", 4));
    g_ptr_array_set(settings->values, SETTING_CF_LOCALHIGHLIGHT,
                    setting_bool_new("cf.localhighlight", TRUE));
    g_ptr_array_set(settings->values, SETTING_CF_SDP_INFO,
                    setting_enum_new("cf.sdpinfo", SETTING_SDP_OFF, SETTING_TYPE_SDP_MODE));
    g_ptr_array_set(settings->values, SETTING_CF_MEDIA,
                    setting_bool_new("cf.media", TRUE));
    g_ptr_array_set(settings->values, SETTING_CF_ONLYMEDIA,
                    setting_bool_new("cf.onlymedia", FALSE));
    g_ptr_array_set(settings->values, SETTING_CF_DELTA,
                    setting_bool_new("cf.deltatime", TRUE));
    g_ptr_array_set(settings->values, SETTING_CF_HIDEDUPLICATE,
                    setting_bool_new("cf.hideduplicate", FALSE));
    g_ptr_array_set(settings->values, SETTING_CR_SCROLLSTEP,
                    setting_number_new("cr.scrollstep", 10));
    g_ptr_array_set(settings->values, SETTING_CR_NON_ASCII,
                    setting_string_new("cr.nonascii", "."));
    g_ptr_array_set(settings->values, SETTING_FILTER_PAYLOAD,
                    setting_string_new("filter.payload", NULL));
    g_ptr_array_set(settings->values, SETTING_FILTER_METHODS,
                    setting_string_new("filter.methods", NULL));
#ifdef USE_HEP
    g_ptr_array_set(settings->values, SETTING_HEP_SEND,
                    setting_bool_new("hep.send", FALSE));
    g_ptr_array_set(settings->values, SETTING_HEP_SEND_VER,
                    setting_number_new("hep.send.version", 3));
    g_ptr_array_set(settings->values, SETTING_HEP_SEND_ADDR,
                    setting_string_new("hep.send.address", "127.0.0.1"));
    g_ptr_array_set(settings->values, SETTING_HEP_SEND_PORT,
                    setting_number_new("hep.send.port", 9060));
    g_ptr_array_set(settings->values, SETTING_HEP_SEND_PASS,
                    setting_string_new("hep.send.pass", ""));
    g_ptr_array_set(settings->values, SETTING_HEP_SEND_ID,
                    setting_number_new("hep.send.id", 2000));
    g_ptr_array_set(settings->values, SETTING_HEP_LISTEN,
                    setting_bool_new("hep.listen", FALSE));
    g_ptr_array_set(settings->values, SETTING_HEP_LISTEN_VER,
                    setting_string_new("hep.listen.version", "3"));
    g_ptr_array_set(settings->values, SETTING_HEP_LISTEN_ADDR,
                    setting_string_new("hep.listen.address", "0.0.0.0"));
    g_ptr_array_set(settings->values, SETTING_HEP_LISTEN_PORT,
                    setting_number_new("hep.listen.port", 9060));
    g_ptr_array_set(settings->values, SETTING_HEP_LISTEN_PASS,
                    setting_string_new("hep.listen.pass", ""));
    g_ptr_array_set(settings->values, SETTING_HEP_LISTEN_UUID,
                    setting_bool_new("hep.listen.uuid", FALSE));
#endif

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
    g_free(curdir);

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
    g_ptr_array_free(settings->values, TRUE);
    g_free(settings);
}

void
settings_dump()
{
    g_print("\nSettings List\n===============\n");
    for (guint i = 1; i < SETTING_COUNT; i++) {
        g_print("SettingId: %d\t SettingName: %-30s Value: %s\n", i,
                setting_name(i),
                setting_get_value(i));
    }

    if (settings->alias) {
        g_print("\nAddress Alias\n===============\n");
        for (GList *l = settings->alias; l != NULL; l = l->next) {
            SettingAlias *alias = l->data;
            g_print("Address: %s\t Alias: %s\n",
                    alias->address,
                    alias->alias
            );
        }
    }

    if (settings->externips) {
        g_print("\nAddress ExternIP\n===============\n");
        for (GList *l = settings->externips; l != NULL; l = l->next) {
            SettingExtenIp *externip = l->data;
            g_print("Address: %s\t ExternIP: %s\n",
                    externip->address,
                    externip->externip
            );
        }
    }
}
