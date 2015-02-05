AC_PREREQ([2.59])
AC_INIT([sngrep], [0.2.2], [kaian@irontec.com], [sngrep], [http://www.irontec.com/])
AM_INIT_AUTOMAKE([1.10])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([src/config.h])

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

AC_CHECK_LIB([menu], [new_item], [], [
    AC_MSG_ERROR([ You need to have ncurses menu library installed to compile sngrep.])
])

AC_CHECK_LIB([pthread], [pthread_create], [], [
    AC_MSG_ERROR([ You need to have libpthread installed to compile sngrep.])
])

AC_CHECK_LIB([pcap], [pcap_open_offline], [], [
    AC_MSG_ERROR([ You need to have libpcap installed to compile sngrep.])
])
AC_CHECK_HEADER([pcap.h], [], [
    AC_MSG_ERROR([ You need to have libpcap development files installed to compile sngrep.])
])

# Check for SSL dependencies
AC_ARG_ENABLE([openssl],
    AC_HELP_STRING([--disable-openssl], [Disable TLS SIP Transport]))
    
AS_IF([test "x$enable_openssl" != "xno"], [

	AC_CHECK_LIB([ssl], [SSL_new], [], [
	    AC_MSG_ERROR([ You need to have libssl installed to compile sngrep (or use --disable-openssl)])
	])
	
	AC_CHECK_LIB([crypto], [EVP_get_cipherbyname], [], [
	    AC_MSG_ERROR([ You need to have libcrypto installed to compile sngrep (or use --disable-openssl)])
	])
	AC_DEFINE([WITH_OPENSSL],[],[Compile With Openssl compatibility])
], [])

# Conditional Source inclusion 
AM_CONDITIONAL([WITH_OPENSSL], [test "x$enable_openssl" != "xno"])


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
AC_MSG_NOTICE(  ----------------------------------------------------  )
AC_MSG_NOTICE( ====================================================== )
AC_MSG_NOTICE


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([config/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_OUTPUT
