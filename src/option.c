/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013,2014 Ivan Alonso (Kaian)
 ** Copyright (C) 2013,2014 Irontec SL. All rights reserved.
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
 * @file option.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source code of functions defined in option.h
 *
 */
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "keybinding.h"
#include "option.h"

/**
 * @brief Configuration options array
 *
 * Contains all availabe options that can be optionured
 * @todo make this dynamic
 */
option_opt_t options[1024];
int optscnt = 0;

int
init_options()
{
    // Custom user conf file
    char userconf[128];
    char *home = getenv("HOME");

    // Set default color options
    set_option_value("color", "on");
    set_option_value("color.request", "on");
    set_option_value("color.callid", "off");
    set_option_value("color.cseq", "off");
    set_option_value("background", "dark");

    // Highlight options
    set_option_value("syntax", "on");
    set_option_value("syntax.branch", "off");
    set_option_value("syntax.tag", "off");

    // Add Call list column options
    set_option_value("cl.column0", "time");
    set_option_value("cl.column0", "sipfrom");
    set_option_value("cl.column1", "sipto");
    set_option_value("cl.column2", "msgcnt");
    set_option_value("cl.column3", "src");
    set_option_value("cl.column4", "dst");
    set_option_value("cl.column5", "method");
    set_option_value("cl.column6", "state");

    // Set Autoscroll in call list
    set_option_value("cl.autoscroll", "on");
    set_option_value("cl.scrollstep", "10");
    set_option_value("cl.defexitbutton", "1");

    // Raw options for Call flow screen
    set_option_value("cf.forceraw", "on");
    set_option_value("cf.rawminwidth", "40");
    set_option_value("cf.splitcallid", "off");
    set_option_value("cf.highlight", "bold");
    set_option_value("cf.scrollstep", "4");
    set_option_value("cf.localhighlight", "on");

    // Default options for Call Raw screen
    set_option_value("cr.scrollstep", "10");

    // Set default mode in message diff screen
    set_option_value("diff.mode", "line");

    // Allow dialogs to be incomplete
    set_option_value("sip.ignoreincomplete", "on");

    // Set default save file location
    set_option_value("sngrep.savepath", home);

    // Set default capture options
    set_option_value("capture.limit", "50000");
    set_option_value("capture.device", "any");
    set_option_value("capture.lookup", "off");

    // Set default filter options
    set_option_value("filter.methods", "REGISTER,INVITE,SUBSCRIBE,NOTIFY,OPTIONS,PUBLISH,MESSAGE");

    // Print defualt keys in bottom bar
    set_option_value("hintkeyalt", "off");

    // Initialize keybindings
    key_bindings_init();

    // Read options from configuration files
    read_options("/etc/sngreprc");
    read_options("/usr/local/etc/sngreprc");
    // Get user homedir configuration
    if (home) {
        sprintf(userconf, "%s/.sngreprc", home);
        read_options(userconf);
    }

    // Unless specified, when capturing with lookup, display
    // hostnames where addresses are printed
    if (!get_option_value("sngrep.displayhost"))
        set_option_value("sngrep.displayhost", is_option_enabled("capture.lookup") ? "on" : "off");

    return 0;
}

void
deinit_options()
{
    int i;
    // Deallocate options memory
    for (i = 0; i < optscnt; i++) {
        free(options[i].opt);
        free(options[i].value);
    }
}

