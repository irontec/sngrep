/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2023 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2023 Irontec SL. All rights reserved.
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
 * @file config.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Generated from config.h.cmake by CMake
 *
 */

#ifndef __SNGREP_CONFIG_H
#define __SNGREP_CONFIG_H

/* Parameters of AC_INIT() */
#define PACKAGE "@AC_PACKAGE_NAME@"
#define PACKAGE_BUGREPORT "@AC_PACKAGE_BUGREPORT@"
#define PACKAGE_NAME "@AC_PACKAGE_NAME@"
#define PACKAGE_STRING "@AC_PACKAGE_STRING@"
#define PACKAGE_URL "@AC_PACKAGE_URL@"
#define PACKAGE_VERSION "@AC_PACKAGE_VERSION@"
#define VERSION "@AC_PACKAGE_VERSION@"

/* Define if you have the `fopencookie' function */
#cmakedefine HAVE_FOPENCOOKIE

/* Compile With Unicode compatibility */
#cmakedefine WITH_UNICODE

/* Compile With GnuTLS compatibility */
#cmakedefine WITH_GNUTLS

/* Compile With Openssl compatibility */
#cmakedefine WITH_OPENSSL

/* Compile With Perl Compatible regular expressions support */
#cmakedefine WITH_PCRE

/* Compile With Perl Compatible regular expressions support */
#cmakedefine WITH_PCRE2

/* Compile With zlib support */
#cmakedefine WITH_ZLIB

/* Compile With IPv6 support */
#cmakedefine USE_IPV6

/* Compile With EEP support */
#cmakedefine USE_EEP

/* CMAKE_CURRENT_BINARY_DIR is needed in tests/test_input.c */
#define CMAKE_CURRENT_BINARY_DIR "@CMAKE_CURRENT_BINARY_DIR@"

#endif /* __SNGREP_CONFIG_H */

