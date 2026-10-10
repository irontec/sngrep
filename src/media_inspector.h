/*
 * sngrep - SIP Messages flow viewer
 * Copyright (C) 2026 contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SNGREP_MEDIA_INSPECTOR_H
#define SNGREP_MEDIA_INSPECTOR_H

#include <stddef.h>
#include <stdint.h>
#include "address.h"
#include "packet.h"

#define MEDIA_INSPECTOR_MAX_FLOWS 2048

/* One unidirectional RTP flow observed on a capture interface. */
typedef struct media_inspector_flow {
    address_t src, dst;
    uint32_t ssrc;
    uint32_t first_seq, max_seq;
    uint64_t seen_recent;
    uint64_t packets, bytes, unique, duplicates, out_of_order;
    uint64_t first_us, last_us;
    uint8_t payload_type;
    uint32_t clock_rate;
    double jitter_units, jitter_ms, previous_transit;
    int jitter_available;
    int initialized;
} media_inspector_flow_t;

/* All functions are called under the capture lock in capture/UI code. */
void media_inspector_set_enabled(int enabled);
int media_inspector_enabled(void);
void media_inspector_reset(void);
/* Returns 1 for a validated, aggregated RTP packet; 0 otherwise. */
int media_inspector_ingest(packet_t *packet);
size_t media_inspector_snapshot(media_inspector_flow_t *out, size_t capacity);
size_t media_inspector_flow_count(void);
uint64_t media_inspector_estimated_loss(const media_inspector_flow_t *flow);

#endif
