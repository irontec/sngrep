cmake_minimum_required(VERSION 3.0)
project(sngrep
        VERSION 2.0.0
        LANGUAGES C)

set(PROJECT_NAME sngrep)
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -Werror -Wall -Wextra -pedantic")

if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_FLAGS "-std=gnu11 ${CMAKE_C_FLAGS}")
    endif ()
else ()
    set(CMAKE_C_STANDARD 11)
endif ()

include(GNUInstallDirs)

include_directories(${CMAKE_SOURCE_DIR}/src)

set(SOURCES
        src/codecs/codec.c
        src/codecs/codec_g711a.c
        src/codecs/codec_g711u.c
        src/capture/capture.c
        src/capture/capture_pcap.c
        src/capture/capture_txt.c
        src/glib-extra/gasyncqueuesource.c
        src/glib-extra/gbytes.c
        src/glib-extra/gdatetime.c
        src/glib-extra/glist.c
        src/glib-extra/gptrarray.c
        src/glib-extra/glib.c
        src/glib-extra/gnode.c
        src/glib-extra/gvalue.c
        src/glib-extra/glib_enum_types.c
        src/tui/tui.c
        src/tui/windows/call_flow_win.c
        src/tui/windows/call_list_win.c
        src/tui/windows/call_raw_win.c
        src/tui/windows/column_select_win.c
        src/tui/windows/filter_win.c
        src/tui/windows/msg_diff_win.c
        src/tui/windows/save_win.c
        src/tui/windows/settings_win.c
        src/tui/windows/stats_win.c
        src/tui/windows/auth_validate_win.c
        src/tui/windows/protocol_select_win.c
        src/tui/widgets/app_window.c
        src/tui/widgets/box.c
        src/tui/widgets/button.c
        src/tui/widgets/container.c
        src/tui/widgets/dialog.c
        src/tui/widgets/entry.c
        src/tui/widgets/label.c
        src/tui/widgets/menu_bar.c
        src/tui/widgets/menu_item.c
        src/tui/widgets/menu.c
        src/tui/widgets/radio_button.c
        src/tui/widgets/separator.c
        src/tui/widgets/scrollbar.c
        src/tui/widgets/table.c
        src/tui/widgets/widget.c
        src/tui/widgets/window.c
        src/tui/dialog.c
        src/tui/keybinding.c
        src/storage/address.c
        src/packet/dissector.c
        src/packet/dissector.h
        src/packet/packet.c
        src/packet/packet_link.c
        src/packet/packet_ip.c
        src/packet/packet_tcp.c
        src/packet/packet_udp.c
        src/packet/packet_sip.c
        src/packet/packet_sdp.c
        src/packet/packet_rtp.c
        src/packet/packet_rtcp.c
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
        src/capture/capture_input.c
        src/capture/capture_output.c
        )

######################################################################
# HEP  Support
option(USE_HEP "Enable HEP/EEP Support" OFF)
if (USE_HEP)
    set(SOURCES ${SOURCES} src/capture/capture_hep.c)
    set(SOURCES ${SOURCES} src/packet/packet_hep.c)
endif (USE_HEP)

# IPv6 Support
option(USE_IPV6 "Enable IPv6 Support" OFF)

# GnuTLS Support
option(WITH_SSL "Enable SSL Support (TLS SIP Transport)" OFF)
if (WITH_SSL)
    set(SOURCES ${SOURCES} src/packet/packet_tls.c)
endif (WITH_SSL)

# libsnd Support
option(WITH_SND "Enable libsnd files (for saving into WAV files)" OFF)

# Pulseaudio Support
option(WITH_PULSE "Enable pulseaudio support (for playing RTP streams)" OFF)
if (WITH_PULSE)
    set(SOURCES ${SOURCES} src/tui/windows/rtp_player_win.c)
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
find_program(GLIB_MKENUMS glib-mkenums HINTS ${glib_mkenums})
set(GLIB_MKENUMS_CMD)
if (NOT GLIB_MKENUMS)
    message(FATAL_ERROR "Cannot find glib-mkenums, which is required to build ${PROJECT_NAME}")
endif (NOT GLIB_MKENUMS)

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
    pkg_check_modules(PULSE REQUIRED libpulse libpulse-mainloop-glib)
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
set(ENUM_HEADERS
        src/packet/packet.h
        src/storage/storage.h
        src/tui/tui.h
        src/tui/widgets/app_window.h
        src/tui/widgets/box.h
        src/tui/windows/call_flow_win.h
        src/tui/widgets/dialog.h
        src/tui/widgets/label.h
        )

add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.h
        COMMAND ${GLIB_MKENUMS}
        ARGS
        --template ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.h.tmpl
        ${ENUM_HEADERS} > ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.h
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        DEPENDS
        ${ENUM_HEADERS}
        ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.h.tmpl
)

add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.c
        COMMAND ${GLIB_MKENUMS}
        ARGS
        --template ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.c.tmpl
        ${ENUM_HEADERS} > ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.c
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        DEPENDS
        ${ENUM_HEADERS} ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.h
        ${PROJECT_SOURCE_DIR}/src/glib-extra/glib_enum_types.c.tmpl
)

######################################################################
add_definitions(-D_GNU_SOURCE)
add_definitions(-D_XOPEN_SOURCE_EXTENDED)

######################################################################
install(TARGETS sngrep RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

######################################################################
# Tests support
enable_testing()
add_subdirectory(tests)

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
