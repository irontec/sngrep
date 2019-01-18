cmake_minimum_required(VERSION 3.0)
project(sngrep
        VERSION 2.0.0
        LANGUAGES C)

set(PROJECT_NAME sngrep)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -p -g")
add_compile_options(-Wall -pedantic -Wextra)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/ncurses)
include_directories(${CMAKE_SOURCE_DIR}/src/ncurses/windows)
include_directories(${CMAKE_SOURCE_DIR}/src/capture/)
include_directories(${CMAKE_SOURCE_DIR}/src/capture/packet)

set(SOURCES
        src/capture/address.c
        src/capture/packet.c
        src/capture/parser.c
        src/capture/packet/packet_link.c
        src/capture/packet/packet_ip.c
        src/capture/packet/packet_tcp.c
        src/capture/packet/packet_udp.c
        src/capture/packet/packet_sip.c
        src/capture/packet/packet_sdp.c
        src/capture/packet/packet_rtp.c
        src/capture/packet/packet_rtcp.c
        src/capture/capture.c
        src/capture/capture_pcap.c
        src/capture/capture_txt.c
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
        src/filter.c
        src/group.c
        src/main.c
        src/stream.c
        src/setting.c
        src/attribute.c
        src/storage.c
        src/call.c
        src/message.c
        src/timeval.c
        src/glib-extra.c
        )

######################################################################
# HEP  Support
option(USE_HEP "Enable HEP Support" ON)
if (USE_HEP)
    set(SOURCES ${SOURCES} src/capture/capture_hep.c)
    set(SOURCES ${SOURCES} src/capture/packet/packet_hep.c)
endif (USE_HEP)

# IPv6 Support
option(USE_IPV6 "Enable IPv6 Support" ON)

# GnuTLS Support
option(WITH_SSL "Enable SSL Support (TLS SIP Transport)" OFF)
if (WITH_SSL)
    set(SOURCES ${SOURCES} src/capture/packet/packet_tls.c)
endif (WITH_SSL)

# libsnd Support
option(WITH_SND "Enable libsnd files (for saving into WAV files)" OFF)
if (WITH_SND)
    set(SOURCES ${SOURCES} src/capture/codecs/codec_g711a.c)
endif (WITH_SND)

######################################################################
add_executable(sngrep ${SOURCES})
target_link_libraries(sngrep pcap)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURSES REQUIRED ncursesw menuw panelw formw)
include_directories(${CURSES_INCLUDE_DIRS})
target_link_libraries(sngrep ${CURSES_LIBRARIES})

pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
target_link_libraries(sngrep ${GLIB_LIBRARIES})

find_program(PCAP_CONFIG_EXECUTABLE NAMES pcap-config)
if (PCAP_CONFIG_EXECUTABLE)
    exec_program(${PCAP_CONFIG_EXECUTABLE} ARGS --cflags OUTPUT_VARIABLE PCAP_CFLAGS )
    exec_program(${PCAP_CONFIG_EXECUTABLE} ARGS --libs   OUTPUT_VARIABLE PCAP_LIBRARIES )
else (PCAP_CONFIG_EXECUTABLE)
    pkg_check_modules(PCAP REQUIRED libpcap)
endif (PCAP_CONFIG_EXECUTABLE)
include_directories(${PCAP_INCLUDE_DIRS})
target_link_libraries(sngrep ${PCAP_LIBRARIES})

if (WITH_SSL)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GNUTLS REQUIRED gnutls)
    include_directories(${GNUTLS_INCLUDE_DIRS})
    target_link_libraries(sngrep ${GNUTLS_LIBRARIES} gcrypt)
endif (WITH_SSL)

if (WITH_SND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SND REQUIRED sndfile)
    include_directories(${SND_INCLUDE_DIRS})
    target_link_libraries(sngrep ${SND_LIBRARIES})
endif (WITH_SND)

configure_file(
        ${PROJECT_SOURCE_DIR}/src/config.h.cmake
        ${PROJECT_SOURCE_DIR}/src/config.h
)

######################################################################
add_definitions(-D_GNU_SOURCE)
add_definitions(-D_XOPEN_SOURCE_EXTENDED)

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
message(" SND Support                  : ${WITH_SND}                ")
message(" SSL Support                  : ${WITH_SSL}	   	        ")
message(" HEP Support                  : ${USE_HEP}                 ")
message(" IPv6 Support                 : ${USE_IPV6}                ")
message(" ======================================================    ")
message(" ")
