# CMakeLists file derived from configure.ac

cmake_minimum_required( VERSION 3.7...3.27 )

option( WITH_GNUTLS    "Enable SSL Support (TLS SIP Transport)"            no )
option( WITH_OPENSSL   "Enable SSL Support (TLS SIP Transport)"            no )
option( WITH_PCRE      "Enable Perl compatible regular expressions"        no )
option( WITH_PCRE2     "Enable Perl compatible regular expressions (v2)"   no )
option( WITH_ZLIB      "Enable zlib to support gzip compressed pcap files" no )
option( WITH_UNICODE   "Enable Ncurses Unicode support"                    no )
option( USE_IPV6       "Enable IPv6 Support"                               no )
option( USE_EEP        "Enable EEP/HEP Support"                            no )
option( DISABLE_LOGO   "Disable Irontec Logo from Summary menu"            no )

# Read parameters of AC_INIT() from file configure.ac
file( STRINGS configure.ac AC_INIT REGEX "AC_INIT\\(.*\\)" )
if( AC_INIT MATCHES "^.*AC_INIT\\( *\\[([^]]*)\\], *\\[([^]]*)\\], *\\[([^]]*)\\], *\\[([^]]*)\\], *\\[([^]]*)\\] *\\).*$" )
	set( AC_PACKAGE_NAME "${CMAKE_MATCH_1}" )
	set( AC_PACKAGE_VERSION "${CMAKE_MATCH_2}" )
	set( AC_PACKAGE_STRING "${CMAKE_MATCH_1} ${CMAKE_MATCH_2}" )
	set( AC_PACKAGE_BUGREPORT "${CMAKE_MATCH_3}" )
	set( AC_PACKAGE_TARNAME "${CMAKE_MATCH_4}" )
	set( AC_PACKAGE_URL "${CMAKE_MATCH_5}" )
	message( STATUS "${AC_PACKAGE_STRING}" )
else()
	message( FATAL_ERROR "Error parsing AC_INIT(...) from file configure.ac" )
endif()

project( ${AC_PACKAGE_NAME}
	VERSION ${AC_PACKAGE_VERSION}
	LANGUAGES C
	)

add_executable( sngrep src/main.c )

set_target_properties( sngrep PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS YES )
target_compile_options( sngrep PRIVATE -Wall -pedantic ) # -Wextra -Wconversion -Werror

# Define _GNU_SOURCE etc.
if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
	target_compile_definitions( sngrep PRIVATE _GNU_SOURCE=1 _XOPEN_SOURCE_EXTENDED=1 )
endif()

# we might want to use this with zlib for compressed pcap support
include( CheckFunctionExists )
check_function_exists( fopencookie HAVE_FOPENCOOKIE )

#######################################################################
# Check for other REQUIRED libraries

find_package( PkgConfig REQUIRED )

message( STATUS "Checking for library 'libpthread'" )
find_library( LIBPTHREAD pthread ) # Note: The use of "REQUIRED" here would need CMake 3.18...
if( LIBPTHREAD )                   # ...therefore we check the result for ourself
	message( STATUS "  Found ${LIBPTHREAD}" )
	target_link_libraries( sngrep PUBLIC pthread )
else()
	message( STATUS "  No library 'libpthread' found" )
	message( FATAL_ERROR "You need to have libpthread installed to compile sngrep." )
endif()

pkg_check_modules( LIBPCAP IMPORTED_TARGET libpcap )
if( LIBPCAP_FOUND )
	target_link_libraries( sngrep PUBLIC PkgConfig::LIBPCAP )
else()
	message( STATUS "Checking for library 'libpcap'" )
	find_library( LIBPCAP pcap )
	if( LIBPCAP )
		message( STATUS "  Found ${LIBPCAP}" )
		target_link_libraries( sngrep PUBLIC pcap )
	else()
		message( STATUS "  No library 'libpcap' found" )
		message( FATAL_ERROR "You need to have libpcap installed to compile sngrep." )
	endif()
endif()

####
#### Ncurses Wide character support
####
if( WITH_UNICODE )
	pkg_check_modules( NCURSES REQUIRED IMPORTED_TARGET ncursesw )
	pkg_check_modules( PANEL REQUIRED IMPORTED_TARGET panelw )
	pkg_check_modules( FORM REQUIRED IMPORTED_TARGET formw )
	pkg_check_modules( MENU REQUIRED IMPORTED_TARGET menuw )
