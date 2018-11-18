cmake_minimum_required(VERSION 3.0)
project(sngrep
        VERSION 2.0.0
        LANGUAGES C)

set(PROJECT_NAME sngrep)
set(CMAKE_C_STANDARD 11)
#add_compile_options(-Werror -g)
add_compile_options(-Werror -g -Wall -pedantic -Wextra)

configure_file(
    ${PROJECT_SOURCE_DIR}/src/config.h.cmake.in
    ${PROJECT_SOURCE_DIR}/src/config.h
)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/curses)
include_directories(${CMAKE_SOURCE_DIR}/src/curses/widgets)
include_directories(${CMAKE_SOURCE_DIR}/src/curses/screens)

set(SOURCES
        src/packet/address.c
        src/packet/packet.c
        src/packet/parser.c
        src/packet/dissectors/packet_link.c
        src/packet/dissectors/packet_ip.c
        src/packet/dissectors/packet_tcp.c
        src/packet/dissectors/packet_udp.c
        src/packet/dissectors/packet_sip.c
        src/packet/dissectors/packet_sdp.c
        src/packet/dissectors/packet_rtp.c
        src/packet/dissectors/packet_rtcp.c
        src/capture/capture.c
        src/capture/capture_pcap.c
        src/capture/capture_txt.c
        src/ncurses/ui_manager.c
        src/ncurses/ui_call_flow.c
        src/ncurses/call_list.c
        src/ncurses/ui_call_raw.c
        src/ncurses/column_select.c
        src/ncurses/ui_filter.c
        src/ncurses/ui_msg_diff.c
        src/ncurses/ui_save.c
        src/ncurses/ui_settings.c
        src/ncurses/ui_stats.c
        src/ncurses/ui_panel.c
        src/ncurses/keybinding.c
        src/ncurses/scrollbar.c
        src/filter.c
        src/group.c
        src/main.c
        src/option.c
        src/stream.c
        src/setting.c
        src/sip_attr.c
        src/storage.c
        src/sip_call.c
        src/sip_msg.c
        src/timeval.c
        src/glib-utils.c
)

######################################################################
# HEP  Support
option (USE_HEP "Enable EEP/HEP Support" OFF)
if (USE_HEP)
    set(SOURCES ${SOURCES} src/capture/capture_hep.c)
endif (USE_HEP)

# IPv6 Support
option (USE_IPV6 "Enable IPv6 Support" ON)

# GnuTLS Support
option (WITH_SSL "Enable SSL Support (TLS SIP Transport)" OFF)
if (WITH_SSL)
    set(SOURCES ${SOURCES} src/packet/dissectors/packet_tls.c)
endif(WITH_SSL)

######################################################################
add_executable(sngrep ${SOURCES})
target_link_libraries(sngrep pcap)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURSES REQUIRED ncursesw menuw panelw formw)
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(sngrep ${CURSES_LIBRARIES} )

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
target_link_libraries(sngrep ${GLIB_LIBRARIES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)
include_directories(${GOBJECT_INCLUDE_DIRS})
target_link_libraries(sngrep ${GOBJECT_LIBRARIES})

find_package(PkgConfig REQUIRED)
find_package (Threads)
target_link_libraries (sngrep ${CMAKE_THREAD_LIBS_INIT})

if (WITH_SSL)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GNUTLS REQUIRED gnutls)
    include_directories(${GNUTLS_INCLUDE_DIRS})
    target_link_libraries(sngrep ${GNUTLS_LIBRARIES} gcrypt)
endif(WITH_SSL)


######################################################################
add_definitions(-D_GNU_SOURCE)
add_definitions(-D_XOPEN_SOURCE_EXTENDED)

######################################################################
ADD_CUSTOM_COMMAND(OUTPUT src/ncurses/gnt/gntmarshal.h src/ncurses/gnt/gntmarshal.c
        WORKING_DIRECTORY src/ncurses/gnt/
        COMMAND glib-genmarshal --body --prefix=gnt_closure_marshal genmarshal > gntmarshal.c
        COMMAND glib-genmarshal --header --prefix=gnt_closure_marshal genmarshal > gntmarshal.h
        DEPENDS "src/ncurses/widgets/genmarshal"
        VERBATIM
        COMMENT "Generating GObject Marshal code...")

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
message(" SSL Support                  : ${WITH_SSL}	   	        ")
message(" HEP Support                  : ${USE_HEP}                 ")
message(" IPv6 Support                 : ${USE_IPV6}                ")
message(" ======================================================    ")
message(" ")
