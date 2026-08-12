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
 * @file capture_bpf.h
 * @author Jurijs Ivolga <jurijs.ivolga@gmail.com>
 *
 * @brief eBPF based capture of SIP over TLS
 *
 * Attaches uprobes to every libssl on the system, recovering SIP plaintext
 * without a private key or a certificate. sngrep has to run on the same host
 * as the SIP daemon, on Linux 5.8 or newer with BTF, as root or with CAP_BPF
 * and CAP_PERFMON.
 *
 * libpcap and libbpf both define struct bpf_insn, and neither guards against
 * the other, so no translation unit may include both. This header therefore
 * pulls in neither: capture_bpf.c owns the libbpf side and capture_bpf_source.c
 * owns the libpcap side.
 *
 */
#ifndef __SNGREP_CAPTURE_BPF_H
#define __SNGREP_CAPTURE_BPF_H

#include "config.h"
#include <stddef.h>
#include "bpf/sngrep_tls.h"

/**
 * @brief Load the eBPF programs and attach them to every libssl found
 *
 * @return 0 on success, non zero on failure with a message already printed
 */
int
capture_bpf_init(void);

/**
 * @brief Detach every probe and release all eBPF resources
 */
void
capture_bpf_deinit(void);

/**
 * @brief Wait for captured plaintext and dispatch whatever arrives
 *
 * @param timeout_ms How long to block when no events are pending
 *
 * @return Events consumed, or a negative errno on failure
 */
int
capture_bpf_poll(int timeout_ms);

/*
 * The functions below live in capture_bpf_source.c, which owns the libpcap
 * side of this feature. They are declared here so capture_bpf.c can reach
 * them without including capture.h, and therefore pcap.h.
 */

/**
 * @brief Register the eBPF capture source with the capture layer
 *
 * @return 0 on success, non zero on failure with a message already printed
 */
int
capture_bpf_source_create(void);

/**
 * @brief Release everything the capture source holds
 */
void
capture_bpf_source_destroy(void);

/**
 * @brief Turn one captured chunk into a packet and hand it to sngrep
 *
 * @param e    Event as delivered by the ring buffer
 * @param size Bytes the ring buffer actually delivered
 */
void
capture_bpf_source_handle(const struct tls_event *e, size_t size);

/**
 * @brief Ring buffer polling loop, run as the capture source thread
 */
void *
capture_bpf_thread(void *info);

#endif /* __SNGREP_CAPTURE_BPF_H */
