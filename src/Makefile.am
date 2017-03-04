AUTOMAKE_OPTIONS=subdir-objects
bin_PROGRAMS=sngrep
sngrep_SOURCES=capture.c
if USE_EEP
sngrep_SOURCES+=capture_eep.c
endif
if WITH_GNUTLS
sngrep_SOURCES+=capture_gnutls.c
endif
if WITH_OPENSSL
sngrep_SOURCES+=capture_openssl.c
endif
sngrep_SOURCES+=address.c packet.c sip.c sip_call.c sip_msg.c sip_attr.c main.c
sngrep_SOURCES+=option.c group.c filter.c keybinding.c media.c setting.c rtp.c
sngrep_SOURCES+=util.c hash.c vector.c curses/ui_panel.c curses/scrollbar.c
sngrep_SOURCES+=curses/ui_manager.c curses/ui_call_list.c curses/ui_call_flow.c curses/ui_call_raw.c
sngrep_SOURCES+=curses/ui_stats.c curses/ui_filter.c curses/ui_save.c curses/ui_msg_diff.c
sngrep_SOURCES+=curses/ui_column_select.c curses/ui_settings.c

