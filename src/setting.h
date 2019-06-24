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
 * optionuration resource files. Configuration will be parsed in this order,
 * from less to more priority, so the later will overwrite the previous.
 *
 *  - Initialization
 *  - \@sysdir\@/sngreprc
 *  - $HOME/.sngreprc
 *  - $SNGREPRC
 *
 * This is a basic approach to configuration, but at least a minimun is required
 * for those who can not see all the list columns or want to disable colours in
 * every sngrep execution.
 */

#ifndef __SNGREP_SETTING_H
#define __SNGREP_SETTING_H

#include <glib.h>
#include "storage/attribute.h"

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
enum SettingId
{
    SETTING_UNKNOWN = -1,
    SETTING_BACKGROUND = 0,
    SETTING_COLORMODE,
    SETTING_SYNTAX,
    SETTING_SYNTAX_TAG,
    SETTING_SYNTAX_BRANCH,
    SETTING_ALTKEY_HINT,
    SETTING_EXITPROMPT,
    SETTING_CAPTURE_LIMIT,
    SETTING_CAPTURE_DEVICE,
    SETTING_CAPTURE_OUTFILE,
#ifdef WITH_SSL
    SETTING_CAPTURE_KEYFILE,
    SETTING_CAPTURE_TLSSERVER,
#endif
    SETTING_CAPTURE_RTP,
    SETTING_CAPTURE_STORAGE,
    SETTING_CAPTURE_ROTATE,
    SETTING_CAPTURE_PACKET_IP,
    SETTING_CAPTURE_PACKET_UDP,
    SETTING_CAPTURE_PACKET_TCP,
    SETTING_CAPTURE_PACKET_TLS,
    SETTING_CAPTURE_PACKET_HEP,
    SETTING_CAPTURE_PACKET_WS,
    SETTING_CAPTURE_PACKET_SIP,
    SETTING_CAPTURE_PACKET_SDP,
    SETTING_CAPTURE_PACKET_RTP,
    SETTING_CAPTURE_PACKET_RTCP,
    SETTING_SIP_NOINCOMPLETE,
    SETTING_SIP_HEADER_X_CID,
    SETTING_SIP_CALLS,
    SETTING_SAVEPATH,
    SETTING_DISPLAY_ALIAS,
    SETTING_CL_SCROLLSTEP,
    SETTING_CL_COLORATTR,
    SETTING_CL_AUTOSCROLL,
    SETTING_CL_SORTFIELD,
    SETTING_CL_SORTORDER,
    SETTING_CL_FIXEDCOLS,
    SETTING_CL_COL_INDEX_POS,
    SETTING_CL_COL_INDEX_WIDTH,
    SETTING_CL_COL_SIPFROM_POS,
    SETTING_CL_COL_SIPFROM_WIDTH,
    SETTING_CL_COL_SIPFROMUSER_POS,
    SETTING_CL_COL_SIPFROMUSER_WIDTH,
    SETTING_CL_COL_SIPTO_POS,
    SETTING_CL_COL_SIPTO_WIDTH,
    SETTING_CL_COL_SIPTOUSER_POS,
    SETTING_CL_COL_SIPTOUSER_WIDTH,
    SETTING_CL_COL_SRC_POS,
    SETTING_CL_COL_SRC_WIDTH,
    SETTING_CL_COL_DST_POS,
    SETTING_CL_COL_DST_WIDTH,
    SETTING_CL_COL_CALLID_POS,
    SETTING_CL_COL_CALLID_WIDTH,
    SETTING_CL_COL_XCALLID_POS,
    SETTING_CL_COL_XCALLID_WIDTH,
    SETTING_CL_COL_DATE_POS,
    SETTING_CL_COL_DATE_WIDTH,
    SETTING_CL_COL_TIME_POS,
    SETTING_CL_COL_TIME_WIDTH,
    SETTING_CL_COL_METHOD_POS,
    SETTING_CL_COL_METHOD_WIDTH,
    SETTING_CL_COL_TRANSPORT_POS,
    SETTING_CL_COL_TRANSPORT_WIDTH,
    SETTING_CL_COL_MSGCNT_POS,
    SETTING_CL_COL_MSGCNT_WIDTH,
    SETTING_CL_COL_CALLSTATE_POS,
    SETTING_CL_COL_CALLSTATE_WIDTH,
    SETTING_CL_COL_CONVDUR_POS,
    SETTING_CL_COL_CONVDUR_WIDTH,
    SETTING_CL_COL_TOTALDUR_POS,
    SETTING_CL_COL_TOTALDUR_WIDTH,
    SETTING_CL_COL_REASON_TXT_POS,
    SETTING_CL_COL_REASON_TXT_WIDTH,
    SETTING_CL_COL_WARNING_POS,
    SETTING_CL_COL_WARNING_WIDTH,
    SETTING_CF_FORCERAW,
    SETTING_CF_RAWMINWIDTH,
    SETTING_CF_RAWFIXEDWIDTH,
    SETTING_CF_SPLITCALLID,
    SETTING_CF_HIGHTLIGHT,
    SETTING_CF_SCROLLSTEP,
    SETTING_CF_LOCALHIGHLIGHT,
    SETTING_CF_SDP_INFO,
    SETTING_CF_MEDIA,
    SETTING_CF_ONLYMEDIA,
    SETTING_CF_DELTA,
    SETTING_CR_SCROLLSTEP,
    SETTING_CR_NON_ASCII,
    SETTING_FILTER_PAYLOAD,
    SETTING_FILTER_METHODS,
#ifdef USE_HEP
    SETTING_HEP_SEND,
    SETTING_HEP_SEND_VER,
    SETTING_HEP_SEND_ADDR,
    SETTING_HEP_SEND_PORT,
    SETTING_HEP_SEND_PASS,
    SETTING_HEP_SEND_ID,
    SETTING_HEP_LISTEN,
    SETTING_HEP_LISTEN_VER,
    SETTING_HEP_LISTEN_ADDR,
    SETTING_HEP_LISTEN_PORT,
    SETTING_HEP_LISTEN_PASS,
    SETTING_HEP_LISTEN_UUID,
#endif
    SETTING_COUNT
};

