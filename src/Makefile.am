bin_PROGRAMS=sngrep
sngrep_SOURCES=capture.c capture_ws.c sip.c sip_attr.c main.c option.c group.c filter.c keybinding.c
sngrep_SOURCES+=ui_manager.c ui_call_list.c ui_call_flow.c ui_call_raw.c 
sngrep_SOURCES+=ui_filter.c ui_save_pcap.c ui_save_raw.c ui_msg_diff.c ui_column_select.c

if WITH_OPENSSL
sngrep_SOURCES+=capture_tls.c 
endif
