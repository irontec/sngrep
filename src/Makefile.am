AUTOMAKE_OPTIONS=subdir-objects
bin_PROGRAMS=sngrep
sngrep_SOURCES=capture.c
sngrep_CFLAGS=
sngrep_LDADD=
if USE_EEP
sngrep_SOURCES+=capture_eep.c
endif
if WITH_GNUTLS
sngrep_SOURCES+=capture_gnutls.c
sngrep_CFLAGS+=$(LIBGNUTLS_CFLAGS) $(LIBGCRYPT_CFLAGS)
sngrep_LDADD+=$(LIBGNUTLS_LIBS) $(LIBGCRYPT_LIBS)
endif
if WITH_OPENSSL
sngrep_SOURCES+=capture_openssl.c
sngrep_CFLAGS+=$(SSL_CFLAGS)
sngrep_LDADD+=$(SSL_LIBS)
endif
if WITH_PCRE2
sngrep_CFLAGS+=$(PCRE2_CFLAGS)
sngrep_LDADD+=$(PCRE2_LIBS)
endif
if WITH_ZLIB
sngrep_CFLAGS+=$(ZLIB_CFLAGS)
sngrep_LDADD+=$(ZLIB_LIBS)
endif
if WITH_BPF
sngrep_SOURCES+=capture_bpf.c capture_bpf_source.c capture_bpf_util.c
sngrep_CFLAGS+=$(LIBBPF_CFLAGS) -I$(srcdir)/bpf
sngrep_LDADD+=$(LIBBPF_LIBS)

BUILT_SOURCES=sngrep_tls.skel.h
CLEANFILES=sngrep_tls.skel.h sngrep_tls.bpf.o

sngrep_tls.bpf.o: $(srcdir)/bpf/sngrep_tls.bpf.c $(srcdir)/bpf/sngrep_tls.h
	$(CLANG) -g -O2 -Wall -target bpf -ffreestanding \
	    -D__TARGET_ARCH_$(BPF_TARGET_ARCH) \
	    -I$(srcdir)/bpf $(LIBBPF_CFLAGS) $(BPF_ARCH_CFLAGS) \
	    -c $< -o $@

sngrep_tls.skel.h: sngrep_tls.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

# Without this, a parallel make can compile capture_bpf.c before bpftool has
# written the skeleton header it includes
capture_bpf.$(OBJEXT): sngrep_tls.skel.h
endif

EXTRA_DIST=bpf/sngrep_tls.bpf.c bpf/sngrep_tls.h

sngrep_SOURCES+=address.c packet.c sip.c sip_call.c sip_msg.c sip_attr.c main.c
sngrep_SOURCES+=option.c group.c filter.c keybinding.c media.c setting.c rtp.c
sngrep_SOURCES+=capture_esp.c
sngrep_SOURCES+=util.c hash.c vector.c curses/ui_panel.c curses/scrollbar.c
sngrep_SOURCES+=curses/ui_manager.c curses/ui_call_list.c curses/ui_call_flow.c curses/ui_call_raw.c
sngrep_SOURCES+=curses/ui_stats.c curses/ui_filter.c curses/ui_save.c curses/ui_msg_diff.c
sngrep_SOURCES+=curses/ui_column_select.c curses/ui_settings.c

