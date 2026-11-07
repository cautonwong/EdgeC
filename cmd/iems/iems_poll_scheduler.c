#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "edge_log.h"
#include "iems_driver_adapter.h"
#include "iems_poll_scheduler.h"
#include "iems_task_loader.h"

typedef struct {
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    int dense_idx;
    char protocol[16];
    uint32_t interval_ms;
    uv_periodic_t scheduler;
    iems_node_t *agg_root;
    struct uv__queue node;
} iems_poll_task_t;

static void on_poll_complete(int status, const pipe_msg_t *result, void *user_data) {
    // user_data 是聚合根(比 arbiter 活得久),不是 poll task —— 关停时 task 会先被释放。
    iems_node_t *node = (iems_node_t *)user_data;
    if (status != 0 || !result || !node) return;
    if (result->type != PIPEMSG_TYPE_SCALAR) return;
    // 镜像只能由聚合根写;这里只转交采样,不做任何堆分配。
    if (iems_node_ingest_sample(node, result->tag_id, result->payload.val.f) != 0) {
        static uint64_t last_warn_ns;  // 热路径:失败日志全局限流到 60s 一次
        uint64_t now = uv_hrtime();
        if (now - last_warn_ns > 60ULL * 1000000000ULL) {
            last_warn_ns = now;
            LOG_WARN(LOG_CAT_CORE, "IEMS ingest failed for tag %u", result->tag_id);
        }
    }
}

static void on_poll_task_trigger(uv_periodic_t *handle) {
    iems_poll_task_t *task = (iems_poll_task_t *)handle->data;
    if (!task) return;

    uint32_t conn_count = 0;
    const edge_connectivity_t *conns = iems_get_connectivity(&conn_count);
    if (!conns) return;

    bool is_southbound = false;
    for (uint32_t i = 0; i < conn_count; i++) {
        if (conns[i].id == task->bus_id && strcmp(conns[i].category, "SOUTHBOUND") == 0) {
            is_southbound = true;
            break;
        }
    }

    if (is_southbound) {
        /* [ADR-002] 驱动只认原生点位（CAN 是 CAN ID），回调只认产品 tag_id，
         * 量纲也由产品给。三件事都在这一层做完，驱动不查表、不标定。 */
        uint32_t native_id = iems_driver_adapter_native_id(task->protocol, task->tag_id);
        double scale = iems_driver_adapter_tag_scale(task->protocol, task->tag_id);
        /* 周期采集是后台任务：低优先级 */
        edge_api_read_tag_ex(task->bus_id, task->protocol, task->slave_id, native_id, task->tag_id,
                             scale, NULL, 0, ARBITER_PRIO_LOW, on_poll_complete, task->agg_root);
    }
}

static void _on_iems_task_loaded(const iems_loaded_task_t *loaded, void *user_data) {
    iems_poll_scheduler_t *s = (iems_poll_scheduler_t *)user_data;
    if (!loaded || !s) return;

    struct uv__queue *q;
    uv__queue_foreach(q, &s->tasks) {
        iems_poll_task_t *existing = uv__queue_data(q, iems_poll_task_t, node);
        if (existing->bus_id == loaded->bus_id && existing->slave_id == loaded->slave_id &&
            existing->tag_id == loaded->tag_id)
            return;
    }

    iems_poll_task_t *task = calloc(1, sizeof(iems_poll_task_t));
    task->bus_id = loaded->bus_id;
    task->slave_id = loaded->slave_id;
    task->tag_id = loaded->tag_id;
    task->dense_idx = edge_tag_get_compact_index(loaded->tag_id);
    task->agg_root = s->node;
    strncpy(task->protocol, loaded->protocol, 15);
    task->interval_ms = 1000;

    const edge_tag_meta_t *meta = iems_get_tag_meta(loaded->tag_id);
    if (meta && meta->poll_interval_ms > 0) {
        task->interval_ms = meta->poll_interval_ms;
    }

    printf("📝 [POLL REGISTER] Tag: %u, Bus: %d, Protocol: %s, Interval: %d\n", task->tag_id,
           task->bus_id, task->protocol, task->interval_ms);

    uv__queue_insert_tail(&s->tasks, &task->node);
    uv_periodic_init(s->loop, &task->scheduler);
    task->scheduler.data = task;
    uv_periodic_set(&task->scheduler, 0, (double)task->interval_ms, NULL);
    uv_periodic_start(&task->scheduler, on_poll_task_trigger);
}

static void iems_load_tasks_timer_cb(uv_timer_t *handle) {
    iems_poll_scheduler_t *s = (iems_poll_scheduler_t *)handle->data;
    if (!s || !s->db) return;

    int loaded = iems_poll_tasks_load(s->db, _on_iems_task_loaded, s);
    if (loaded > 0) {
        s->assets_loaded = true;
        uv_timer_stop(handle);
        uv_close((uv_handle_t *)handle, NULL);
        s->load_timer_open = false;
    }
}

static void _on_task_close(uv_handle_t *handle) {
    uv_periodic_t *periodic = (uv_periodic_t *)handle->data;
    iems_poll_task_t *task = (iems_poll_task_t *)periodic->data;
    free(task);
}

void iems_poll_scheduler_init(iems_poll_scheduler_t *s, uv_loop_t *loop, iems_node_t *node,
                              sqlite_svc_t *db) {
    if (!s) return;
    s->loop = loop;
    s->node = node;
    s->db = db;
    uv__queue_init(&s->tasks);
    s->load_timer_open = false;
    s->assets_loaded = false;
}

void iems_poll_scheduler_start(iems_poll_scheduler_t *s) {
    if (!s || !s->loop) return;
    uv_timer_init(s->loop, &s->load_timer);
    s->load_timer.data = s;
    s->load_timer_open = true;
    uv_timer_start(&s->load_timer, iems_load_tasks_timer_cb, 50, 200);
}

void iems_poll_scheduler_stop(iems_poll_scheduler_t *s) {
    if (!s) return;

    if (s->load_timer_open) {
        uv_timer_stop(&s->load_timer);
        uv_close((uv_handle_t *)&s->load_timer, NULL);
        s->load_timer_open = false;
    }

    while (!uv__queue_empty(&s->tasks)) {
        struct uv__queue *q = uv__queue_head(&s->tasks);
        iems_poll_task_t *task = uv__queue_data(q, iems_poll_task_t, node);
        uv__queue_remove(q);
        uv_periodic_stop(&task->scheduler);
        uv_close((uv_handle_t *)&task->scheduler.timer, _on_task_close);
    }
}
