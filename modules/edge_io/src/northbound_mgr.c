#include "northbound_mgr.h"

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "contract_data_types.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "snf_manager.h"
#include "sparkplug_b.h"
#include "web_server.h"

#ifndef uv__queue_data
#define uv__queue_data(q, type, link) (type*)((char*)q - offsetof(type, link))
#endif

#ifndef uv__queue_empty
#define uv__queue_empty(q) ((q) == (q)->next)
#endif

#ifndef uv__queue_head
#define uv__queue_head(q) ((q)->next)
#endif

#ifndef uv__queue_remove
#define uv__queue_remove(p)          \
    do {                             \
        (p)->next->prev = (p)->prev; \
        (p)->prev->next = (p)->next; \
    } while (0)
#endif

#ifndef uv__queue_init
#define uv__queue_init(q) \
    do {                  \
        (q)->next = (q);  \
        (q)->prev = (q);  \
    } while (0)
#endif

extern int sparkplug_b_driver_register(driver_manager_t* mgr);

/* 主题身份由驱动持有，这里只问不猜（早先复制一份结构体去 cast ctx->priv，
 * 布局一改就静默错位）。 */
static void _nb_topic(northbound_mgr_t* mgr, edge_driver_ctx_t* ctx, const char* suffix,
                      char* out, size_t out_sz) {
    char group[64] = {0};
    char node[64] = {0};
    if (sparkplug_b_identity(ctx->priv, group, sizeof(group), node, sizeof(node)) != 0) {
        snprintf(out, out_sz, "spBv1.0/unknown/%s/unknown", suffix);
        return;
    }
    snprintf(out, out_sz, "spBv1.0/%s/%s/%s", group, suffix, node);
    (void)mgr;
}

static int _push_to_transport_direct(northbound_mgr_t* mgr, uint32_t tag_id, double val,
                                     uint64_t ts) {
    if (!mgr->transport.is_connected || !mgr->transport.is_connected(mgr->transport.ctx)) {
        printf("☁️  [CLOUD-MOCK] 物理连接未就绪，通过影子链路推送告警: TagID=%X, Value=%.2f\n",
               tag_id, val);
        return 0;
    }

    if (!mgr || !mgr->nb_driver) return -1;

    edge_driver_ctx_t* ctx = (edge_driver_ctx_t*)mgr->nb_driver;
    net_buf_t* nb = net_buf_alloc(ctx->pool, 0);
    int rc = -1;
    if (nb) {
        /* [ADR-002] 北向生产者走 native 槽位：metric 名由端口给，驱动不查 datahub。
         * scratch 是请求级的，prepare 与 encode 背靠背调用，放栈上就够。 */
        alignas(max_align_t) uint8_t scratch[REQ_DRV_PRIV_MAX];
        memset(scratch, 0, sizeof(scratch));
        const char* name = (mgr->north_src && mgr->north_src->name)
                               ? mgr->north_src->name(mgr->north_src_ctx, tag_id)
                               : NULL;
        bool ok = ctx->driver->req_prepare &&
                  ctx->driver->req_prepare(scratch, 1, tag_id, NULL, 0, name,
                                           name ? strlen(name) + 1 : 0, true, val) == 0 &&
                  ctx->driver->req_encode && ctx->driver->req_encode(scratch, ctx->priv, nb) == 0;
        if (ok) {
            char topic[256];
            _nb_topic(mgr, ctx, "NDATA", topic, sizeof(topic));
            if (mgr->transport.publish) {
                if (mgr->transport.publish(mgr->transport.ctx, topic, nb->b.data, nb->b.len) == 0) {
                    rc = 0;
                }
            }
        }
        net_buf_unref(nb);
    }
    return rc;
}

static void _push_to_transport(northbound_mgr_t* mgr, uint32_t tag_id, double val, uint64_t ts) {
    if (!mgr) return;

    if (mgr->transport.is_connected && mgr->transport.is_connected(mgr->transport.ctx)) {
        if (_push_to_transport_direct(mgr, tag_id, val, ts) == 0) return;
    }

    // [SnF] If offline or publish failed, buffer to SQLite
    if (mgr->snf) {
        PipeMsg msg = {0};
        msg.ts = ts;
        msg.tag_id = tag_id;
        msg.payload.val.f = val;
        msg.type = PIPEMSG_TYPE_SCALAR;
        snf_manager_buffer_data(mgr->snf, &msg);
    }
}

static int _snf_publish_wrapper(void* user_data, const PipeMsg* msg) {
    return _push_to_transport_direct((northbound_mgr_t*)user_data, msg->tag_id, msg->payload.val.f,
                                     msg->ts);
}

static void on_data_notify_timer(uv_timer_t* handle) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)handle->data;
    if (!mgr || !mgr->user_data) return;

    bool is_online = mgr->transport.is_connected && mgr->transport.is_connected(mgr->transport.ctx);
    if (mgr->snf) {
        snf_manager_set_online(mgr->snf, is_online);
    }

    if (is_online && mgr->nb_driver) {
        edge_driver_ctx_t* ctx = (edge_driver_ctx_t*)mgr->nb_driver;
        /* request-native 生产者：状态由端口给，驱动不碰 datahub */
        if (ctx->driver->req_tick) {
            net_buf_t* tick_buf = NULL;
            if (ctx->driver->req_tick(ctx->priv, mgr->north_src, mgr->north_src_ctx, NULL, NULL,
                                      ctx->pool, &tick_buf) == 0 &&
                tick_buf) {
                char topic[256];
                _nb_topic(mgr, ctx, "NBIRTH", topic, sizeof(topic));
                if (mgr->transport.publish) {
                    mgr->transport.publish(mgr->transport.ctx, topic, tick_buf->b.data,
                                           tick_buf->b.len);
                }
                net_buf_unref(tick_buf);
            }
        }
    }

    PipeMsg msg;
    while (mgr->source.read_next && mgr->source.read_next(mgr->source.ctx, &mgr->last_seq, &msg) == 0) {
        if (msg.type == PIPEMSG_TYPE_SCALAR) {
            _push_to_transport(mgr, msg.tag_id, msg.payload.val.f, msg.ts);
        }
    }
}

