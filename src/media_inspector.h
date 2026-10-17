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
 * @file media_inspector.h
 * @author Mohammad Bagher Amini <darwvin@hotmail.com>
 *
 * @brief Public API for standalone RTP stream inspection
 */

#ifndef SNGREP_MEDIA_INSPECTOR_H
#define SNGREP_MEDIA_INSPECTOR_H

#include <stddef.h>
#include <stdint.h>
#include "address.h"
#include "packet.h"

#define MEDIA_INSPECTOR_MAX_FLOWS 2048

/**
 * @brief Aggregate counters of all currently retained RTP flows.
 */
typedef struct media_inspector_stats
{
    size_t count;        //!< Number of retained flow entries.
    uint64_t packets;    //!< Observed RTP packets across the table.
    uint64_t loss;       //!< Estimated sequence gaps across the table.
} media_inspector_stats_t;

/**
 * @brief A unidirectional RTP flow observed at the capture point.
 */
typedef struct media_inspector_flow
{
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

/**
 * @brief Enable or disable passive RTP inspection.
 *
 * @param value Non-zero enables media inspection, zero disables it.
 */
void
media_inspector_set_enabled(int value);

/**
 * @brief Enable or disable standalone media-only mode.
 *
 * Standalone mode bypasses SIP parsing; F6 inspection in normal mode does not.
 *
 * @param value Non-zero enables standalone mode (and inspection), zero disables it.
 */
void
media_inspector_set_standalone(int value);

/**
 * @brief Check whether standalone media-only mode is enabled.
 *
 * @return Non-zero when enabled, zero otherwise.
 */
int
media_inspector_standalone(void);

/**
 * @brief Check whether media-only inspection is enabled.
 *
 * @return Non-zero when enabled, zero otherwise.
 */
int
media_inspector_enabled(void);

/**
 * @brief Reset the retained flow table.
 *
 * @note Caller must hold the capture lock if capture is active.
 */
void
media_inspector_reset(void);

/**
 * @brief Validate and aggregate one RTP packet without retaining its payload.
 *
 * @param packet Packet received from the capture pipeline.
 * @return 1 for an accepted RTP packet, zero otherwise.
 * @note Caller must hold the capture lock.
 */
int
media_inspector_ingest(packet_t *packet);

/**
 * @brief Copy up to capacity tracked flows into the output array.
 *
 * @param out Destination array, or NULL to request a limited count.
 * @param capacity Maximum number of flows to copy.
 * @return Minimum of retained flow count and capacity.
 * @note Caller must hold the capture lock.
 */
size_t
media_inspector_snapshot(media_inspector_flow_t *out, size_t capacity);

/**
 * @brief Return the number of retained RTP flows.
 *
 * @return Active flow count, at most MEDIA_INSPECTOR_MAX_FLOWS.
 * @note Caller must hold the capture lock.
 */
size_t
media_inspector_flow_count(void);

/**
 * @brief Estimate missing RTP sequence numbers for a flow.
 *
 * @param flow Flow to inspect, or NULL.
 * @return Estimated sequence gaps, or zero for invalid flow.
 */
uint64_t
media_inspector_estimated_loss(const media_inspector_flow_t *flow);

/**
 * @brief Snapshot total packets and estimated gaps across all retained flows.
 *
 * @return Aggregate flow count, RTP packet count, and estimated loss.
 * @note Acquires/releases the capture lock internally.
 */
media_inspector_stats_t
media_inspector_snapshot_stats(void);

#endif /* SNGREP_MEDIA_INSPECTOR_H */
