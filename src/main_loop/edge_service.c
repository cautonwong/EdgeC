#include "edge_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "sqlite_service.h"

struct service_s {
    uv_loop_t *loop;
    service_options_t opts;
    datahub_t *data_hub;
    sqlite_svc_t *storage;
    arbiter_t *arbiter;
    northbound_mgr_t *cloud;
    net_buf_pool_t *pool;
    struct stream_engine_s *stream_engine;  // [新增]

    // 命令处理队列 (异步控制面)
    uv_mutex_t cmd_lock;
    uv_async_t cmd_async;
    cmd_msg_t cmd_queue[16];  // 对齐权威类型
    int cmd_head;
    int cmd_tail;

    char *contract_path;
};

static void on_cmd_async(uv_async_t *handle) {
    service_t *svc = (service_t *)handle->data;
    while (1) {
        cmd_msg_t cmd;
        uv_mutex_lock(&svc->cmd_lock);
        if (svc->cmd_head == svc->cmd_tail) {
            uv_mutex_unlock(&svc->cmd_lock);
            break;
        }
        cmd = svc->cmd_queue[svc->cmd_tail];
        svc->cmd_tail = (svc->cmd_tail + 1) % 16;
        uv_mutex_unlock(&svc->cmd_lock);

        // 转发给 Arbiter。目前暂定总线 ID 为 0
        if (svc->arbiter) arbiter_submit_cmd(svc->arbiter, 0, &cmd);
    }
}

service_t *service_new(service_options_t *opts) {
    service_t *svc = calloc(1, sizeof(service_t));
    if (svc) {
        if (opts) svc->opts = *opts;
        uv_mutex_init(&svc->cmd_lock);
    }
    return svc;
}

void service_set_loop(service_t *svc, uv_loop_t *loop) {
    if (svc) svc->loop = loop;
}
void service_set_datahub(service_t *svc, datahub_t *hub) {
    if (svc) svc->data_hub = hub;
}
void service_set_storage(service_t *svc, sqlite_svc_t *storage) {
    if (svc) svc->storage = storage;
}
uv_loop_t *service_get_loop(service_t *svc) {
    return svc ? svc->loop : NULL;
}

shm_ring_t *service_get_ring(service_t *svc) {
    return (svc && svc->data_hub) ? &svc->data_hub->ring : NULL;
}

datahub_t *service_get_datahub(service_t *svc) {
    return svc ? svc->data_hub : NULL;
}
arbiter_t *service_get_arbiter(service_t *svc) {
    return svc ? svc->arbiter : NULL;
}
sqlite_svc_t *service_get_storage(service_t *svc) {
    return svc ? svc->storage : NULL;
}

void service_set_stream_engine(service_t *svc, struct stream_engine_s *engine) {
    if (svc) svc->stream_engine = engine;
}
struct stream_engine_s *service_get_stream_engine(service_t *svc) {
    return svc ? svc->stream_engine : NULL;
}

void service_set_contract_path(service_t *svc, const char *path) {
    if (!svc) return;
    if (svc->contract_path) free(svc->contract_path);
    svc->contract_path = strdup(path);
}

const char *service_get_contract_path(service_t *svc) {
    return svc ? svc->contract_path : NULL;
}

int service_run(service_t *svc, uv_loop_t *loop, net_buf_pool_t *pool, arbiter_t *arbiter,
                datahub_t *hub, persistence_mgr_t *storage, northbound_mgr_t *cloud) {
    svc->loop = loop;
    svc->pool = pool;
    svc->arbiter = arbiter;
    svc->data_hub = hub;
    svc->storage = (sqlite_svc_t *)storage->writer;
    svc->cloud = cloud;

    printf("[Service] Container booting up in mode: %s\n", svc->opts.mode);
    uv_async_init(svc->loop, &svc->cmd_async, on_cmd_async);
    svc->cmd_async.data = svc;

    return uv_run(loop, UV_RUN_DEFAULT);
}

int service_post_cmd(service_t *svc, cmd_msg_t *cmd) {
    uv_mutex_lock(&svc->cmd_lock);
    int next = (svc->cmd_head + 1) % 16;
    if (next == svc->cmd_tail) {
        uv_mutex_unlock(&svc->cmd_lock);
        return -1;
    }
    svc->cmd_queue[svc->cmd_head] = *cmd;
    svc->cmd_head = next;
    uv_mutex_unlock(&svc->cmd_lock);
    return uv_async_send(&svc->cmd_async);
}

void service_delete(service_t *svc) {
    if (!svc) return;
    if (svc->contract_path) free(svc->contract_path);
    uv_mutex_destroy(&svc->cmd_lock);
    free(svc);
}