else()
	pkg_check_modules( NCURSES REQUIRED IMPORTED_TARGET ncurses )
	pkg_check_modules( PANEL REQUIRED IMPORTED_TARGET panel )
	pkg_check_modules( FORM REQUIRED IMPORTED_TARGET form )
	pkg_check_modules( MENU REQUIRED IMPORTED_TARGET menu )
endif()
target_link_libraries( sngrep PUBLIC PkgConfig::NCURSES PkgConfig::PANEL PkgConfig::FORM PkgConfig::MENU )

####
#### GnuTLS Support
####
if( WITH_GNUTLS )
	pkg_check_modules( GNUTLS REQUIRED IMPORTED_TARGET gnutls )
	message( STATUS "Checking for library 'libgcrypt'" )
	find_library( LIBGCRYPT gcrypt ) # Note: The use of "REQUIRED" here would need CMake 3.18...
	if( LIBGCRYPT )                  # ...therefore we check the result for ourself
		message( STATUS "  Found ${LIBGCRYPT}" )
	else()
		message( STATUS "  No library 'libgcrypt' found" )
		message( FATAL_ERROR "You need to have libgcrypt installed to compile sngrep" )
	endif()
	target_link_libraries( sngrep PUBLIC pthread PkgConfig::GNUTLS gcrypt )
endif()

####
#### OpenSSL Support
####
if( WITH_OPENSSL )
	if( WITH_GNUTLS )
		message( FATAL_ERROR "GnuTLS and OpenSSL can not be enabled at the same time" )
	endif()

	pkg_check_modules( LIBSSL REQUIRED IMPORTED_TARGET libssl )
	pkg_check_modules( LIBCRYPTO REQUIRED IMPORTED_TARGET libcrypto )
	target_link_libraries( sngrep PUBLIC pthread PkgConfig::LIBSSL PkgConfig::LIBCRYPTO )
endif()

####
#### PCRE Support
####
if( WITH_PCRE )
	pkg_check_modules( LIBPCRE REQUIRED IMPORTED_TARGET libpcre )
	target_link_libraries( sngrep PUBLIC pthread PkgConfig::LIBPCRE )
endif()

####
#### PCRE2 Support
####
if( WITH_PCRE2 )
	if( WITH_PCRE )
		message( FATAL_ERROR "libpcre-2 and libpcre-3 can not be enabled at the same time" )
	endif()

	target_compile_definitions( sngrep PRIVATE PCRE2_CODE_UNIT_WIDTH=8 ) # Required for including pcre2.h

	pkg_check_modules( LIBPCRE2 REQUIRED IMPORTED_TARGET libpcre2-8 )
	target_link_libraries( sngrep PUBLIC pthread PkgConfig::LIBPCRE2 )
endif()

####
#### zlib Support
####
if ( WITH_ZLIB )
	pkg_check_modules( ZLIB REQUIRED IMPORTED_TARGET zlib )
	target_link_libraries( sngrep PUBLIC pthread PkgConfig::ZLIB )
endif()

# Source inclusion
target_sources( sngrep
	PRIVATE
		src/address.c
		src/packet.c
		src/sip.c
		src/sip_call.c
		src/sip_msg.c
		src/sip_attr.c
		src/option.c
		src/group.c
		src/filter.c
		src/keybinding.c
		src/media.c
		src/setting.c
		src/rtp.c
		src/util.c
		src/hash.c
		src/vector.c
	#
		src/curses/ui_panel.c
		src/curses/scrollbar.c
		src/curses/ui_manager.c
		src/curses/ui_call_list.c
		src/curses/ui_call_flow.c
		src/curses/ui_call_raw.c
		src/curses/ui_stats.c
		src/curses/ui_filter.c
		src/curses/ui_save.c
		src/curses/ui_msg_diff.c
		src/curses/ui_column_select.c
		src/curses/ui_settings.c
	)

target_include_directories( sngrep PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src )

# Conditional Source inclusion
target_sources( sngrep PRIVATE src/capture.c )
if( WITH_GNUTLS )
	target_sources( sngrep PRIVATE src/capture_gnutls.c )
endif()
if( WITH_OPENSSL )
	target_sources( sngrep PRIVATE src/capture_openssl.c )
endif()
if( USE_EEP )
	target_sources( sngrep PRIVATE src/capture_eep.c )
endif()

######################################################################
# Generate config.h

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
target_include_directories( sngrep PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )

######################################################################
# Installing

include( GNUInstallDirs )

