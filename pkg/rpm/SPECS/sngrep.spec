%bcond_with openssl
%bcond_with gnutls
# Build against legacy PCRE (v1) instead of PCRE2. PCRE2 is the default because
# PCRE(v1) has been dropped from recent Fedora releases.
%bcond_with pcre1

Summary:            SIP Messages flow viewer
Name:               sngrep
Version:            0.0.99
Release:            0%{?dist}
License:            GPLv3
Group:              Applications/Engineering
Source:             https://github.com/irontec/sngrep/archive/refs/tags/0.0.99.tar.gz
URL:                http://github.com/irontec/sngrep

BuildRequires: gcc
BuildRequires: make
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: pkgconfig
BuildRequires: ncurses-devel
BuildRequires: libpcap-devel
%if %{with pcre1}
BuildRequires: pcre-devel
%else
BuildRequires: pcre2-devel
%endif
%if %{with openssl}
BuildRequires: openssl-devel
%endif
%if %{with gnutls}
BuildRequires: gnutls-devel
%endif

%description
sngrep displays SIP Messages grouped by Call-Id into flow
diagrams. It can be used as an offline PCAP viewer or online
capture using libpcap functions.

It supports SIP UDP, TCP and TLS transports (when each message is
delivered in one packet).

You can also create new PCAP files from captures or displayed dialogs.

%prep
%setup -q

%build
./bootstrap.sh
%configure \
%if %{with pcre1}
    --with-pcre \
%else
    --with-pcre2 \
%endif
    --enable-unicode \
    --enable-ipv6 \
    --enable-eep \
    %{?with_openssl:--with-openssl} \
    %{?with_gnutls:--with-gnutls}

%make_build

%install
%make_install

%files
%license COPYING
%doc README TODO ChangeLog
%{_bindir}/sngrep
%{_mandir}/man8/sngrep.8*
%config(noreplace) %{_sysconfdir}/sngreprc

%changelog
* Thu Oct 16 2025 Ivan Alonso <kaian@irontec.com> - 1.8.3
 - Version 1.8.3
* Mon Jul 08 2024 Ivan Alonso <kaian@irontec.com> - 1.8.2
 - Version 1.8.2
* Mon Apr 08 2024 Ivan Alonso <kaian@irontec.com> - 1.8.1
 - Version 1.8.1
* Wed Dec 20 2023 Ivan Alonso <kaian@irontec.com> - 1.8.0
 - Version 1.8.0
* Fri Mar 31 2023 Ivan Alonso <kaian@irontec.com> - 1.7.0
 - Version 1.7.0
* Wed Aug 31 2022 Ivan Alonso <kaian@irontec.com> - 1.6.0
 - Version 1.6.0
* Tue Apr 26 2022 Ivan Alonso <kaian@irontec.com> - 1.5.0
 - Version 1.5.0
* Fri Nov 19 2021 Ivan Alonso <kaian@irontec.com> - 1.4.10
 - Version 1.4.10
* Thu May 20 2021 Ivan Alonso <kaian@irontec.com> - 1.4.9
 - Version 1.4.9
* Tue Oct 10 2020 Ivan Alonso <kaian@irontec.com> - 1.4.8
 - Version 1.4.8
* Thu May 21 2020 Ivan Alonso <kaian@irontec.com> - 1.4.7
 - Version 1.4.7
* Wed Oct 31 2018 Ivan Alonso <kaian@irontec.com> - 1.4.6
 - Version 1.4.6
* Fri Dec 22 2017 Ivan Alonso <kaian@irontec.com> - 1.4.5
 - Version 1.4.5
* Sun Sep 17 2017 Ivan Alonso <kaian@irontec.com> - 1.4.4
 - Version 1.4.4
* Wed May 10 2017 Ivan Alonso <kaian@irontec.com> - 1.4.3
 - Version 1.4.3
* Fri Dec 19 2016 Ivan Alonso <kaian@irontec.com> - 1.4.2
 - Version 1.4.2
* Fri Oct 28 2016 Ivan Alonso <kaian@irontec.com> - 1.4.1
 - Version 1.4.1
* Tue Aug 23 2016 Ivan Alonso <kaian@irontec.com> - 1.4.0
 - Version 1.4.0
* Mon Mar 28 2016 Ivan Alonso <kaian@irontec.com> - 1.3.1
 - Version 1.3.1
* Tue Mar 15 2016 Ivan Alonso <kaian@irontec.com> - 1.3.0
 - Version 1.3.0
* Thu Dec 10 2015 Ivan Alonso <kaian@irontec.com> - 1.2.0
 - Version 1.2.0
* Wed Oct 28 2015 Ivan Alonso <kaian@irontec.com> - 1.1.0
 - Version 1.1.0
* Tue Oct 06 2015 Ivan Alonso <kaian@irontec.com> - 1.0.0
 - Version 1.0.0
* Mon Aug 31 2015 Ivan Alonso <kaian@irontec.com> - 0.4.2
 - Version 0.4.2
* Tue Jul 07 2015 Ivan Alonso <kaian@irontec.com> - 0.4.1
 - Version 0.4.1
* Mon Jun 29 2015 Ivan Alonso <kaian@irontec.com> - 0.4.0
 - Version 0.4.0
* Tue Apr 14 2015 Ivan Alonso <kaian@irontec.com> - 0.3.1
 - Version 0.3.1
* Wed Mar 04 2015 Ivan Alonso <kaian@irontec.com> - 0.3.0
 - First RPM version of sngrep