int
read_options(const char *fname)
{
    FILE *fh;
    regex_t empty, setting;
    regmatch_t matches[4];
    char line[1024], *type, *option, *value;

    if (!(fh = fopen(fname, "rt")))
        return -1;

    // Compile expression matching
    regcomp(&empty, "^#|^\\s*$", REG_EXTENDED | REG_NOSUB);
    regcomp(&setting, "^\\s*(\\S+)\\s+(\\S*)\\s+(\\S+)\\s*$", REG_EXTENDED);

    while (fgets(line, 1024, fh) != NULL) {
        // Check if this line is a commentary or empty line
        if (!regexec(&empty, line, 0, NULL, 0))
            continue;
        // Get configuration option from setting line
        if (!regexec(&setting, line, 4, matches, 0)) {
            type = line + matches[1].rm_so;
            line[matches[1].rm_eo] = '\0';
            option = line + matches[2].rm_so;
            line[matches[2].rm_eo] = '\0';
            value = line + matches[3].rm_so;
            line[matches[3].rm_eo] = '\0';
            if (!strcasecmp(type, "set")) {
                set_option_value(option, value);
            } else if (!strcasecmp(type, "ignore")) {
                set_ignore_value(option, value);
            } else if (!strcasecmp(type, "alias")) {
                set_alias_value(option, value);
            } else if (!strcasecmp(type, "bind")) {
                key_bind_action(key_action_id(option), key_from_str(value));
            } else if (!strcasecmp(type, "unbind")) {
                key_unbind_action(key_action_id(option), key_from_str(value));
            }
        }
    }
    regfree(&empty);
    regfree(&setting);
    fclose(fh);
    return 0;
}

const char*
get_option_value(const char *opt)
{
    int i;
    for (i = 0; i < optscnt; i++) {
        if (!strcasecmp(opt, options[i].opt)) {
            return options[i].value;
        }
    }
    return NULL;
}

int
get_option_int_value(const char *opt)
{
    const char *value;
    if ((value = get_option_value(opt))) {
        return atoi(value);
    }
    return -1;
}

void
set_option_int_value(const char *opt, int value)
{
    char cvalue[10];
    sprintf(cvalue, "%d", value);
    set_option_value(opt, cvalue);
}

void
set_option_value(const char *opt, const char *value)
{
    if (!opt || !value)
        return;

    int i;
    if (!get_option_value(opt)) {
        options[optscnt].type = SETTING;
        options[optscnt].opt = strdup(opt);
        options[optscnt].value = strdup(value);
        optscnt++;
    } else {
        for (i = 0; i < optscnt; i++) {
            if (!strcasecmp(opt, options[i].opt)) {
                free(options[i].value);
                options[i].value = strdup(value);
            }
        }
    }
}

int
is_option_enabled(const char *opt)
{
    const char *value;
    if ((value = get_option_value(opt))) {
        if (!strcasecmp(value, "on"))
            return 1;
        if (!strcasecmp(value, "1"))
            return 1;
    }
    return 0;
}

int
is_option_disabled(const char *opt)
{
    const char *value;
    if ((value = get_option_value(opt))) {
        if (!strcasecmp(value, "off"))
            return 1;
        if (!strcasecmp(value, "0"))
            return 1;
    }
    return 0;
}

void
set_ignore_value(const char *opt, const char *value)
{
    options[optscnt].type = IGNORE;
    options[optscnt].opt = strdup(opt);
    options[optscnt].value = strdup(value);
    optscnt++;
}

void
set_alias_value(const char *address, const char *alias)
{
    options[optscnt].type = ALIAS;
    options[optscnt].opt = strdup(address);
    options[optscnt].value = strdup(alias);
    optscnt++;
}

int
is_option_value(const char *opt, const char *expected)
{
    const char *value;
    if ((value = get_option_value(opt)))
        return !strcasecmp(value, expected);
    return 0;
}

int
is_ignored_value(const char *field, const char *fvalue)
{
    int i;
    for (i = 0; i < optscnt; i++) {
        if (!strcasecmp(options[i].opt, field) && !strcasecmp(options[i].value, fvalue)) {
            return 1;
        }
    }
    return 0;
}

const char *
get_alias_value(const char *address)
{
    int i;

    if (!address)
        return NULL;

    for (i = 0; i < optscnt; i++) {
        if (options[i].type != ALIAS)
            continue;
        if (!strcmp(options[i].opt, address))
            return options[i].value;
    }

    return address;
}


void
toggle_option(const char *option)
{
    set_option_value(option, is_option_enabled(option) ? "off" : "on");
}
