#ifndef EDGE_DATAHUB_SHM_H
#define EDGE_DATAHUB_SHM_H

#include "datahub.h"

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
