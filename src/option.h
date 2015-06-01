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
 * @file option.h
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
 *
 * This is a basic approach to configuration, but at least a minimun is required
 * for those who can not see all the list columns or want to disable colours in
 * every sngrep execution.
 *
 */

#ifndef __SNGREP_CONFIG_H
#define __SNGREP_CONFIG_H

//! Shorter declarartion of attribute struct
typedef struct config_option option_opt_t;

enum option_type {
    SETTING = 0,
    IGNORE,
    ALIAS
};

/**
 * @brief Configurable option structure
 *
 * sngrep is optionured by a group of attributes that can be
 * modified using resource files.
 */
struct config_option {
    //! Setting type
    enum option_type type;
    //! Name of attribute
    char *opt;
    //! Value of attribute
    char *value;
};

/**
 * @brief Initialize all program options
 *
 * This function will give all available settings an initial value.
 * This values can be overriden using resources files, either from system dir
 * or user home dir.
 *
 * @return 0 in all cases
 */
int
init_options();

/**
 * @brief Deallocate options memory
 *
 * Deallocate memory used for program configurations
 */
void
deinit_options();

/**
 * @brief Read optionuration directives from file
 *
 * This funtion will parse passed filenames searching for configuration
 * directives of sngrep. See documentation for a list of available
 * directives and attributes
 *
 * @param fname Full path configuration file name
 * @return 0 in case of parse success, -1 otherwise
 */
int
read_options(const char *fname);

/**
 * @brief Get settings option value (string)
 *
 * Used in all the program to access the optionurable options of sngrep
 * Use this function instead of accessing optionuration array.
 *
 * @param opt Name of optionurable option
 * @return configuration option value or NULL if not found
 */
const char*
get_option_value(const char *opt);

/**
 * @brief Get settings option value (int)
 *
 * Basically the same as get_option_value converting the result to
 * integer.
 * Use this function instead of accessing configuration array.
 *
 * @todo -1 is an error!
 *
 * @param opt Name of optionurable option
 * @return option numeric value or -1 in case of error
 */
int
get_option_int_value(const char *opt);

/**
 * @brief Sets a settings option value form an int
 *
 * Basic setter for 'set' directive attributes
 *
 * @param opt Name of configuration option
 * @param value Value of configuration option
 */
void
set_option_int_value(const char *opt, int value);

/**
 * @brief Sets a settings option value
 *
 * Basic setter for 'set' directive attributes
 *
 * @param opt Name of configuration option
 * @param value Value of configuration option
 */
void
set_option_value(const char *opt, const char *value);

/**
 * @brief Sets a ignore option value
 *
 * Basic setter for 'ignore' directive attributes
 *
 * @param opt Name of configuration option
 * @param value Value of configuration option
 */
void
set_ignore_value(const char *opt, const char *value);

/**
 * @brief Sets an alias for a given address
 *
 * @param address IP Address
 * @param string representing the alias
 */
void
set_alias_value(const char *address, const char *alias);

/**
 * @brief Check if a configuration option has a specific value
 *
 * @param opt Name of configuration option
 * @return 1 if value is "on" or "1", 0 otherwise
 */
int
is_option_value(const char *opt, const char *value);

/**
 * @brief Check if a configuration option is "enabled"
 *
 * An option is considered enabled if it has "on" or "1" as value,
 * otherwise will return disabled (0)
 *
 * @param opt Name of configuration option
 * @return 1 if value is "on" or "1", 0 otherwise
 */
int
is_option_enabled(const char *opt);

/**
 * @brief Check if a configuration option is "disabled"
 *
 * An option is considered disabled if it has "off" or "0" as value,
 * otherwise will return enabled (1);
 *
 * @param opt Name of configuration option
 * @return 1 if value is "off" or "0", 0 otherwise
 */
int
is_option_disabled(const char *opt);

/**
 * @brief Check if a exits an ignore directive for the given field and value
 *
 * Like is_option_enabled, this check if there is a match in configuration
 * options that match the given  field and value
 *
 * @param field Name of configuration option
 * @param fvalue Value to check if it has an ignore directive
 * @return 1 if value for field exists
 */
int
is_ignored_value(const char *field, const char *fvalue);

/**
 * @brief Get alias for a given address (string)
 *
 * @param address IP Address
 * @return configured alias or address if not alias found
 */
const char *
get_alias_value(const char *address);

/**
 * @brief Toggle a boolean option
 *
 * An option is considered disabled if it has "off" or "0" as value,
 * otherwise will be enabled.
 * This function will set off for those options with on value, and
 * viceversa.
 *
 * @todo Implement also for "1"/"0" options
 *
 * @param option Name of configuration option
 */
void
toggle_option(const char *option);

#endif
