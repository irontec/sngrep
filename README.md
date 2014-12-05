# sngrep [![Build Status](https://travis-ci.org/irontec/sngrep.svg)](https://travis-ci.org/irontec/sngrep)

sngrep is a tool for displaying SIP calls message flows.

It supports live capture to display realtime SIP packets and can also be used
as PCAP viewer.

## Prerequisites

 - libncurse5 - for UI , windows, panels.
 - libpcap - for capturing packets.
 - libssl - (optional) for tls decrypt

## Installing
 
On most systems the commands to build will be the standard atotools procedure: 

	./configure
	make
	make install (as root)

If you dont want to use libpcap (because is not available in your system or
don't want to install more development libraries), you can use release 0.0.0
available at https://github.com/irontec/sngrep/releases

## Usage

sngrep can be used to view SIP packets from a pcap file

    sngrep file.pcap

or live capturing

	sngrep port 5060 and udp

You can configure some options using a sngreprc file

## Frequent Asked Questions
 <dl>
  <dt>Why a new tool from network filtering?</dt>
  <dd>Don't know. I didn't find any console tool that will display call flows.</dd>
  <dt>Extended Call flow window doesn't work</dt>
  <dd>If you want to make relations between different dialogs (extended callflow)
   a header must be present in of the dialogs referencing the other one.
   This header can be X-CID or X-Call-ID and must contain the Call-ID of the 
   other related dialog.</dd>
</dl>

## License 
    sngrep - SIP Messages flow viewer
    Copyright (C) 2013,2014 Irontec S.L.

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

