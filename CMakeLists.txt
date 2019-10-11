cmake_minimum_required(VERSION 3.0)
project(sngrep
        VERSION 2.0.0
        LANGUAGES C)

set(PROJECT_NAME sngrep)
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -pg -Werror -Wall -Wextra -pedantic")
#set(CMAKE_VERBOSE_MAKEFILE ON)

if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_FLAGS "-std=gnu11 ${CMAKE_C_FLAGS}")
    endif ()
else ()
    set(CMAKE_C_STANDARD 11)
endif ()

include(GNUInstallDirs)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/glib)
include_directories(${CMAKE_SOURCE_DIR}/src/ncurses)
include_directories(${CMAKE_SOURCE_DIR}/src/ncurses/windows)
include_directories(${CMAKE_SOURCE_DIR}/src/capture/)
include_directories(${CMAKE_SOURCE_DIR}/src/capture/dissectors)
include_directories(${CMAKE_SOURCE_DIR}/src/capture/codecs)

set(SOURCES
        src/codecs/codec_g711a.c
        src/capture/capture.c
        src/capture/capture_pcap.c
        src/capture/capture_txt.c
        src/glib/gasyncqueuesource.c
        src/glib/glib-extra.c
        src/ncurses/manager.c
        src/ncurses/windows/call_flow_win.c
        src/ncurses/windows/call_list_win.c
        src/ncurses/windows/call_raw_win.c
        src/ncurses/windows/column_select_win.c
        src/ncurses/windows/filter_win.c
        src/ncurses/windows/msg_diff_win.c
        src/ncurses/windows/save_win.c
        src/ncurses/windows/settings_win.c
        src/ncurses/windows/stats_win.c
        src/ncurses/windows/auth_validate_win.c
        src/ncurses/window.c
        src/ncurses/dialog.c
        src/ncurses/keybinding.c
        src/ncurses/scrollbar.c
        src/parser/address.c
        src/parser/packet.c
        src/parser/parser.c
        src/parser/packet_link.c
        src/parser/packet_ip.c
        src/parser/packet_tcp.c
        src/parser/packet_udp.c
        src/parser/packet_sip.c
        src/parser/packet_sdp.c
        src/parser/packet_rtp.c
        src/parser/packet_rtcp.c
        src/storage/filter.c
        src/storage/group.c
        src/storage/stream.c
        src/storage/attribute.c
        src/storage/storage.c
        src/storage/call.c
        src/storage/message.c
        src/storage/datetime.c
        src/setting.c
        src/main.c
        )

######################################################################
# HEP  Support
option(USE_HEP "Enable HEP/EEP Support" OFF)
if (USE_HEP)
    set(SOURCES ${SOURCES} src/capture/capture_hep.c)
    set(SOURCES ${SOURCES} src/parser/packet_hep.c)
endif (USE_HEP)

# IPv6 Support
option(USE_IPV6 "Enable IPv6 Support" OFF)

# GnuTLS Support
option(WITH_SSL "Enable SSL Support (TLS SIP Transport)" OFF)
if (WITH_SSL)
    set(SOURCES ${SOURCES} src/parser/packet_tls.c)
endif (WITH_SSL)

# libsnd Support
option(WITH_SND "Enable libsnd files (for saving into WAV files)" OFF)

# Pulseaudio Support
option(WITH_PULSE "Enable pulseaudio support (for playing RTP streams)" OFF)
if (WITH_PULSE)
    set(SOURCES ${SOURCES} src/ncurses/windows/rtp_player_win.c)
endif (WITH_PULSE)

# G.729 decoding support
option(WITH_G729 "Enable g729 RTP decoding" OFF)
if (WITH_G729)
    set(SOURCES ${SOURCES} src/codecs/codec_g729.c)
endif (WITH_G729)

######################################################################
add_executable(sngrep ${SOURCES})
install(
        TARGETS sngrep
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURSES REQUIRED ncursesw menuw panelw formw)
include_directories(${CURSES_INCLUDE_DIRS})
target_link_libraries(sngrep ${CURSES_LIBRARIES})

pkg_check_modules(GLIB REQUIRED glib-2.0>=2.44 gobject-2.0>=2.44)
include_directories(${GLIB_INCLUDE_DIRS})
target_link_libraries(sngrep ${GLIB_LIBRARIES})

