/**
 * @file northbound_dual_track_snf.h
 * @brief Dual-Track Northbound Relay (IEC 104 & MQTT) with Store-and-Forward (SnF) Resuming (AEP-463)
 */

#ifndef NORTHBOUND_DUAL_TRACK_SNF_H
#define NORTHBOUND_DUAL_TRACK_SNF_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SNF_QUEUE_CAPACITY 8192

typedef enum {
    TRACK_IEC104 = 0, // IEC 104 real-time dispatch channel
    TRACK_MQTT = 1    // MQTT / JSON IoT management channel
} northbound_track_t;

typedef struct {
    uint32_t tag_id;
    uint32_t device_id;
    double value;
    uint64_t timestamp_ms;
    uint8_t quality;
    bool is_soe_event;
} northbound_record_t;

typedef struct {
    northbound_record_t queue[SNF_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t dropped_count;
} snf_ring_queue_t;

typedef struct {
    bool iec104_online;
    bool mqtt_online;
    uint32_t replay_rate_limit_per_tick; // Max historical records to replay per cycle
    uint64_t total_realtime_sent;
    uint64_t total_snf_buffered;
    uint64_t total_snf_replayed;
    snf_ring_queue_t snf_storage;
} northbound_dual_track_engine_t;

/* Engine Initialization */
void northbound_dual_track_init(northbound_dual_track_engine_t *engine, uint32_t replay_rate_limit);

/* Set Channel Online/Offline Status */
void northbound_dual_track_set_channel_status(northbound_dual_track_engine_t *engine,
                                              northbound_track_t track, bool is_online);

/* Push New Record to Dual-Track Engine */
int northbound_dual_track_dispatch(northbound_dual_track_engine_t *engine,
                                   const northbound_record_t *rec);

/* Step Reconnection Replay Loop (Replays up to replay_rate_limit records in order) */
uint32_t northbound_dual_track_step_replay(northbound_dual_track_engine_t *engine,
                                           northbound_record_t *out_replayed, uint32_t max_out);

/* Query Engine State */
uint32_t northbound_dual_track_get_snf_count(const northbound_dual_track_engine_t *engine);

#ifdef __cplusplus
}
#endif

#endif // NORTHBOUND_DUAL_TRACK_SNF_H
