%bcond_with openssl

Summary:            SIP Messages flow viewer
Name:               sngrep
Version:            1.4.7
Release:            0%{?dist}
License:            GPLv3
Group:              Applications/Engineering
BuildRoot:          %{_tmppath}/%{name}-%{version}-%{release}-root
Source:             https://github.com/irontec/%{name}/archive/v%{version}.zip
URL:                http://github.com/irontec/sngrep
BuildRequires: ncurses-devel 
BuildRequires: make 
BuildRequires: libpcap-devel 
BuildRequires: pcre-devel
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: gcc
%if %{with openssl}
BuildRequires: openssl-devel
%endif
Requires: ncurses
Requires: libpcap
Requires: pcre

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
%configure --with-pcre \
    --enable-unicode \
    --enable-ipv6 \
    --enable-eep \
    %{?_with_openssl}

make %{?_smp_mflags}

%install
%{__make} install DESTDIR=%{buildroot}

%files
%doc README TODO COPYING ChangeLog
%{_bindir}/*
%{_mandir}/man8/*
%config(noreplace) %{_sysconfdir}/*

%clean
%{__rm} -rf %{buildroot}

%changelog
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
