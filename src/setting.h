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
 * @file setting.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage application settings
 *
 * This file contains the functions to manage application settings and
 * options resource files. Configuration will be parsed in this order,
 * from less to more priority, so the later will overwrite the previous.
 *
 *  - Initialization
 *  - \@sysdir\@/sngreprc
 *  - $HOME/.sngreprc
 *  - $SNGREPRC
 *
 * This is a basic approach to configuration, but at least a minimum is required
 * for those who can not see all the list columns or want to disable colours in
 * every sngrep execution.
 */

#ifndef __SNGREP_SETTING_H
#define __SNGREP_SETTING_H

#include <glib.h>
#include <glib-object.h>
#include "storage/attribute.h"
#include "storage/storage.h"
#include "tui/tui.h"
#include "tui/windows/call_flow_win.h"

//! Max setting value
#define SETTING_MAX_LEN   1024

//! Shorter declaration of settings structs
typedef struct _Setting Setting;
typedef struct _SettingAlias SettingAlias;
typedef struct _SettingExtenIp SettingExtenIp;
typedef struct _SettingStorageOpts SettingOpts;
typedef struct _SettingStorage SettingStorage;

//! Other useful defines
#define SETTING_ON  "on"
#define SETTING_OFF "off"

//! Available setting Options
typedef enum
{
    SETTING_UNKNOWN = -1,
    SETTING_CAPTURE_LIMIT,
    SETTING_CAPTURE_DEVICE,
    SETTING_CAPTURE_OUTFILE,
#ifdef WITH_SSL
    SETTING_CAPTURE_KEYFILE,
    SETTING_CAPTURE_TLSSERVER,
#endif
#ifdef USE_HEP
    SETTING_CAPTURE_HEP_SEND,
    SETTING_CAPTURE_HEP_SEND_VER,
    SETTING_CAPTURE_HEP_SEND_ADDR,
    SETTING_CAPTURE_HEP_SEND_PORT,
    SETTING_CAPTURE_HEP_SEND_PASS,
    SETTING_CAPTURE_HEP_SEND_ID,
    SETTING_CAPTURE_HEP_LISTEN,
    SETTING_CAPTURE_HEP_LISTEN_VER,
    SETTING_CAPTURE_HEP_LISTEN_ADDR,
    SETTING_CAPTURE_HEP_LISTEN_PORT,
    SETTING_CAPTURE_HEP_LISTEN_PASS,
    SETTING_CAPTURE_HEP_LISTEN_UUID,
#endif
    SETTING_PACKET_IP,
    SETTING_PACKET_UDP,
    SETTING_PACKET_TCP,
    SETTING_PACKET_TLS,
    SETTING_PACKET_HEP,
    SETTING_PACKET_WS,
    SETTING_PACKET_SIP,
    SETTING_PACKET_SDP,
    SETTING_PACKET_RTP,
    SETTING_PACKET_RTCP,
    SETTING_STORAGE_RTP,
    SETTING_STORAGE_MODE,
    SETTING_STORAGE_MEMORY_LIMIT,
    SETTING_STORAGE_ROTATE,
    SETTING_STORAGE_INCOMPLETE_DLG,
    SETTING_STORAGE_CALLS,
    SETTING_STORAGE_SAVEPATH,
    SETTING_STORAGE_FILTER_PAYLOAD,
    SETTING_STORAGE_FILTER_METHODS,
    SETTING_TUI_DISPLAY_ALIAS,
    SETTING_TUI_BACKGROUND = 0,
    SETTING_TUI_COLORMODE,
    SETTING_TUI_SYNTAX,
    SETTING_TUI_SYNTAX_TAG,
    SETTING_TUI_SYNTAX_BRANCH,
    SETTING_TUI_ALTKEY_HINT,
    SETTING_TUI_EXITPROMPT,
    SETTING_TUI_CL_SCROLLSTEP,
    SETTING_TUI_CL_COLORATTR,
    SETTING_TUI_CL_AUTOSCROLL,
    SETTING_TUI_CL_SORTFIELD,
    SETTING_TUI_CL_SORTORDER,
    SETTING_TUI_CL_FIXEDCOLS,
    SETTING_TUI_CL_COLUMNS,
    SETTING_TUI_CF_FORCERAW,
    SETTING_TUI_CF_RAWMINWIDTH,
    SETTING_TUI_CF_RAWFIXEDWIDTH,
    SETTING_TUI_CF_SPLITCALLID,
    SETTING_TUI_CF_HIGHTLIGHT,
    SETTING_TUI_CF_SCROLLSTEP,
    SETTING_TUI_CF_LOCALHIGHLIGHT,
    SETTING_TUI_CF_SDP_INFO,
    SETTING_TUI_CF_MEDIA,
    SETTING_TUI_CF_ONLYMEDIA,
    SETTING_TUI_CF_DELTA,
    SETTING_TUI_CF_HIDEDUPLICATE,
    SETTING_TUI_CR_SCROLLSTEP,
    SETTING_TUI_CR_NON_ASCII,
    SETTING_COUNT
} SettingId;

