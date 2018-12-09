AC_PREREQ([2.59])
AC_INIT([sngrep], [1.4.5], [kaian@irontec.com], [sngrep], [http://www.irontec.com/])
AM_INIT_AUTOMAKE([1.9])
AC_CONFIG_HEADERS([src/config.h])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifdef([AC_CONFIG_MACRO_DIRS], AC_CONFIG_MACRO_DIRS([m4]), m4_include([m4/sngrep.m4]))

AC_COPYRIGHT("Irontec S.L.")

# Define _GNU_SOURCE etc.
m4_ifdef([AC_USE_SYSTEM_EXTENSIONS], [AC_USE_SYSTEM_EXTENSIONS])

# debug compilation
AC_ARG_ENABLE(debug,
    AC_HELP_STRING(--enable-debug, [Debug compilation (Default = no)]),
    enable_debug=$enableval, enable_debug=no)

if test "$enable_debug" = "yes" ; then
    CFLAGS="$CFLAGS  -g -O0 -Wall -Werror -Wno-unused-but-set-variable"
    CXXFLAGS="$CXXFLAGS $CFLAGS"
fi

# Minimum checks for a C program :)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_EGREP
AC_LANG(C)
AM_PROG_CC_C_O

#######################################################################
# Check for other REQUIRED libraries
AC_CHECK_LIB([pcap], [pcap_open_offline], [], [
    AC_MSG_ERROR([ You need to have libpcap installed to compile sngrep.])
])
AC_CHECK_HEADER([pcap.h], [], [
    AC_MSG_ERROR([ You need to have libpcap development files installed to compile sngrep.])
])

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.16])

####
#### Ncurses Wide character support
####
SNGREP_CHECK_SCRIPT([ncursesw6], [addnwstr], [WITH_UNICODE], "ncursesw6-config",
SNGREP_CHECK_SCRIPT([ncursesw], [addnwstr], [WITH_UNICODE], "ncursesw5-config",
 SNGREP_CHECK_SCRIPT([ncurses], [addnwstr], [WITH_UNICODE], "ncurses5-config",
  SNGREP_CHECK_LIB([ncursesw6], [addnwstr], [WITH_UNICODE],
   SNGREP_CHECK_LIB([ncursesw], [addnwstr], [WITH_UNICODE],
    SNGREP_CHECK_LIB([ncurses], [addnwstr], [WITH_UNICODE],
))))))

AC_CHECK_LIB([panelw], [new_panel], [], [
    AC_MSG_ERROR([ You need to have ncurses panelw library installed to compile sngrep.])
])

AC_CHECK_LIB([formw], [new_form], [], [
    AC_MSG_ERROR([ You need to have ncurses formsw library installed to compile sngrep.])
])

AC_CHECK_LIB([menuw], [new_item], [], [
    AC_MSG_ERROR([ You need to have ncurses menuw library installed to compile sngrep.])
])

####
#### SSL Support
####
AC_ARG_WITH([ssl],
    AS_HELP_STRING([--with-ssl], [Enable SSL Support (TLS SIP Transport)]),
    [AC_SUBST(WITH_SSL, $withval)],
    [AC_SUBST(WITH_SSL, no)]
)

AS_IF([test "x$WITH_SSL" == "xyes"], [

    m4_ifdef([PKG_CHECK_MODULES], [
        PKG_CHECK_MODULES([LIBGNUTLS], [gnutls])
    ], [
	AC_CHECK_LIB([gnutls], [gnutls_init], [], [
	    AC_MSG_ERROR([ You need to have gnutls installed to compile sngrep])
	])
    ])

	AC_PATH_PROG([LIBGCRYPT_CONFIG],[libgcrypt-config],[no])
	if test "x${LIBGCRYPT_CONFIG}" = "xno"; then
	    AC_MSG_FAILURE([libgcrypt-config not found in PATH])
	fi
	AC_CHECK_LIB(
		[gcrypt],
		[gcry_md_map_name],
		[LIBGCRYPT_CFLAGS="`${LIBGCRYPT_CONFIG} --cflags`"
		LIBGCRYPT_LIBS="`${LIBGCRYPT_CONFIG} --libs`"
		],
		[AC_MSG_ERROR([ You need to have libgcrypt installed to compile sngrep])],
		[`${LIBGCRYPT_CONFIG} --libs --cflags`]
		)
	AC_DEFINE([WITH_SSL],[],[Compile With GnuTLS compatibility])
	AC_SUBST(LIBGCRYPT_CFLAGS)
	AC_SUBST(LIBGCRYPT_LIBS)
], [])

####
#### IPv6 Support
####
AC_ARG_ENABLE([ipv6],
    AS_HELP_STRING([--enable-ipv6], [Enable IPv6 Support]),
    [AC_SUBST(USE_IPV6, $enableval)],
    [AC_SUBST(USE_IPV6, no)]
)

AS_IF([test "x$USE_IPV6" == "xyes"], [
	AC_CHECK_HEADERS([netinet/in.h netinet/ip6.h], [], [
	    AC_MSG_ERROR([ You dont seem to have ipv6 support (no ip6.h found).])
	], [
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
	])
	AC_DEFINE([USE_IPV6],[],[Compile With IPv6 support])
], [])


####
#### HEP Support
####
AC_ARG_ENABLE([hep],
    AS_HELP_STRING([--enable-hep], [Enable EEP/HEP Support]),
    [AC_SUBST(USE_HEP, $enableval)],
    [AC_SUBST(USE_HEP, no)]
)

AS_IF([test "x$USE_HEP" == "xyes"], [
	AC_DEFINE([USE_HEP],[],[Compile With EEP support])
], [])

# Conditional Source inclusion 
AM_CONDITIONAL([WITH_SSL], [test "x$WITH_SSL" == "xyes"])
AM_CONDITIONAL([USE_HEP], [test "x$USE_HEP" == "xyes"])


######################################################################
# Print Logo
AC_ARG_ENABLE(logo,
    AC_HELP_STRING(--disable-logo, [Disable Irontec Logo from Summary menu]),
    [ enable_logo=$enableval],
    [ enable_logo=yes])

AS_IF([test "x$enable_logo" == "xyes"], [
    echo ''
    echo ' ██╗██████╗  ██████╗ ███╗   ██╗████████╗███████╗ ██████╗'
    echo ' ██║██╔══██╗██╔═══██╗████╗  ██║╚══██╔══╝██╔════╝██╔════╝'
    echo ' ██║██████╔╝██║   ██║██╔██╗ ██║   ██║   █████╗  ██║     '
    echo ' ██║██╔══██╗██║   ██║██║╚██╗██║   ██║   ██╔══╝  ██║     '
    echo ' ██║██║  ██║╚██████╔╝██║ ╚████║   ██║   ███████╗╚██████╗'
    echo ' ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝   ╚═╝   ╚══════╝ ╚═════╝'
    echo ''
])

AC_MSG_NOTICE
AC_MSG_NOTICE(   sngrep configure finished                            	)
AC_MSG_NOTICE( ====================================================== 	)
AC_MSG_NOTICE( SSL Support                  : ${WITH_SSL}		)
AC_MSG_NOTICE( IPv6 Support                 : ${USE_IPV6}               )
AC_MSG_NOTICE( HEP Support                  : ${USE_HEP}               )
AC_MSG_NOTICE( ====================================================== 	)
AC_MSG_NOTICE


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([config/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([tests/Makefile])
AC_OUTPUT
