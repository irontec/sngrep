AUTOMAKE_OPTIONS=subdir-objects
bin_PROGRAMS=sngrep

sngrep_CFLAGS=$(GLIB_CFLAGS)
sngrep_LDADD=$(GLIB_LIBS)
sngrep_SOURCES=main.c

if USE_HEP
sngrep_SOURCES+=capture/capture_hep.c
endif

if WITH_GNUTLS
sngrep_SOURCES+=packet/dissector/packet_tls.c
sngrep_CFLAGS+=$(LIBGNUTLS_CFLAGS) $(LIBGCRYPT_CFLAGS)
sngrep_LDADD+=$(LIBGNUTLS_LIBS) $(LIBGCRYPT_LIBS)
endif

sngrep_SOURCES+=packet/address.c
sngrep_SOURCES+=packet/packet.c
sngrep_SOURCES+=packet/dissectors/packet_rtcp.c
sngrep_SOURCES+=packet/dissectors/packet_ip.c
sngrep_SOURCES+=packet/dissectors/packet_sdp.c
sngrep_SOURCES+=packet/dissectors/packet_link.c
sngrep_SOURCES+=packet/dissectors/packet_ws.c
sngrep_SOURCES+=packet/dissectors/packet_rtp.c
sngrep_SOURCES+=packet/dissectors/packet_tcp.c
sngrep_SOURCES+=packet/dissectors/packet_sip.c
sngrep_SOURCES+=packet/dissectors/packet_udp.c
sngrep_SOURCES+=packet/parser.c
sngrep_SOURCES+=packet/dissector.c
sngrep_SOURCES+=sip_attr.c
sngrep_SOURCES+=group.c
sngrep_SOURCES+=stream.c
sngrep_SOURCES+=sip_call.c
sngrep_SOURCES+=sip_msg.c
sngrep_SOURCES+=timeval.c
sngrep_SOURCES+=setting.c
sngrep_SOURCES+=glib-utils.c
sngrep_SOURCES+=storage.c
sngrep_SOURCES+=ncurses/keybinding.c
sngrep_SOURCES+=ncurses/scrollbar.c
sngrep_SOURCES+=ncurses/ui_manager.c
sngrep_SOURCES+=ncurses/ui_stats.c
sngrep_SOURCES+=ncurses/ui_filter.c
sngrep_SOURCES+=ncurses/ui_panel.c
sngrep_SOURCES+=ncurses/ui_settings.c
sngrep_SOURCES+=ncurses/ui_call_list.c
sngrep_SOURCES+=ncurses/ui_save.c
sngrep_SOURCES+=ncurses/ui_call_raw.c
sngrep_SOURCES+=ncurses/ui_column_select.c
sngrep_SOURCES+=ncurses/ui_call_flow.c
sngrep_SOURCES+=ncurses/ui_msg_diff.c
sngrep_SOURCES+=capture/capture.c
sngrep_SOURCES+=capture/capture_txt.c
sngrep_SOURCES+=capture/capture_pcap.c
sngrep_SOURCES+=filter.c
sngrep_SOURCES+=option.c