/**
 * @brief Configurable Setting structure
 */
struct _Setting
{
    //! Setting name
    const gchar *name;
    //! Value of the setting
    GValue value;
};

/**
 * @brief Alias setting structure
 */
struct _SettingAlias
{
    //! Original address value
    gchar *address;
    //! Alias name
    gchar *alias;

};

/**
 * @brief Externip setting structure
 */
struct _SettingExtenIp
{
    //! Original address value
    gchar *address;
    //! Twin address value
    gchar *externip;

};

struct _SettingStorageOpts
{
    //! Use default settings values
    gboolean use_defaults;
    //! Also read settings from given file
    const gchar *file;
};

struct _SettingStorage
{
    //! Array of settings
    GPtrArray *values;
    //! List of configured IP address aliases
    GList *alias;
    //! List of configure IP address extern
    GList *externips;
};

Setting *
setting_by_id(SettingId id);

Setting *
setting_by_name(const gchar *name);

/**
 * @brief Return the setting id of a given string
 *
 * @param name String representing configurable setting
 * @return setting id or -1 if setting is not found
 */
SettingId
setting_id(const gchar *name);

/**
 * @brief Return string representing given setting id
 *
 * @param id Setting id from settings enum
 * @return string representation of setting or NULL
 */
const gchar *
setting_name(SettingId id);

GType
setting_get_type(SettingId id);

const gchar **
setting_valid_values(SettingId id);

const gchar *
setting_get_value(SettingId id);

gint
setting_get_intvalue(SettingId id);

gint
setting_get_enum(SettingId id);

void
setting_set_value(SettingId id, const gchar *value);

void
setting_set_intvalue(SettingId id, gint value);

gboolean
setting_enabled(SettingId id);

gboolean
setting_disabled(SettingId id);


void
setting_toggle(SettingId id);

gint
setting_enum_next(SettingId id);

gint
setting_column_pos(Attribute *attr);

/**
 * @brief Get alias for a given address (string)
 *
 * @param address IP Address
 * @return configured alias or address if alias not found
 */
const gchar *
setting_get_alias(const gchar *address);

/**
 * @brief Get external ip  for a given address (string)
 *
 * @param address IP Address
 * @return configured external ip or NULL
 */
const gchar *
setting_get_externip(const gchar *address);

/**
 * @brief Read optionuration directives from file
 *
 * This function will parse passed filenames searching for configuration
 * directives of sngrep. See documentation for a list of available
 * directives and attributes
 *
 * @param fname Full path configuration file name
 * @return 0 in case of parse success, -1 otherwise
 */
int
setting_read_file(const gchar *fname);

/**
 * @brief Initialize all program options
 *
 * This function will give all available settings an initial value.
 * This values can be overridden using resources files, either from system dir
 * or user home dir.
 *
 * @param no_config Do not load config file if set to 1
 * @return 0 in all cases
 */
gint
settings_init(SettingOpts options);

/**
 * @brief Deallocate options memory
 *
 * Deallocate memory used for program configurations
 */
void
settings_deinit();

/**
 * @brief Dump configuration settings
 *
 * This function will print to stdout configuration settings
 * after reading system/local/user resource files (in that order).
 *
 */
void
settings_dump();

#endif /* __SNGREP_SETTING_H */