static void _on_req_notify(uv_async_t* handle) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)handle->data;
    struct uv__queue* q;
    uv_mutex_lock(&mgr->req_lock);
    while (!uv__queue_empty(&mgr->req_queue)) {
        q = uv__queue_head(&mgr->req_queue);
        nb_req_entry_t* entry = uv__queue_data(q, nb_req_entry_t, node);
        uv__queue_remove(q);
        uv_mutex_unlock(&mgr->req_lock);

        if (entry->req->ops && entry->req->ops->setup) {
            entry->req->ops->setup(entry->req, mgr->user_data);
        }

        free(entry->req->priv_data);
        free(entry->req);
        free(entry);

        uv_mutex_lock(&mgr->req_lock);
    }
    uv_mutex_unlock(&mgr->req_lock);
}

static void on_north_stop(uv_async_t* handle) {
    uv_stop(handle->loop);
}

static void north_thread_entry(void* arg) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)arg;
    if (!mgr) return;

    if (mgr->nb_driver) {
        ((edge_driver_ctx_t*)mgr->nb_driver)->user_data = mgr->source.ctx;
    }

    mgr->loop = &mgr->loop_storage;
    uv_loop_init(mgr->loop);

    uv_timer_init(mgr->loop, &mgr->poll_timer);
    mgr->poll_timer.data = mgr;
    uv_timer_start(&mgr->poll_timer, on_data_notify_timer, 100, 100);

    uv_async_init(mgr->loop, &mgr->stop_async, on_north_stop);
    mgr->stop_async.data = mgr;

    uv_async_init(mgr->loop, &mgr->req_async, _on_req_notify);
    mgr->req_async.data = mgr;

    // [SnF] Initialize Store-and-Forward
    if (mgr->db) {
        snf_config_t snf_cfg = {.db = mgr->db,
                                .loop = mgr->loop,
                                .publish_fn = _snf_publish_wrapper,
                                .user_data = mgr,
                                .sync_batch_size = 50,
                                .sync_interval_ms = 5000};
        mgr->snf = snf_manager_create(&snf_cfg);
    }

    uv_run(mgr->loop, UV_RUN_DEFAULT);

    if (mgr->snf) snf_manager_destroy(mgr->snf);

    uv_loop_close(mgr->loop);
    mgr->loop = NULL;
}

int northbound_mgr_init(northbound_mgr_t* mgr, uv_loop_t* loop, datahub_t* hub,
                        const char* product_name, struct sqlite_svc_s* db) {
    (void)loop;
    nb_source_ops_t src = {
        .ctx = hub,
        .read_next = (int (*)(void*, uint64_t*, pipe_msg_t*))datahub_read_ring
    };
    nb_transport_ops_t transport = {0};
    return northbound_mgr_init_ex(mgr, src, product_name, db, transport);
}

int northbound_mgr_init_ex(northbound_mgr_t* mgr, nb_source_ops_t source, const char* product_name,
                           struct sqlite_svc_s* db, nb_transport_ops_t transport) {
    memset(mgr, 0, sizeof(*mgr));
    mgr->source = source;
    mgr->db = db;
    strncpy(mgr->product_name, product_name, sizeof(mgr->product_name) - 1);
    mgr->transport = transport;

    uv__queue_init(&mgr->req_queue);
    uv_mutex_init(&mgr->req_lock);

    mgr->drv_mgr = driver_manager_create();
    sparkplug_b_driver_register(mgr->drv_mgr);
    mgr->nb_driver = driver_instance_create(mgr->drv_mgr, "SparkplugB", 1);

    if (mgr->nb_driver) {
        mgr->nb_driver->pool = net_buf_pool_new(32, 1024, 0);
    }

    return 0;
}

int northbound_mgr_start(northbound_mgr_t* mgr) {
    if (!mgr || mgr->loop) return -1;
    return uv_thread_create(&mgr->thread, north_thread_entry, mgr);
}

void northbound_mgr_stop(northbound_mgr_t* mgr) {
    if (mgr && mgr->loop) {
        uv_async_send(&mgr->stop_async);
        uv_thread_join(&mgr->thread);
    }
}

void northbound_mgr_destroy(northbound_mgr_t* mgr) {
    if (!mgr) return;
    if (mgr->nb_driver) {
        if (mgr->nb_driver->pool) net_buf_pool_unref(mgr->nb_driver->pool);
        driver_instance_destroy(mgr->nb_driver, "SparkplugB");
        mgr->nb_driver = NULL;
    }
    if (mgr->drv_mgr) {
        driver_manager_destroy(mgr->drv_mgr);
        mgr->drv_mgr = NULL;
    }
    uv_mutex_destroy(&mgr->req_lock);
}

void northbound_mgr_set_rest_server(northbound_mgr_t* mgr, void* rest_srv) {
    if (mgr) {
        mgr->rest_srv = rest_srv;
    }
}
