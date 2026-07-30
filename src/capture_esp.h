/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2026 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2026 Irontec SL. All rights reserved.
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
 ****************************************************************************/
/**
 * @file capture_esp.h
 * @author Marco Sinibaldi <marco.sinibaldi@hpe.com>
 *
 * @brief Decode SIP carried inside IPsec ESP with NULL encryption
 *
 * When ESP uses the NULL encryption transform (transport mode), the inner
 * UDP/TCP header and its payload travel in clear text right after the 8-byte
 * ESP header (SPI + Sequence Number). These helpers locate the inner transport
 * header and the SIP payload without needing any decryption key. The exact
 * payload boundary is recovered by parsing the ESP trailer (Pad Length + Next
 * Header) and stripping the Integrity Check Value (ICV), whose length is
 * auto-detected from the standard self-describing padding pattern.
 */
#ifndef __SNGREP_CAPTURE_ESP_H
#define __SNGREP_CAPTURE_ESP_H

#include <sys/types.h>
#include <stdint.h>

//! Size of the ESP header (SPI + Sequence Number)
#define ESP_HEADER_LEN 8

/**
 * @brief Locate the inner transport header and SIP payload of an ESP-NULL packet
 *
 * Given a buffer starting at the ESP header, detect whether the inner (clear
 * text) payload is a UDP datagram or TCP segment and, if so, report the inner
 * protocol, ports and the offset/length of the SIP payload.
 *
 * The ESP trailer ([ Padding | Pad Length | Next Header | ICV ]) is parsed to
 * recover the exact inner segment length. The ICV length is not on the wire,
 * so common values are probed and validated against the self-describing
 * padding pattern (1, 2, 3, ...) and the resulting transport header. This
 * yields an exact payload boundary for both UDP and TCP (required so TCP
 * reassembly is not corrupted by trailing trailer/ICV bytes).
 *
 * @param esp         Pointer to the ESP header (start of SPI)
 * @param esp_len     Number of valid bytes available from esp
 * @param proto       Out: detected inner protocol (IPPROTO_UDP or IPPROTO_TCP)
 * @param sport       Out: inner source port (host byte order)
 * @param dport       Out: inner destination port (host byte order)
 * @param payload_off Out: offset (from esp) of the SIP payload
 * @param payload_len Out: length of the SIP payload
 * @return 0 on success, -1 if the payload is not a decodable ESP-NULL packet
 */
int capture_esp_null_parse(const u_char *esp, uint32_t esp_len,
                           uint8_t *proto, uint16_t *sport, uint16_t *dport,
                           uint32_t *payload_off, uint32_t *payload_len);

#endif /* __SNGREP_CAPTURE_ESP_H */
