/**
 * @file edge_amp_ipc.c
 * @brief Heterogeneous Multi-Core IPC & Zero-Copy SHM Ring Engine Implementation
 */

#include "edge_amp_ipc.h"
#include <string.h>

static uint32_t calculate_checksum(const amp_ipc_msg_t *msg) {
    uint32_t c = msg->seq ^ msg->tag_id ^ (uint32_t)(msg->timestamp_ns & 0xFFFFFFFF) ^ msg->payload_len;
    for (uint32_t i = 0; i < msg->payload_len; i++) {
        c = (c << 3) ^ msg->payload[i];
    }
    return c;
}

int edge_amp_ipc_init(edge_amp_ipc_t *ipc, void *shm_base, size_t shm_size) {
    if (!ipc) return -1;
    memset(ipc, 0, sizeof(edge_amp_ipc_t));

    if (shm_base && shm_size >= sizeof(amp_shm_ring_t) * 2) {
        /* Use external carveout SHM memory */
        ipc->tx_ring = (amp_shm_ring_t *)shm_base;
        ipc->rx_ring = (amp_shm_ring_t *)((uint8_t *)shm_base + sizeof(amp_shm_ring_t));
    } else {
        /* Use internal static storage */
        ipc->tx_ring = &ipc->internal_tx_storage;
        ipc->rx_ring = &ipc->internal_rx_storage;
    }

    memset(ipc->tx_ring, 0, sizeof(amp_shm_ring_t));
    memset(ipc->rx_ring, 0, sizeof(amp_shm_ring_t));
    ipc->tx_ring->capacity = AMP_IPC_RING_CAPACITY;
    ipc->rx_ring->capacity = AMP_IPC_RING_CAPACITY;

    ipc->remote_core_state = AMP_CORE_STATE_RUNNING;
    ipc->next_tx_seq = 1;
    ipc->next_rx_seq = 1;
    return 0;
}

int edge_amp_ipc_send(edge_amp_ipc_t *ipc, uint32_t tag_id, const void *payload, uint32_t len, uint64_t timestamp_ns) {
    if (!ipc || !ipc->tx_ring) return -1;
    if (len > AMP_IPC_MAX_PAYLOAD) return -2;

    amp_shm_ring_t *ring = ipc->tx_ring;
    uint32_t head = ring->head;
    uint32_t tail = ring->tail;

    if ((head - tail) >= ring->capacity) {
        ipc->telemetry.msgs_dropped++;
        return -3; /* Ring Full */
    }

    uint32_t idx = head % ring->capacity;
    amp_ipc_msg_t *slot = &ring->slots[idx];

    slot->seq = ipc->next_tx_seq++;
    slot->tag_id = tag_id;
    slot->timestamp_ns = timestamp_ns;
    slot->payload_len = len;
    if (payload && len > 0) {
        memcpy(slot->payload, payload, len);
    }
    slot->checksum = calculate_checksum(slot);

    __sync_synchronize(); /* Memory barrier */
    ring->head = head + 1;

    ipc->telemetry.msgs_sent++;
    return 0;
}

int edge_amp_ipc_recv(edge_amp_ipc_t *ipc, amp_ipc_msg_t *out_msg) {
    if (!ipc || !ipc->rx_ring || !out_msg) return -1;

    amp_shm_ring_t *ring = ipc->rx_ring;
    uint32_t head = ring->head;
    uint32_t tail = ring->tail;

    if (head == tail) {
        return 0; /* Ring Empty */
    }

    uint32_t idx = tail % ring->capacity;
    amp_ipc_msg_t *slot = &ring->slots[idx];

    uint32_t expected_chk = calculate_checksum(slot);
    if (slot->checksum != expected_chk) {
        ipc->telemetry.checksum_errors++;
        ring->tail = tail + 1;
        return -2; /* Checksum error */
    }

    memcpy(out_msg, slot, sizeof(amp_ipc_msg_t));

    __sync_synchronize(); /* Memory barrier */
    ring->tail = tail + 1;

    ipc->telemetry.msgs_received++;
    return 1; /* 1 message received */
}

int edge_amp_ipc_heartbeat_tick(edge_amp_ipc_t *ipc, uint64_t current_time_ms) {
    if (!ipc) return -1;
    ipc->last_heartbeat_time_ms = current_time_ms;
    ipc->telemetry.heartbeats_received++;
    if (ipc->remote_core_state == AMP_CORE_STATE_OFFLINE || ipc->remote_core_state == AMP_CORE_STATE_FAULT) {
        ipc->remote_core_state = AMP_CORE_STATE_RUNNING;
    }
    return 0;
}

int edge_amp_ipc_poll_watchdog(edge_amp_ipc_t *ipc, uint64_t current_time_ms, bool *out_reloaded) {
    if (!ipc) return -1;
    if (out_reloaded) *out_reloaded = false;

    if (ipc->last_heartbeat_time_ms == 0) {
        ipc->last_heartbeat_time_ms = current_time_ms;
        return 0;
    }

    uint64_t elapsed = current_time_ms - ipc->last_heartbeat_time_ms;

    switch (ipc->remote_core_state) {
        case AMP_CORE_STATE_RUNNING:
            if (elapsed > AMP_HEARTBEAT_TIMEOUT_MS) {
                /* Remote core timed out! Trigger fault */
                ipc->remote_core_state = AMP_CORE_STATE_FAULT;
                ipc->telemetry.heartbeat_timeouts++;
                ipc->reload_start_time_ms = current_time_ms;
            }
            break;

        case AMP_CORE_STATE_FAULT:
            /* Transition to Reloading */
            ipc->remote_core_state = AMP_CORE_STATE_RELOADING;
            ipc->telemetry.reload_events++;
            break;

        case AMP_CORE_STATE_RELOADING: {
            uint64_t reload_elapsed = current_time_ms - ipc->reload_start_time_ms;
            if (reload_elapsed >= 50) { /* Simulate 50ms fast firmware reload */
                /* Reset ring pointers */
                ipc->tx_ring->head = 0;
                ipc->tx_ring->tail = 0;
                ipc->rx_ring->head = 0;
                ipc->rx_ring->tail = 0;
                ipc->last_heartbeat_time_ms = current_time_ms;
                ipc->remote_core_state = AMP_CORE_STATE_RECOVERED;
                if (out_reloaded) *out_reloaded = true;
            } else if (reload_elapsed > AMP_RELOAD_MAX_TIME_MS) {
                /* Timeout during reload */
                ipc->remote_core_state = AMP_CORE_STATE_OFFLINE;
            }
            break;
        }

        case AMP_CORE_STATE_RECOVERED:
            ipc->remote_core_state = AMP_CORE_STATE_RUNNING;
            break;

        default:
            break;
    }

    return 0;
}

void edge_amp_ipc_get_telemetry(const edge_amp_ipc_t *ipc, amp_telemetry_t *out_telem) {
    if (ipc && out_telem) {
        memcpy(out_telem, &ipc->telemetry, sizeof(amp_telemetry_t));
    }
}
