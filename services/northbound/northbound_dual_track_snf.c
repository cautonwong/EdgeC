/**
 * @file northbound_dual_track_snf.c
 * @brief Dual-Track Northbound Relay (IEC 104 & MQTT) with Store-and-Forward (SnF) Implementation (AEP-463)
 */

#include "northbound_dual_track_snf.h"

#include <string.h>

void northbound_dual_track_init(northbound_dual_track_engine_t *engine, uint32_t replay_rate_limit) {
    if (!engine) return;
    memset(engine, 0, sizeof(*engine));
    engine->iec104_online = true;
    engine->mqtt_online = true;
    engine->replay_rate_limit_per_tick = replay_rate_limit > 0 ? replay_rate_limit : 64;
}

void northbound_dual_track_set_channel_status(northbound_dual_track_engine_t *engine,
                                              northbound_track_t track, bool is_online) {
    if (!engine) return;
    if (track == TRACK_IEC104) {
        engine->iec104_online = is_online;
    } else if (track == TRACK_MQTT) {
        engine->mqtt_online = is_online;
    }
}

int northbound_dual_track_dispatch(northbound_dual_track_engine_t *engine,
                                   const northbound_record_t *rec) {
    if (!engine || !rec) return -1;

    // 1. Primary Realtime IEC 104 Channel
    if (engine->iec104_online) {
        engine->total_realtime_sent++;
    }

    // 2. Secondary MQTT IoT Management Channel
    if (engine->mqtt_online) {
        // Direct online transmission
        engine->total_realtime_sent++;
    } else {
        // Offline: Store into SnF ring queue
        snf_ring_queue_t *q = &engine->snf_storage;
        if (q->count < SNF_QUEUE_CAPACITY) {
            q->queue[q->tail] = *rec;
            q->tail = (q->tail + 1) % SNF_QUEUE_CAPACITY;
            q->count++;
            engine->total_snf_buffered++;
        } else {
            // Buffer full: drop oldest to maintain FIFO bounded queue
            q->head = (q->head + 1) % SNF_QUEUE_CAPACITY;
            q->queue[q->tail] = *rec;
            q->tail = (q->tail + 1) % SNF_QUEUE_CAPACITY;
            q->dropped_count++;
            engine->total_snf_buffered++;
        }
    }

    return 0;
}

uint32_t northbound_dual_track_step_replay(northbound_dual_track_engine_t *engine,
                                           northbound_record_t *out_replayed, uint32_t max_out) {
    if (!engine || !engine->mqtt_online || engine->snf_storage.count == 0) {
        return 0;
    }

    uint32_t limit = engine->replay_rate_limit_per_tick;
    if (max_out > 0 && limit > max_out) limit = max_out;

    snf_ring_queue_t *q = &engine->snf_storage;
    uint32_t replayed = 0;

    while (q->count > 0 && replayed < limit) {
        if (out_replayed) {
            out_replayed[replayed] = q->queue[q->head];
        }
        q->head = (q->head + 1) % SNF_QUEUE_CAPACITY;
        q->count--;
        replayed++;
        engine->total_snf_replayed++;
    }

    return replayed;
}

uint32_t northbound_dual_track_get_snf_count(const northbound_dual_track_engine_t *engine) {
    return engine ? engine->snf_storage.count : 0;
}
