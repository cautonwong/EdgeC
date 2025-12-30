#ifndef DATAHUB_H
#define DATAHUB_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;
typedef void (*datahub_on_data_cb)(datahub_t *hub, void *user_data);

typedef struct {
    PipeMsg msg;
    bool valid;
} tag_slot_t;

struct datahub_s {
    shm_ring_t ring;
    uv_loop_t *loop;
    uv_async_t async_handle;
    datahub_on_data_cb on_data;
    void *user_data;

    uv_rwlock_t shadow_lock;
    tag_slot_t *shadow_cache;
};

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 datahub_on_data_cb on_data, void *user_data);
void datahub_destroy(datahub_t *hub);
int datahub_publish(datahub_t *hub, const pipe_msg_t *msg);
int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, PipeMsg *out);

#ifdef __cplusplus
}
#endif

#endif