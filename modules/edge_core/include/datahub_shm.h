#ifndef EDGE_DATAHUB_SHM_H
#define EDGE_DATAHUB_SHM_H

#include <stdint.h>
#include "datahub.h"

#define DATAHUB_SHM_MAGIC 0x44484231u /* "DHB1" */
#define DATAHUB_SHM_ABI_VERSION 1u
#define DATAHUB_SHM_HEADER_SIZE 32u

/* Header is followed by the pipe_msg_t table. sequence is the global
 * writer seqlock; data_offset is 8-byte aligned. */
typedef struct {
    uint32_t magic;
    uint16_t abi_version;
    uint16_t header_size;
    uint32_t capacity;
    uint32_t record_size;
    uint64_t sequence;
    uint64_t data_offset;
} datahub_shm_header_t;

_Static_assert(sizeof(datahub_shm_header_t) == DATAHUB_SHM_HEADER_SIZE,
               "datahub SHM header must match DATAHUB_SHM_HEADER_SIZE");

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief POSIX Shared Memory VTable implementation
 */
extern const datahub_ops_t g_datahub_shm_ops;

int datahub_shm_init(datahub_t *hub, const datahub_config_t *config);
int datahub_shm_publish(datahub_t *hub, const pipe_msg_t *msg);
int datahub_shm_write_shadow(datahub_t *hub, const pipe_msg_t *msg);
int datahub_shm_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg);
int datahub_shm_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data);
int datahub_shm_query_snapshot(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg);
int datahub_shm_get_stats(datahub_t *hub, datahub_backend_stats_t *stats);
void datahub_shm_destroy(datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_DATAHUB_SHM_H */
