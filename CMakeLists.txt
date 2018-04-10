cmake_minimum_required(VERSION 3.0)
project(sngrep
        VERSION 2.0.0
        LANGUAGES C)

set(PROJECT_NAME sngrep)
set(CMAKE_C_STANDARD 99)
add_compile_options(-Werror)

configure_file(
    ${PROJECT_SOURCE_DIR}/src/config.h.cmake
    ${PROJECT_SOURCE_DIR}/src/config.h
)

include_directories(${CMAKE_SOURCE_DIR}/src)

set(SOURCES
    src/address.c
    src/capture.c
    src/curses/scrollbar.c
    src/curses/ui_call_flow.c
    src/curses/ui_call_list.c
    src/curses/ui_call_raw.c
    src/curses/ui_column_select.c
    src/curses/ui_filter.c
    src/curses/ui_manager.c
    src/curses/ui_msg_diff.c
    src/curses/ui_panel.c
    src/curses/ui_save.c
    src/curses/ui_settings.c
    src/curses/ui_stats.c
    src/filter.c
    src/group.c
    src/keybinding.c
    src/main.c
    src/media.c
    src/option.c
    src/packet.c
    src/rtp.c
    src/setting.c
    src/sip_attr.c
    src/sip.c
    src/sip_call.c
    src/sip_msg.c
    src/util.c
    src/glib-utils.c
)

######################################################################
# HEP  Support
option (USE_EEP "Enable EEP/HEP Support" ON)
if (USE_EEP)
    set(SOURCES ${SOURCES} src/capture_eep.c)
endif (USE_EEP)

# IPv6 Support
option (USE_IPV6 "Enable IPv6 Support" ON)

# GnuTLS Support
option (WITH_GNUTLS "Enable SSL Support (TLS SIP Transport)" ON)
if (WITH_GNUTLS)
    set(SOURCES ${SOURCES} src/capture_gnutls.c)
endif(WITH_GNUTLS)

######################################################################
add_executable(sngrep ${SOURCES})
target_link_libraries(sngrep pcap)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURSES REQUIRED ncursesw)
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(sngrep ${CURSES_LIBRARIES} menu panel form)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
target_link_libraries(sngrep ${GLIB_LIBRARIES})

find_package(PkgConfig REQUIRED)
find_package (Threads)
target_link_libraries (sngrep ${CMAKE_THREAD_LIBS_INIT})

if (WITH_GNUTLS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GNUTLS REQUIRED gnutls)
    include_directories(${GNUTLS_INCLUDE_DIRS})
    target_link_libraries(sngrep ${GNUTLS_LIBRARIES} gcrypt)
endif(WITH_GNUTLS)


######################################################################
add_definitions(-D_GNU_SOURCE)

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
message("   sngrep configure finished                            	")
message(" ====================================================== 	")
message(" SSL Support                  : ${WITH_GNUTLS}	   	            ")
message(" IPv6 Support                 : ${USE_IPV6}                ")
message(" EEP Support                  : ${USE_EEP}                 ")
message(" ====================================================== 	")
message(" ")
