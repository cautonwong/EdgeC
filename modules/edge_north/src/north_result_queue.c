#include "north_result_queue.h"

#include <stdio.h>
#include <string.h>

#include "edge_log.h"

/* 线路名与线路值只在这里生成：accepted 与 result 两类回执共用同一队列与编码路径。 */
static void _format(const nb_receipt_entry_t *e, char *point, size_t point_sz, double *value) {
    if (e->kind == NB_RECEIPT_ACCEPTED) {
        if (e->name[0]) {
            snprintf(point, point_sz, "%s.ack", e->name);
        } else {
            snprintf(point, point_sz, "ack.%u", e->tag_id);
        }
        *value = e->ok ? 1.0 : 0.0;
        return;
    }

    if (e->name[0]) {
        snprintf(point, point_sz, "%s.result", e->name);
    } else {
        snprintf(point, point_sz, "result.%u", e->tag_id);
    }
    *value = (e->status == NB_RESULT_STATUS_OK)       ? 1.0
             : (e->status == NB_RESULT_STATUS_TIMEOUT) ? 2.0
                                                       : 0.0;
}

static void _publish_entry(north_result_queue_t *q, const nb_receipt_entry_t *entry) {
    char point[NB_RECEIPT_NAME_MAX + 8];
    double value = 0.0;
    _format(entry, point, sizeof(point), &value);
    if (q->publish) (void)q->publish(q->publish_ctx, point, entry->tag_id, value);
}

static void _on_async(uv_async_t *handle) {
    north_result_queue_t *q = (north_result_queue_t *)handle->data;
    if (!q) return;
    for (;;) {
        nb_receipt_entry_t entry;
        uv_mutex_lock(&q->lock);
        if (q->count == 0) {
            uv_mutex_unlock(&q->lock);
            break;
        }
        entry = q->entries[q->head];
        q->head = (q->head + 1) % NB_RESULT_QUEUE_CAP;
        q->count--;
        uv_mutex_unlock(&q->lock);
        _publish_entry(q, &entry);
    }
}

void north_result_queue_init(north_result_queue_t *q, nb_receipt_publish_fn publish, void *ctx) {
    if (!q) return;
    uv_mutex_init(&q->lock);
    q->publish = publish;
    q->publish_ctx = ctx;
    q->head = 0;
    q->count = 0;
    q->async_init = false;
    q->stopped = false;
}

void north_result_queue_attach(north_result_queue_t *q, uv_loop_t *loop) {
    if (!q || !loop) return;
    uv_mutex_lock(&q->lock);
    /* 上一轮没 drain 的条目不得在新运行里重放（restart 语义）。 */
    q->head = 0;
    q->count = 0;
    q->stopped = false;
    uv_async_init(loop, &q->async, _on_async);
    q->async.data = q;
    q->async_init = true;
    uv_mutex_unlock(&q->lock);
}

void north_result_queue_stop(north_result_queue_t *q) {
    if (!q) return;
    uv_mutex_lock(&q->lock);
    q->stopped = true;
    uv_mutex_unlock(&q->lock);
}

void north_result_queue_detach(north_result_queue_t *q) {
    if (!q) return;
    uv_mutex_lock(&q->lock);
    bool created = q->async_init;
    q->async_init = false;
    q->stopped = true;
    q->head = 0;
    q->count = 0;
    uv_mutex_unlock(&q->lock);
    /* 置位在锁内：post 在锁内 send，不会碰到正在关闭的 handle。 */
    if (created) uv_close((uv_handle_t *)&q->async, NULL);
}

void north_result_queue_destroy(north_result_queue_t *q) {
    if (!q) return;
    uv_mutex_destroy(&q->lock);
}

static int _post(north_result_queue_t *q, nb_receipt_kind_t kind, uint32_t tag_id,
                 const char *name, bool ok, int status) {
    if (!q) return -1;
    uv_mutex_lock(&q->lock);
    if (!q->async_init || q->stopped) {
        uv_mutex_unlock(&q->lock);
        return -1;
    }
    if (q->count >= NB_RESULT_QUEUE_CAP) {
        uv_mutex_unlock(&q->lock);
        LOG_WARN(LOG_CAT_NB, "Northbound: receipt queue full (%d), dropping tag %u",
                 NB_RESULT_QUEUE_CAP, tag_id);
        return -1;
    }

    nb_receipt_entry_t *entry = &q->entries[(q->head + q->count) % NB_RESULT_QUEUE_CAP];
    entry->kind = (uint8_t)kind;
    entry->tag_id = tag_id;
    entry->ok = ok;
    entry->status = status;
    snprintf(entry->name, sizeof(entry->name), "%s", name ? name : "");
    q->count++;
    uv_async_send(&q->async);
    uv_mutex_unlock(&q->lock);
    return 0;
}

int north_result_queue_post_accepted(north_result_queue_t *q, uint32_t tag_id, const char *name,
                                     bool ok) {
    return _post(q, NB_RECEIPT_ACCEPTED, tag_id, name, ok, 0);
}

int north_result_queue_post_result(north_result_queue_t *q, uint32_t tag_id, const char *name,
                                   int status) {
    return _post(q, NB_RECEIPT_RESULT, tag_id, name, false, status);
}

int north_result_queue_publish_accepted_now(north_result_queue_t *q, uint32_t tag_id,
                                            const char *name, bool ok) {
    if (!q || !q->publish) return -1;
    nb_receipt_entry_t entry = {
        .kind = NB_RECEIPT_ACCEPTED, .tag_id = tag_id, .ok = ok, .status = 0};
    snprintf(entry.name, sizeof(entry.name), "%s", name ? name : "");
    char point[NB_RECEIPT_NAME_MAX + 8];
    double value = 0.0;
    _format(&entry, point, sizeof(point), &value);
    return q->publish(q->publish_ctx, point, tag_id, value);
}