find_program(PCAP_CONFIG_EXECUTABLE NAMES pcap-config)
if (PCAP_CONFIG_EXECUTABLE)
    exec_program(${PCAP_CONFIG_EXECUTABLE} ARGS --cflags OUTPUT_VARIABLE PCAP_CFLAGS)
    exec_program(${PCAP_CONFIG_EXECUTABLE} ARGS --libs OUTPUT_VARIABLE PCAP_LIBRARIES)
else (PCAP_CONFIG_EXECUTABLE)
    pkg_check_modules(PCAP REQUIRED libpcap)
endif (PCAP_CONFIG_EXECUTABLE)
include_directories(${PCAP_INCLUDE_DIRS})
target_link_libraries(sngrep ${PCAP_LIBRARIES})

if (WITH_SSL)
    pkg_check_modules(GNUTLS REQUIRED gnutls)
    include_directories(${GNUTLS_INCLUDE_DIRS})
    target_link_libraries(sngrep ${GNUTLS_LIBRARIES} gcrypt)

    # Use libgcrypt-config to get configure flags
    find_program(GCRYPT_CONFIG_EXECUTABLE NAMES libgcrypt-config)
    if (GCRYPT_CONFIG_EXECUTABLE)
        exec_program(${GCRYPT_CONFIG_EXECUTABLE} ARGS --cflags OUTPUT_VARIABLE GCRYPT_CFLAGS)
        exec_program(${GCRYPT_CONFIG_EXECUTABLE} ARGS --libs OUTPUT_VARIABLE GCRYPT_LIBRARIES)
    endif (GCRYPT_CONFIG_EXECUTABLE)
    include_directories(${GCRYPT_INCLUDE_DIRS})
    target_link_libraries(sngrep ${GCRYPT_LIBRARIES})
endif (WITH_SSL)

if (WITH_SND)
    pkg_check_modules(SND REQUIRED sndfile)
    include_directories(${SND_INCLUDE_DIRS})
    target_link_libraries(sngrep ${SND_LIBRARIES})
endif (WITH_SND)

if (WITH_PULSE)
    pkg_check_modules(PULSE REQUIRED libpulse)
    include_directories(${PULSE_INCLUDE_DIRS})
    target_link_libraries(sngrep ${PULSE_LIBRARIES})
endif (WITH_PULSE)


if (WITH_G729)
    pkg_check_modules(BCG729 REQUIRED libbcg729>=1.0.4)
    include_directories(${BCG729_INCLUDE_DIRS})
    target_link_libraries(sngrep ${BCG729_LIBRARIES})
endif (WITH_G729)

configure_file(
        ${PROJECT_SOURCE_DIR}/src/config.h.cmake
        ${PROJECT_SOURCE_DIR}/src/config.h
)

######################################################################
add_definitions(-D_GNU_SOURCE)
add_definitions(-D_XOPEN_SOURCE_EXTENDED)

######################################################################
install(TARGETS sngrep RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

######################################################################
# Print Logo
option(DISPLAY_LOGO "Disable Irontec Logo from Summary menu" ON)

if (DISPLAY_LOGO)
    message(" ")
    message("  ██╗██████╗  ██████╗ ███╗   ██╗████████╗███████╗ ██████╗  ")
    message("  ██║██╔══██╗██╔═══██╗████╗  ██║╚══██╔══╝██╔════╝██╔════╝  ")
    message("  ██║██████╔╝██║   ██║██╔██╗ ██║   ██║   █████╗  ██║       ")
    message("  ██║██╔══██╗██║   ██║██║╚██╗██║   ██║   ██╔══╝  ██║       ")
    message("  ██║██║  ██║╚██████╔╝██║ ╚████║   ██║   ███████╗╚██████╗  ")
    message("  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝   ╚═╝   ╚══════╝ ╚═════╝  ")
    message(" ")
endif (DISPLAY_LOGO)

message(" ")
message("   sngrep configure finished                               ")
message(" ======================================================    ")
message(" SND Support (WITH_SND)          : ${WITH_SND}             ")
message(" SSL Support (WITH_SSL)          : ${WITH_SSL}	   	        ")
message(" Pulseaudio Support (WITH_PULSE) : ${WITH_PULSE}           ")
message(" g729 Support (WITH_G729)        : ${WITH_G729}            ")
message(" HEP Support (USE_HEP)           : ${USE_HEP}              ")
message(" IPv6 Support (USE_IPV6)         : ${USE_IPV6}             ")
message(" ======================================================    ")
message(" ")
