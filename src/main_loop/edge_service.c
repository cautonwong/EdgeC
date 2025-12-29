#include "edge_service.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 结构体定义隐藏在 .c 文件中，实现黑盒封装
struct service_s {
    uv_loop_t *loop;
    service_options_t opts;

    // 组件指针
    net_buf_pool_t *pool;
    arbiter_t *arbiter;
    datahub_t *data_hub;
    persistence_mgr_t *storage;
    northbound_mgr_t *cloud;

    // 指令桥接
    uv_async_t cmd_async;
    uv_mutex_t cmd_lock;
    service_cmd_t cmd_queue[16];
    int cmd_head, cmd_tail;

    int shutting_down;
};

static void on_signal(uv_signal_t *handle, int signum) {
    (void)signum;
    service_t *svc = (service_t *)handle->data;
    printf("\n[Service] Signal received, stopping loop...\n");
    uv_stop(svc->loop);
}

static void on_cmd_async(uv_async_t *handle) {
    service_t *svc = (service_t *)handle->data;
    service_cmd_t cmd;

    while (true) {
        bool has_cmd = false;
        uv_mutex_lock(&svc->cmd_lock);
        if (svc->cmd_head != svc->cmd_tail) {
            cmd = svc->cmd_queue[svc->cmd_tail];
            svc->cmd_tail = (svc->cmd_tail + 1) % 16;
            has_cmd = true;
        }
        uv_mutex_unlock(&svc->cmd_lock);

        if (!has_cmd) break;

        // 这里执行指令分发逻辑 (Scenarios S5, S6, S7)
        printf("[Service] Processing Command Type: %d for Tag: %u\n", cmd.type, cmd.tag_id);
    }
}

service_t *service_new(service_options_t *opts) {
    service_t *svc = calloc(1, sizeof(struct service_s));
    if (svc && opts) {
        svc->opts = *opts;
    }
    return svc;
}

uv_loop_t *service_get_loop(service_t *svc) {
    return svc ? svc->loop : NULL;
}

shm_ring_t *service_get_ring(service_t *svc) {
    return (svc && svc->data_hub) ? &svc->data_hub->ring : NULL;
}

shm_ring_t *service_get_shmring(service_t *svc) {
    return service_get_ring(svc);
}

arbiter_t *service_get_arbiter(service_t *svc) {
    return svc ? svc->arbiter : NULL;
}

datahub_t *service_get_datahub(service_t *svc) {
    return svc ? svc->data_hub : NULL;
}

persistence_mgr_t *service_get_storage(service_t *svc) {
    return svc ? svc->storage : NULL;
}

void service_set_loop(service_t *svc, uv_loop_t *loop) {
    if (svc) svc->loop = loop;
}

int service_run(service_t *svc, uv_loop_t *loop, net_buf_pool_t *pool, arbiter_t *arbiter,
                datahub_t *hub, persistence_mgr_t *storage, northbound_mgr_t *cloud) {
    if (!svc || !loop) return -1;

    // 注入依赖
    svc->loop = loop;
    svc->pool = pool;
    svc->arbiter = arbiter;
    svc->data_hub = hub;
    svc->storage = storage;
    svc->cloud = cloud;

    printf("[Service] Container booting up in mode: %s\n", svc->opts.mode);

    // 初始化内部桥接
    uv_mutex_init(&svc->cmd_lock);
    uv_async_init(svc->loop, &svc->cmd_async, on_cmd_async);
    svc->cmd_async.data = svc;

    // 注册信号
    static uv_signal_t sig_int, sig_term;
    uv_signal_init(svc->loop, &sig_int);
    sig_int.data = svc;
    uv_signal_start(&sig_int, on_signal, SIGINT);

    uv_signal_init(svc->loop, &sig_term);
    sig_term.data = svc;
    uv_signal_start(&sig_term, on_signal, SIGTERM);

    return uv_run(svc->loop, UV_RUN_DEFAULT);
}

int service_post_cmd(service_t *svc, service_cmd_t *cmd) {
    if (!svc || !cmd) return -1;
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

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        // 如果句柄有自己的自定义释放逻辑（如 malloc 分配的），应在各自的 close_cb 中处理
        // 这里只是强制发起关闭请求
        uv_close(handle, NULL);
    }
}

void service_delete(service_t *svc) {
    if (!svc) return;

    if (svc->loop) {
        // 1. 发起所有句柄的关闭请求 (包括 cmd_async)
        uv_walk(svc->loop, on_walk_close, NULL);

        // 2. 驱动一次 Loop 以处理所有 Close 回调
        uv_run(svc->loop, UV_RUN_DEFAULT);

        // 3. 循环运行直到 Loop 彻底释放
        int ret;
        do {
            ret = uv_loop_close(svc->loop);
            if (ret == UV_EBUSY) {
                uv_run(svc->loop, UV_RUN_ONCE);
            }
        } while (ret == UV_EBUSY);
    }

    uv_mutex_destroy(&svc->cmd_lock);
    free(svc);
}
