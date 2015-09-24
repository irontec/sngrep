/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2015 Ivan Alonso (Kaian)
 ** Copyright (C) 2015 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ** In addition, as a special exception, the copyright holders give
 ** permission to link the code of portions of this program with the
 ** OpenSSL library under certain conditions as described in each
 ** individual source file, and distribute linked combinations
 ** including the two.
 ** You must obey the GNU General Public License in all respects
 ** for all of the code used other than OpenSSL.  If you modify
 ** file(s) with this exception, you may extend this exception to your
 ** version of the file(s), but you are not obligated to do so.  If you
 ** do not wish to do so, delete this exception statement from your
 ** version.  If you delete this exception statement from all source
 ** files in the program, then also delete it here.
 **
 ****************************************************************************/
/**
 * @file capture_tcpreasm.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage reassembly IP/TCP packets
 *
 * This file contains the functions and structures to manage the reassembly of
 * captured packets.
 */

#ifndef __SNGREP_CAPTURE_REASM_H
#define __SNGREP_CAPTURE_REASM_H

#include "capture.h"

/**
 * @brief Reassembly capture IP fragments
 *
 * This function will try to assemble received PCAP data into a single IP packet.
 * It will return a packet structure if no fragmentation is found or a full packet
 * has been assembled.
 *
 * @note We assume packets higher than MAX_CAPTURE_LEN won't be SIP. This has been
 * done to avoid reassembling too big packets, that aren't likely to be interesting
 * for sngrep.
 *
 * TODO
 * Assembly only works when all of the IP fragments are received in the good order.
 * Properly check memory boundaries during packet reconstruction.
 * Implement a way to timeout pending IP fragments after some time.
 * TODO
 *
 * @param capinfo Packet capture session information
 * @para header Header received from libpcap callback
 * @para packet Packet contents received from libpcap callback
 * @param size Packet size (not including Layer and Network headers)
 * @param caplen Full packet size (current fragment -> whole assembled packet)
 * @return a Packet structure when packet is not fragmented or fully reassembled
 * @return NULL when packet has not been completely assembled
 */
capture_packet_t *
capture_packet_reasm_ip(capture_info_t *capinfo, const struct pcap_pkthdr *header,
                        u_char *packet, uint32_t *size, uint32_t *caplen);

/**
 * @brief Reassembly capture TCP segments
 *
 * This function will try to assemble TCP segments of an existing packet.
 *
 * @note We assume packets higher than MAX_CAPTURE_LEN won't be SIP. This has been
 * done to avoid reassembling too big packets, that aren't likely to be interesting
 * for sngrep.
 *
 * @param packet Capture packet structure
 * @param tcp TCP header extracted from capture packet data
 * @param payload Assembled TCP packet payload content
 * @param size_payload Payload length
 * @return a Packet structure when packet is not segmented or fully reassembled
 * @return NULL when packet has not been completely assembled
 */
capture_packet_t *
capture_packet_reasm_tcp(capture_packet_t *packet, struct tcphdr *tcp,
                         u_char *payload, int size_payload);

#endif /* __SNGREP_CAPTURE_REASM_H */
