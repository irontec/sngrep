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
#define SETTING_CAPTURE_LIMIT           "capture.limit"
#define SETTING_CAPTURE_DEVICE          "capture.device"
#define SETTING_CAPTURE_OUTFILE         "capture.outfile"
#ifdef WITH_SSL
#define SETTING_CAPTURE_KEYFILE         "capture.keyfile"
#define SETTING_CAPTURE_TLSSERVER       "capture.tlsserver"
#endif
#ifdef USE_HEP
#define SETTING_CAPTURE_HEP_SEND        "capture.hep.send"
#define SETTING_CAPTURE_HEP_SEND_VER    "capture.hep.send.version"
#define SETTING_CAPTURE_HEP_SEND_ADDR   "capture.hep.send.address"
#define SETTING_CAPTURE_HEP_SEND_PORT   "capture.hep.send.port"
#define SETTING_CAPTURE_HEP_SEND_PASS   "capture.hep.send.pass"
#define SETTING_CAPTURE_HEP_SEND_ID     "capture.hep.send.id"
#define SETTING_CAPTURE_HEP_LISTEN      "capture.hep.listen"
#define SETTING_CAPTURE_HEP_LISTEN_VER  "capture.hep.listen.version"
#define SETTING_CAPTURE_HEP_LISTEN_ADDR "capture.hep.listen.address"
#define SETTING_CAPTURE_HEP_LISTEN_PORT "capture.hep.listen.port"
#define SETTING_CAPTURE_HEP_LISTEN_PASS "capture.hep.listen.pass"
#define SETTING_CAPTURE_HEP_LISTEN_UUID "capture.hep.listen.uuid"
#endif
#define SETTING_PACKET_IP               "packet.ip.enabled"
#define SETTING_PACKET_UDP              "packet.udp.enabled"
#define SETTING_PACKET_TCP              "packet.tcp.enabled"
#define SETTING_PACKET_TLS              "packet.tls.enabled"
#define SETTING_PACKET_HEP              "packet.hep.enabled"
#define SETTING_PACKET_WS               "packet.ws.enabled"
#define SETTING_PACKET_SIP              "packet.sip.enabled"
#define SETTING_PACKET_SDP              "packet.sdp.enabled"
#define SETTING_PACKET_RTP              "packet.rtp.enabled"
#define SETTING_PACKET_RTCP             "packet.rtcp.enabled"
#define SETTING_STORAGE_RTP             "storage.rtp"
#define SETTING_STORAGE_MODE            "storage.mode"
#define SETTING_STORAGE_MEMORY_LIMIT    "storage.memory_limit"
#define SETTING_STORAGE_ROTATE          "storage.rotate"
#define SETTING_STORAGE_INCOMPLETE_DLG  "storage.incomplete"
#define SETTING_STORAGE_CALLS           "storage.calls"
#define SETTING_STORAGE_SAVEPATH        "storage.savepath"
#define SETTING_STORAGE_FILTER_PAYLOAD  "storage.filter.payload"
#define SETTING_STORAGE_FILTER_METHODS  "sotrage.filter.methods"
#define SETTING_TUI_DISPLAY_ALIAS       "tui.alias"
#define SETTING_TUI_BACKGROUND          "tui.background"
#define SETTING_TUI_COLORMODE           "tui.colormode"
#define SETTING_TUI_SYNTAX              "tui.syntax"
#define SETTING_TUI_SYNTAX_TAG          "tui.syntax.tag"
#define SETTING_TUI_SYNTAX_BRANCH       "tui.syntax.branch"
#define SETTING_TUI_ALTKEY_HINT         "tui.hints.altkey"
#define SETTING_TUI_EXITPROMPT          "tui.exitpromt"
#define SETTING_TUI_CL_SCROLLSTEP       "tui.cl.scrollstep"
#define SETTING_TUI_CL_COLORATTR        "tui.cl.colorattr"
#define SETTING_TUI_CL_AUTOSCROLL       "tui.cl.autoscroll"
#define SETTING_TUI_CL_SORTFIELD        "tui.cl.sortfield"
#define SETTING_TUI_CL_SORTORDER        "tui.cl.sortorder"
#define SETTING_TUI_CL_FIXEDCOLS        "tui.cl.fixedcols"
#define SETTING_TUI_CL_COLUMNS          "tui.cl.columns"
#define SETTING_TUI_CF_FORCERAW         "tui.cf.raw.force"
#define SETTING_TUI_CF_RAWMINWIDTH      "tui.cf.raw.minwidth"
#define SETTING_TUI_CF_RAWFIXEDWIDTH    "tui.cf.raw.fixedwidth"
#define SETTING_TUI_CF_SPLITCALLID      "tui.cf.splitcallid"
#define SETTING_TUI_CF_HIGHTLIGHT       "tui.cf.highlight"
#define SETTING_TUI_CF_SCROLLSTEP       "tui.cf.scrollstep"
#define SETTING_TUI_CF_LOCALHIGHLIGHT   "tui.cf.localhighlight"
#define SETTING_TUI_CF_SDP_INFO         "tui.cf.sdpinfo"
#define SETTING_TUI_CF_MEDIA            "tui.cf.media"
#define SETTING_TUI_CF_ONLYMEDIA        "tui.cf.onlymedia"
#define SETTING_TUI_CF_DELTA            "tui.cf.delta"
#define SETTING_TUI_CF_HIDEDUPLICATE    "tui.cf.hideduplicate"
#define SETTING_TUI_CR_SCROLLSTEP       "tui.cr.scrollstep"
#define SETTING_TUI_CR_NON_ASCII        "tui.cr.noascii"

/**
 * @brief Configurable Setting structure
 */
struct _Setting
{
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
    GData *values;
    //! List of configured IP address aliases
    GList *alias;
    //! List of configure IP address extern
    GList *externips;
};

Setting *
setting_by_name(const gchar *name);

/**
 * @brief Return string representing given setting id
 *
 * @param id Setting id from settings enum
 * @return string representation of setting or NULL
 */
const gchar *
setting_name(const gchar *id);

GType
setting_get_type(const gchar *id);

const gchar **
setting_valid_values(const gchar *id);

const gchar *
setting_get_value(const gchar *id);

gint
setting_get_intvalue(const gchar *id);

gint
setting_get_enum(const gchar *id);

void
setting_set_value(const gchar *id, const gchar *value);

void
setting_set_intvalue(const gchar *id, gint value);

gboolean
setting_enabled(const gchar *id);

gboolean
setting_disabled(const gchar *id);


void
setting_toggle(const gchar *id);

gint
setting_enum_next(const gchar *id);

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
