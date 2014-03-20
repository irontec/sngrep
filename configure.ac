AC_PREREQ([2.59])
AC_INIT([sngrep], [0.0-alpha], [kaian@irontec.com], [sngrep], [http://www.irontec.com/])
AM_INIT_AUTOMAKE([1.10])
AM_SILENT_RULES([yes])

AC_COPYRIGHT("Irontec S.L.")

# Define _GNU_SOURCE etc.
AC_USE_SYSTEM_EXTENSIONS

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

#######################################################################
# Add configurable options

AC_ARG_ENABLE([ngrep],
    AS_HELP_STRING([--disable-ngrep], [Disable ngrep for parsing messages]))


AC_ARG_ENABLE([libpcap],
    AS_HELP_STRING([--disable-libpcap], [Disable libpcap for parsing messages]))


AS_IF([test "x$enable_ngrep" == "xno"], [
	AS_IF([test "x$enable_libpcap" == "xno"], [
    AC_MSG_ERROR([ You can not disable both ngrep and libpcap. Choose one. ])
])])

#######################################################################
# Check for other REQUIRED libraries

AC_CHECK_HEADER([ncurses.h], [], [
    AC_MSG_ERROR([ You need to have ncurses development files installed to compile sngrep.])
])

AC_CHECK_LIB([ncurses], [initscr], [], [
    AC_MSG_ERROR([ You need to have libncurses installed to compile sngrep.])
])

AC_CHECK_LIB([panel], [new_panel], [], [
    AC_MSG_ERROR([ You need to have ncurses panel library installed to compile sngrep.])
])

AC_CHECK_LIB([form], [new_form], [], [
    AC_MSG_ERROR([ You need to have ncurses forms library installed to compile sngrep.])
])

AC_CHECK_LIB([pthread], [pthread_create], [], [
    AC_MSG_ERROR([ You need to have libpthread installed to compile sngrep.])
])

AS_IF([test "x$enable_libpcap" != "xno"], [
	AC_CHECK_LIB([pcap], [pcap_open_offline], [], [
	    AC_MSG_ERROR([ You need to have libpcap installed to compile sngrep.])
	])
	AC_CHECK_HEADER([pcap.h], [], [
	    AC_MSG_ERROR([ You need to have libpcap development files installed to compile sngrep.])
	])
	AC_DEFINE(WITH_LIBPCAP)
])

#######################################################################
# Check for REQUIRED binaries
AS_IF([test "x$enable_ngrep" != "xno"], [
	AC_DEFINE(WITH_NGREP)

	AC_PATH_PROG(STDBUF, stdbuf, [:], $PATH)
	AS_IF([test "x$STDBUF" != "x:"],
	    [AC_DEFINE_UNQUOTED([STDBUF_BIN], ["$STDBUF"])],
	    [AC_MSG_ERROR([stdbuf (coreutils) is needed to run sngrep using ngrep.
If you dont have stdbuf available in your distribution, you can disable ngrep support using --disable-ngrep flag.])]
	)
	AC_PATH_PROG(NGREP, ngrep, [:], $PATH)
	AS_IF([test "x$NGREP" != "x:"],
	    [AC_DEFINE_UNQUOTED([NGREP_BIN], ["$NGREP"])],
	    [AC_MSG_ERROR([ngrep is required to run sngrep.
If you dont have ngrep available in your distribution, you can disable ngrep support using --disable-ngrep flag.])]
	)
])

######################################################################

if test "x${silent}" != "xyes" ; then
    echo
    echo '             OZZZO                 ' 
    echo '             OZZZO                 ' 
    echo '         .:  OZZZO  :.             ' 
    echo '       :DDD. OZZZO  DDD~           ' 
    echo '     :DDDDD. OZZZO  DDDDD:         ' 
    echo '    DDDDDDD. OZZZO  DDDDDDD.       ' 
    echo '  .DDDDDD    OZZZO   .DDDDDD.      ' 
    echo '  ODDDD?.    OZZZO    .~DDDDZ.     ' 
    echo '  DDDDD.     OZZZO      8DDDD      ' 
    echo ' ,DDDD.      .....       DDDD,     ' 
    echo ' ~DDDD                   DDDD+     ' 
    echo ' :DDDD.                  DDDD,     ' 
    echo '  DDDDD                 8DDDD.     ' 
    echo '  ODDDD?               ~DDDDZ      ' 
    echo '  .DDDDDD.           .DDDDDD.      ' 
    echo '   .DDDDDD8.       .8DDDDDD        ' 
    echo '    .:DDDDDDDDDDDDDDDDDDD:.        ' 
    echo '      .~DDDDDDDDDDDDDDD~.          ' 
    echo '         .:DDDDDDDDD,.             ' 
    echo
fi


AC_MSG_NOTICE
AC_MSG_NOTICE( ====================================================== )
AC_MSG_NOTICE(   sngrep configure finished                            )
AC_MSG_NOTICE(   Current Development stage: BETA                      )
AC_MSG_NOTICE(  ----------------------------------------------------  )
AC_MSG_NOTICE( ====================================================== )
AC_MSG_NOTICE


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([config/Makefile])
AC_OUTPUT
