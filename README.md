# sngrep [![Build Status](https://travis-ci.org/irontec/sngrep.svg)](https://travis-ci.org/irontec/sngrep)

sngrep is a tool for displaying SIP calls message flows from terminal.

It supports live capture to display realtime SIP packets and can also be used
as PCAP viewer.

[Some screenshots of sngrep](https://github.com/irontec/sngrep/wiki/Screenshots)

## Installing

### Binaries
* [Debian / Ubuntu](https://github.com/irontec/sngrep/wiki/Installing-Binaries#debian--ubuntu)
* [CentOS / RedHat / Fedora](https://github.com/irontec/sngrep/wiki/Installing-Binaries#centos--fedora--rhel)
* [Alpine Linux](https://github.com/irontec/sngrep/wiki/Installing-Binaries#alpine-linux)
* [Gentoo](https://github.com/irontec/sngrep/wiki/Installing-Binaries#gentoo)
* [Arch](https://github.com/irontec/sngrep/wiki/Installing-Binaries#arch)
* [OSX](https://github.com/irontec/sngrep/wiki/Installing-Binaries#osx)
* [OpenWRT/LEDE](https://github.com/irontec/sngrep/wiki/Installing-Binaries#openwrtlede)

### Building from sources
Prerequisites

 - glib2 - core library
 - libncursesw5 - for UI, windows, panels.
 - libpcap - for capturing packets.
 - gnutls - (optional) for TLS transport decrypt using GnuTLS and libgcrypt
 - libsnd - (optional) for saving RTP streams to wav files
 - pulseaudio - (optional) for RTP stream player

#### Preparing source code

Use cmake to generate required makefiles to build the project

    cmake .

All compiler flags are disabled by default, you can enable extra feature siw following flags

| cmake flag | Feature |
| ------------- | ------------- |
| `-DWITH_SSL=ON`    | Adds GnuTLS support to parse TLS captured messages (req. gnutls)  |
| `-DWITH_PULSE=ON`   | Adds Pulseaudio support for RTP playback (req. libpulse) |
| `-DWITH_SND=ON`   | Adds libsnd support for RTP saving to wav (req. libsnd) |
| `-DWITH_G729=ON`   | Adds G.729 support for saving and play streams (req. libbcg729) |
| `-DUSE_IPV6=ON`   | Enable IPv6 packet capture support. |
| `-DUSE_HEP=ON`   | Enable HEPv3 packet send/receive support. |


#### Building and installing

	cmake .
	make install (as root)

You can find [detailed instructions for some distributions](https://github.com/irontec/sngrep/wiki/Building) on wiki.

## Usage

See `--help` for a list of available flags and their syntax

For example, sngrep can be used to view SIP packets from a pcap file, also applying filters

    sngrep -I file.pcap host 192.168.1.1 and port 5060

or live capturing, saving packets to a new file

	sngrep -d eth0 -O save.pcap port 5060 and udp


## Configuration

You can configure some options using [sngreprc](https://github.com/irontec/sngrep/wiki/Configuration) file

## Frequent Asked Questions
Any feedback, request or question are welcomed at [#sngrep](https://webchat.freenode.net/?channels=sngrep) channel at irc.freenode.net

See FAQ on [Github Wiki](https://github.com/irontec/sngrep/wiki#frequent-asked-questions)

## License
    sngrep - SIP Messages flow viewer
    Copyright (C) 2013-2019 Irontec S.L.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations
    including the two.
    You must obey the GNU General Public License in all respects
    for all of the code used other than OpenSSL.  If you modify
    file(s) with this exception, you may extend this exception to your
    version of the file(s), but you are not obligated to do so.  If you
    do not wish to do so, delete this exception statement from your
    version.  If you delete this exception statement from all source
    files in the program, then also delete it here.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

