# sngrep

This 'tool' aims to make easier the task of my workmates while using ngrep
on heavy load production systems. When a lot of SIP messages are flowing
in your screen, it's useful to have a tool that order them and print in a
fashion way.

This can be also used as a PCAP file viewer, although will only show SIP
packages.

sngrep is a small development done in a couple of days. It has not been
properly coded. It has not been propely tested. It should not even reach
the alpha stage, but can be handy somehow in strange cases.

## Prerequisites

 - libncurse5 - for UI , windows, panels.
 - libpcap - for parsing pcap files. (Optional, see below)
 - ngrep - for live captures. (Optional, see below)
 - stdbuf - for piping ngrep output unbuffered. (Optional, see below)

## Installing
 
On most systems the commands to build will be the standard atotools procedure: 

	./configure
	make
	make install (as root)

If you dont want to use ngrep (because is not available in your system or you 
don't have stdbuf), you can use libpcap instead. You will lose all available
options of ngrep (such as saving to .pcap while capturing) but both online and
offline modes will work.

	./configure --disable-ngrep
	make
	make install (as root)

If you dont want to use libpcap (because is not available in your system or
don't want to install more development libraries), you can use ngrep insted.
Parsing with ngrep will be a slower, but both online and offline modes will
work.

	./configure --disable-libpcap
	make
	make install (as root)


## Usage

sngrep can be used to view SIP packages from a pcap file 

    sngrep file.pcap

or online parsing using ngrep filters and parameters (man ngrep for a full list) 

    sngrep -O file.pcap port 5060 and udp and host 192.168.8.101

if you compiled without ngrep support, use only the filters

	sngrep port 5060 and udp


## Frequent Asked Questions
 <dl>
  <dt>Why a new tool from network filtering?</dt>
  <dd>Don't know. I didn't find any console tool that will display call flows.</dd>
  <dt>Why dont you filter packages in online mode instead of using ngrep?</dt>
  <dd>You can actually capture disabling ngrep using only pcap filters (see Installing options above)</dd>
  <dt>Why only parsing SIP Messages?</dt>
  <dd>Because it's useful for us</dd>
  <dt>Extended Call flow window doesn't work</dt>
  <dd>If you want to make relations between different dialogs (extended callflow)
   a header must be present in of the dialogs referencing the other one.
   This header can be X-CID or X-Call-ID and must contain the Call-ID of the 
   other related dialog.</dd>
  <dt>I have found a bug, what should I do?</dt>
  <dd>There are LOTS of bugs. The strange thing will be you haven't found one.
   Just write an issue at github and I will try to fix it.</dd>
  <dt>I think the idea is better than the tool.</dt>
  <dd> I think that too. If you want to start a new tool with the same purpose
   send me an email, I'll want to contribute.</dd>
</dl>

## License 
    sngrep - SIP callflow viewer using ngrep
    Copyright (C) 2013 Irontec S.L.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