//! Available setting formats
enum SettingFormat
{
    SETTING_FMT_STRING = 0,
    SETTING_FMT_BOOLEAN,
    SETTING_FMT_NUMBER,
    SETTING_FMT_ENUM,
};

/**
 * @brief Configurable Setting structure
 */
struct _Setting
{
    //! Setting name
    const gchar *name;
    //! Setting format
    enum SettingFormat fmt;
    //! Value of the setting
    char value[SETTING_MAX_LEN];
    //! Comma separated valid values
    gchar **valuelist;
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
setting_by_id(enum SettingId id);

Setting *
setting_by_name(const gchar *name);

/**
 * @brief Return the setting id of a given string
 *
 * @param name String representing configurable setting
 * @return setting id or -1 if setting is not found
 */
enum SettingId
setting_id(const gchar *name);

/**
 * @brief Return string representing given setting id
 *
 * @param id Setting id from settings enum
 * @return string representation of setting or NULL
 */
const gchar *
setting_name(enum SettingId id);

gint
setting_format(enum SettingId id);

const gchar **
setting_valid_values(enum SettingId id);

const gchar *
setting_get_value(enum SettingId id);

gint
setting_get_intvalue(enum SettingId id);

void
setting_set_value(enum SettingId id, const gchar *value);

void
setting_set_intvalue(enum SettingId id, gint value);

gint
setting_enabled(enum SettingId id);

gint
setting_disabled(enum SettingId id);

gint
setting_has_value(enum SettingId id, const gchar *value);

void
setting_toggle(enum SettingId id);

const char *
setting_enum_next(enum SettingId id, const gchar *value);

gint
setting_column_pos(enum AttributeId id);

/**
 * @brief Get Attribute column display width
 *
 * @param id Attribute id
 * @return configured column width
 */
gint
setting_column_width(enum AttributeId id);

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
 * This values can be overriden using resources files, either from system dir
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
