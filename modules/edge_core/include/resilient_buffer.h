/**
 * @file resilient_buffer.h
 * @brief Resilient Telemetry Store-and-Forward Buffer Engine for EdgeC (Crash-Resilient WAL Support)
 */

#ifndef RESILIENT_BUFFER_H
#define RESILIENT_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define RESILIENT_BUFFER_CAPACITY 128
#define RESILIENT_WAL_MAGIC 0x52425546 /* "RBUF" */

typedef struct {
    uint32_t record_id;
    uint32_t tag_id;
    double val;
    uint64_t timestamp_ms;
} telemetry_record_t;

typedef struct {
    uint32_t magic;
    uint32_t count;
    uint32_t head;
    uint32_t tail;
} resilient_wal_header_t;

typedef struct {
    size_t head;
    size_t tail;
    size_t count;
    bool network_online;
    uint32_t max_rate_per_sec;
    telemetry_record_t records[RESILIENT_BUFFER_CAPACITY];
} resilient_buffer_t;

/**
 * @brief Initializes resilient buffer
 * @param buf Pointer to resilient buffer instance
 * @param max_rate Record drain rate limit per second
 * @return 0 on success, negative error code on failure
 */
int resilient_buffer_init(resilient_buffer_t *buf, uint32_t max_rate);

/**
 * @brief Appends a new telemetry record to the buffer (Store)
 * @param buf Pointer to resilient buffer instance
 * @param tag_id DataHub Tag ID
 * @param val Physical value
 * @param timestamp_ms Measurement timestamp in ms
 * @return 0 on success, negative error code on failure (buffer full)
 */
int resilient_buffer_push(resilient_buffer_t *buf, uint32_t tag_id, double val, uint64_t timestamp_ms);

/**
 * @brief Pops pending telemetry records when network is restored (Forward)
 * @param buf Pointer to resilient buffer instance
 * @param out_record Output record
 * @return 0 on success, 1 if empty/throttled, negative error code on failure
 */
int resilient_buffer_pop(resilient_buffer_t *buf, telemetry_record_t *out_record);

/**
 * @brief Updates network connectivity state
 * @param buf Pointer to resilient buffer instance
 * @param online True if network connected, false if offline
 */
void resilient_buffer_set_net_state(resilient_buffer_t *buf, bool online);

/**
 * @brief Persists buffer contents to a crash-resilient WAL file
 * @param buf Pointer to resilient buffer instance
 * @param filepath Output WAL filepath
 * @return 0 on success, negative error code on failure
 */
int resilient_buffer_flush_to_file(const resilient_buffer_t *buf, const char *filepath);

/**
 * @brief Restores buffer contents from a crash-resilient WAL file upon restart
 * @param buf Pointer to resilient buffer instance
 * @param filepath Input WAL filepath
 * @return 0 on success, negative error code on failure
 */
int resilient_buffer_load_from_file(resilient_buffer_t *buf, const char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* RESILIENT_BUFFER_H */