install( TARGETS sngrep RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
install( FILES config/sngreprc DESTINATION /etc )
install( FILES doc/sngrep.8 DESTINATION ${CMAKE_INSTALL_MANDIR}/man8 )
install( FILES README TODO COPYING ChangeLog DESTINATION "${CMAKE_INSTALL_DOCDIR}" )

######################################################################
# Packaging

set( CPACK_PACKAGE_NAME "${AC_PACKAGE_NAME}" )
set( CPACK_PACKAGE_VERSION "${AC_PACKAGE_VERSION}" )
set( CPACK_PACKAGE_CONTACT "Ivan Alonso <kaian@irontec.com>" ) # ${AC_PACKAGE_BUGREPORT} only contains the E-Mail address, therefore we don't use it here
set( CPACK_PACKAGE_HOMEPAGE_URL "${AC_PACKAGE_URL}" )
set( CPACK_PACKAGE_VENDOR "Irontec S.L." )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" )
set( CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README" )
set( CPACK_STRIP_FILES TRUE )

set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ncurses SIP Messages flow viewer" )

set( CPACK_PACKAGE_DESCRIPTION
"sngrep displays SIP Messages grouped by Call-Id into flow
diagrams. It can be used as an offline PCAP viewer or online
capture using libpcap functions.

It supports SIP UDP, TCP and TLS transports (when each message is
delivered in one packet).

You can also create new PCAP files from captures or displayed dialogs." )

set( CPACK_DEBIAN_FILE_NAME DEB-DEFAULT )
#set( CPACK_DEBIAN_PACKAGE_RELEASE 1 )
set( CPACK_DEBIAN_PACKAGE_SECTION "comm" )
set( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )

set( CPACK_RPM_FILE_NAME RPM-DEFAULT )
#set( CPACK_RPM_PACKAGE_RELEASE 1 )
set( CPACK_RPM_PACKAGE_RELEASE_DIST ON )
set( CPACK_RPM_PACKAGE_GROUP "Applications/Engineering" )
set( CPACK_RPM_PACKAGE_LICENSE "GPLv3" )
set( CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}" )
set( CPACK_RPM_PACKAGE_AUTOREQ YES )

include( CPack )

######################################################################
# Print Logo

if( NOT DISABLE_LOGO )
	message( STATUS "" )
	message( STATUS " ██╗██████╗  ██████╗ ███╗   ██╗████████╗███████╗ ██████╗" )
	message( STATUS " ██║██╔══██╗██╔═══██╗████╗  ██║╚══██╔══╝██╔════╝██╔════╝" )
	message( STATUS " ██║██████╔╝██║   ██║██╔██╗ ██║   ██║   █████╗  ██║     " )
	message( STATUS " ██║██╔══██╗██║   ██║██║╚██╗██║   ██║   ██╔══╝  ██║     " )
	message( STATUS " ██║██║  ██║╚██████╔╝██║ ╚████║   ██║   ███████╗╚██████╗" )
	message( STATUS " ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝   ╚═╝   ╚══════╝ ╚═════╝" )
endif()

message( STATUS "" )
message( STATUS "sngrep configure finished"                              )
message( STATUS "======================================================" )
message( STATUS "GnuTLS Support               : ${WITH_GNUTLS}"          )
message( STATUS "OpenSSL Support              : ${WITH_OPENSSL}"         )
message( STATUS "Unicode Support              : ${WITH_UNICODE}"         )
message( STATUS "Perl Expressions Support     : ${WITH_PCRE}"            )
message( STATUS "Perl Expressions Support (v2): ${WITH_PCRE2}"           )
message( STATUS "IPv6 Support                 : ${USE_IPV6}"             )
message( STATUS "EEP Support                  : ${USE_EEP}"              )
message( STATUS "Zlib Support                 : ${WITH_ZLIB}"            )
message( STATUS "======================================================" )
message( STATUS "" )

######################################################################
# Tests

enable_testing()            # "ctest" will run all tests
add_custom_target( tests )  # "make tests" will build all tests

foreach( i 001 002 003 004 005 006 007 008 009 010 011 )
	add_executable( test_${i} EXCLUDE_FROM_ALL tests/test_${i}.c )
	if( i STREQUAL "007" )
		target_sources( test_${i} PUBLIC src/vector.c src/util.c )
	elseif( i STREQUAL "010" )
		target_sources( test_${i} PUBLIC src/hash.c )
	endif()
	target_include_directories( test_${i} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )

	add_test( NAME test_${i} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_${i} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests )
	add_dependencies( tests test_${i} )
endforeach()

