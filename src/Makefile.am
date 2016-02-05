bin_PROGRAMS=sngrep
VPATH=curses
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
sngrep_SOURCES+=util.c vector.c ui_panel.c scrollbar.c
sngrep_SOURCES+=ui_manager.c ui_call_list.c ui_call_flow.c ui_call_raw.c
sngrep_SOURCES+=ui_stats.c ui_filter.c ui_save.c ui_msg_diff.c
sngrep_SOURCES+=ui_column_select.c ui_settings.c

