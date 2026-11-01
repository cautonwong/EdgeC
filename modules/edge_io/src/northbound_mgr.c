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
extern int mqtt_json_driver_register(driver_manager_t* mgr);

#define NB_DEFAULT_ADDRESS_TEMPLATE "spBv1.0/{group}/{kind}/{node}"

/* ==============================================================================
 * 传输集合（ADR-0011：数组多路并存）
 * ========================================================================== */

static bool _any_connected(const northbound_mgr_t* mgr) {
    for (uint32_t i = 0; i < mgr->transport_count; i++) {
        const nb_transport_ops_t* t = &mgr->transports[i];
        if (t->is_connected && t->is_connected(t->ctx)) return true;
    }
    return false;
}

/**
 * @brief 会话代数签名 = 各路代数之和。
 *
 * 单调不减，**任一路**重连都会让它变大。单独取某一路或取最大值都不行：
 * 取最大值时，代数低的那一路重连会被高的一路盖住，BIRTH 就不重发了。
 */
static uint32_t _epoch_signature(const northbound_mgr_t* mgr) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < mgr->transport_count; i++) {
        const nb_transport_ops_t* t = &mgr->transports[i];
        if (t->session_epoch) sum += t->session_epoch(t->ctx);
    }
    return sum;
}

/**
 * @brief 在途总数（保守取和）。
 *
 * 任一路还有在途就不确认 —— 宁可晚一轮确认（at-least-once 允许多发一次），
 * 也不要在某一路还没送达时就把记录标成已同步。
 */
static uint32_t _pending_total(const northbound_mgr_t* mgr) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < mgr->transport_count; i++) {
        const nb_transport_ops_t* t = &mgr->transports[i];
        if (t->pending) sum += t->pending(t->ctx);
    }
    return sum;
}

/** 发给所有在线传输。任一路成功即算成功；失败那路自己重试。 */
static int _publish_all(northbound_mgr_t* mgr, const char* address, const uint8_t* data,
                        size_t len) {
    int rc = -1;
    for (uint32_t i = 0; i < mgr->transport_count; i++) {
        nb_transport_ops_t* t = &mgr->transports[i];
        if (!t->publish) continue;
        if (t->is_connected && !t->is_connected(t->ctx)) continue;
        if (t->publish(t->ctx, address, data, len) == 0) rc = 0;
    }
    return rc;
}

/* ==============================================================================
 * 地址渲染（ADR-0011：地址归编排层，不归驱动）
 * ========================================================================== */

/**
 * @brief 用模板渲染地址，替换 `{group}` / `{node}` / `{kind}` / `{point}`。
 *
 * 不认识的占位符**原样保留**：宁可让主站看到一个显眼的 `{xxx}`，
 * 也不要静默拼出一个"看起来正常"的错误主题——那种错排查起来最费时间。
 */
static void _render_address(northbound_mgr_t* mgr, const char* kind, const char* point, char* out,
                            size_t out_sz) {
    if (!out || out_sz == 0) return;

    const char* tmpl =
        mgr->address_template[0] ? mgr->address_template : NB_DEFAULT_ADDRESS_TEMPLATE;
    const char* group = mgr->group_id[0] ? mgr->group_id : "unknown";
    const char* node = mgr->node_id[0] ? mgr->node_id : "unknown";
    const char* pt = point ? point : "";

    size_t o = 0;
    for (const char* p = tmpl; *p && o + 1 < out_sz;) {
        if (*p == '{') {
            const char* close = strchr(p, '}');
            if (close) {
                size_t klen = (size_t)(close - p - 1);
                char kbuf[16];
                const char* v = NULL;
                if (klen < sizeof(kbuf)) {
                    memcpy(kbuf, p + 1, klen);
                    kbuf[klen] = '\0';
                    if (strcmp(kbuf, "group") == 0) {
                        v = group;
                    } else if (strcmp(kbuf, "node") == 0) {
                        v = node;
                    } else if (strcmp(kbuf, "kind") == 0) {
                        v = kind ? kind : "";
                    } else if (strcmp(kbuf, "point") == 0) {
                        v = pt;
                    }
                }
                if (v) {
                    size_t vlen = strlen(v);
                    if (o + vlen + 1 > out_sz) vlen = (o + 1 < out_sz) ? out_sz - o - 1 : 0;
                    memcpy(out + o, v, vlen);
                    o += vlen;
                    p = close + 1;
                    continue;
                }
            }
        }
        out[o++] = *p++;
    }
    out[o] = '\0';
}

static int _push_to_transport_direct_named(northbound_mgr_t* mgr, uint32_t tag_id, double val,
                                           uint64_t ts, const char* name_override) {
    if (!mgr || !mgr->nb_driver) return -1;

    /* ⚠️ 已知缺口（A-P2-3）：`ts` 到这里就断了。
     *
     * `req_prepare` 的签名里没有时间戳，驱动编 payload 时自己取当前时间
     * （sparkplug 用 `uv_hrtime()`）。实时路径影响不大，但 **SnF 回放会出错**：
     * 三天前缓存的数据补发时会打上"现在"的时间戳，历史曲线就假了。
     *
     * 正确修法是把 `ts` 加进 `req_prepare`（13 个驱动各多一个形参，多数忽略）。
     * 这是跨模块签名变更，单独一次做，不夹在地址模板这次里。 */
    (void)ts;

    /* 未连接就是未连接。这里原来 printf 一条然后 return 0，等于告诉调用方
     * "推成功了"——SnF 会据此把记录标成已同步，数据就此消失且不留日志。
     * 与南向 OI 8000 的"假成功"是同一个病。 */
    if (!_any_connected(mgr)) return -1;

    edge_driver_ctx_t* ctx = (edge_driver_ctx_t*)mgr->nb_driver;
    net_buf_t* nb = net_buf_alloc(ctx->pool, 0);
    int rc = -1;
    if (nb) {
        /* [ADR-002] 北向生产者走 native 槽位：metric 名由端口给，驱动不查 datahub。
         * scratch 是请求级的，prepare 与 encode 背靠背调用，放栈上就够。 */
        alignas(max_align_t) uint8_t scratch[REQ_DRV_PRIV_MAX];
        memset(scratch, 0, sizeof(scratch));
        const char* name = name_override;
        if (!name && mgr->north_src && mgr->north_src->name) {
            name = mgr->north_src->name(mgr->north_src_ctx, tag_id);
        }
        bool ok = ctx->driver->req_prepare &&
                  ctx->driver->req_prepare(scratch, 1, tag_id, NULL, 0, name,
                                           name ? strlen(name) + 1 : 0, true, val) == 0 &&
                  ctx->driver->req_encode && ctx->driver->req_encode(scratch, ctx->priv, nb) == 0;
        if (ok) {
            char address[256];
            _render_address(mgr, "NDATA", name, address, sizeof(address));
            rc = _publish_all(mgr, address, nb->b.data, nb->b.len);
        }
        net_buf_unref(nb);
    }
    return rc;
}

static int _push_to_transport_direct(northbound_mgr_t* mgr, uint32_t tag_id, double val,
                                     uint64_t ts) {
    return _push_to_transport_direct_named(mgr, tag_id, val, ts, NULL);
}

static void _push_to_transport(northbound_mgr_t* mgr, uint32_t tag_id, double val, uint64_t ts) {
    if (!mgr) return;

    if (_any_connected(mgr)) {
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

/* ==============================================================================
 * 下行（ADR-0011 N6-2：编排层只送字节，解析归产品）
 * ========================================================================== */

/**
 * @brief 传输层送来一条下行报文 —— 原样转给产品，不解析。
 *
 * 为什么不在这里解：两个协议的点位位置不同（JSON 可能在主题里、Sparkplug NCMD
 * 在 protobuf 的 metric name 里），编排层统一解析只会得到"两边都不像"的东西。
 * 字段语义（哪个命令控哪个继电器）本来就是产品知识。
 */
static void _on_transport_msg(void* user, const char* address, const uint8_t* payload,
                              size_t len) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)user;
    if (!mgr || !address) return;

    if (!mgr->north_src || !mgr->north_src->on_downlink) {
        /* 链路通了但没人处理：明确记一条，不要让云端以为命令被吞了。 */
        LOG_WARN(LOG_CAT_NB, "Northbound: downlink on '%s' (%zu B) has no handler", address, len);
        return;
    }

    int rc = mgr->north_src->on_downlink(mgr->north_src_ctx, address, payload, len);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_NB, "Northbound: downlink on '%s' rejected (rc=%d)", address, rc);
    }
}

/**
 * @brief 惰性订阅下行主题。
 *
 * 不放在 `north_thread_entry` 里一次定死：组合根可能在 start 之后才注入
 * 北向端口，那时也该订得上。每次 poll 检查一次，订上就不再重复。
 */
static void _ensure_downlink_subscribed(northbound_mgr_t* mgr) {
    if (mgr->downlink_subscribed) return;
    if (!mgr->north_src || !mgr->north_src->on_downlink) return;

    char address[256];
    _render_address(mgr, "NCMD", NULL, address, sizeof(address));

    bool any = false;
    for (uint32_t i = 0; i < mgr->transport_count; i++) {
        nb_transport_ops_t* t = &mgr->transports[i];
        if (!t->subscribe) continue;
        if (t->subscribe(t->ctx, address, _on_transport_msg, mgr) == 0) any = true;
    }

    if (any) {
        mgr->downlink_subscribed = true;
        LOG_INFO(LOG_CAT_NB, "Northbound: downlink subscribed on '%s'", address);
    }
}

/* SnF 用它判断"这一批是不是真的送达了"：publish 返回 0 只代表入队。 */
static uint32_t _snf_pending_wrapper(void* user_data) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)user_data;
    return mgr ? _pending_total(mgr) : 0;
}

/**
 * @brief 传输会话重建时，把北向驱动的会话级状态清掉。
 *
 * 只看 `is_connected()` 的 false→true 跳变会漏（断连与重连可能都发生在两次
 * 采样之间），所以比的是单调递增的会话代数签名。首次采样只记基线。
 */
static void _sync_session_epoch(northbound_mgr_t* mgr) {
    if (!mgr || !mgr->nb_driver) return;

    uint32_t epoch = _epoch_signature(mgr);

    if (mgr->last_epoch == UINT32_MAX) {
        mgr->last_epoch = epoch; /* 首次采样：只记基线，不触发复位 */
        return;
    }
    if (epoch == mgr->last_epoch) return;

    mgr->last_epoch = epoch;

    edge_driver_ctx_t* ctx = (edge_driver_ctx_t*)mgr->nb_driver;
    if (ctx->driver && ctx->driver->req_session_reset) {
        ctx->driver->req_session_reset(ctx->priv);
    }
    LOG_INFO(LOG_CAT_NB,
             "Northbound: transport session rebuilt (epoch sig %u) — driver session state reset.",
             epoch);
}

/**
 * @brief 把 ring 里的消息抽干。
 *
 * 两条路都会调它：数据到达唤醒（`_on_data_async`）与兜底轮询（`poll_timer`）。
 * 抽干而不是"每次取一条"——一次唤醒通常带一批，逐条取会把唤醒次数放大。
 */
static void _drain_source(northbound_mgr_t* mgr) {
    PipeMsg msg;
    while (mgr->source.read_next &&
           mgr->source.read_next(mgr->source.ctx, &mgr->last_seq, &msg) == 0) {
        if (msg.type == PIPEMSG_TYPE_SCALAR) {
            _push_to_transport(mgr, msg.tag_id, msg.payload.val.f, msg.ts);
        }
    }
}

static void on_data_notify_timer(uv_timer_t* handle) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)handle->data;
    if (!mgr) return;

    /* 这三件都不依赖 service：订阅下行、跟踪会话代数、把 ring 抽干。
     * 放在 `user_data` 门禁之前 —— 否则"服务还没接上"会连带把下行订阅和
     * 数据上报一起停掉，那是三件不相关的事。
     * （`user_data` 由 `service_new` 设置，原来被当成"整个北向能不能跑"的开关，
     *  但它实际只表示"service 已接管"。） */
    _ensure_downlink_subscribed(mgr);
    _sync_session_epoch(mgr);
    _drain_source(mgr);

    if (!mgr->user_data) return;

    bool is_online = _any_connected(mgr);
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
                char address[256];
                _render_address(mgr, "NBIRTH", NULL, address, sizeof(address));
                _publish_all(mgr, address, tick_buf->b.data, tick_buf->b.len);
                net_buf_unref(tick_buf);
            }
        }
    }

    _drain_source(mgr);
}

/**
 * @brief 数据到达唤醒（由 source 的 subscribe_notify 触发，任意线程）。
 *
 * 只做 `uv_async_send` —— 这是 uv 里唯一明确线程安全的唤醒手段。
 * 实际取数在 loop 线程的 `_on_data_async` 里做。
 */
static void _nb_wake(void* user) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)user;
    if (!mgr || !mgr->loop) return;
    uv_async_send(&mgr->data_async);
}

static void _on_data_async(uv_async_t* handle) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)handle->data;
    if (!mgr) return;
    _drain_source(mgr);
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

/* ==============================================================================
 * 北向协议注册表（ADR-0011：换协议不改这个文件）
 *
 * 加一个新协议 = 在这里加一行 + 写一个驱动。管理器本身不动。
 * ========================================================================== */
typedef struct {
    const char* name;
    int (*reg)(driver_manager_t* mgr);
    /* 该协议缺省的地址模板。换协议时地址规则必须跟着换 ——
     * Sparkplug 的主题里要 kind，JSON 的 topic 树完全不同。 */
    const char* default_address;
} nb_protocol_entry_t;

static const nb_protocol_entry_t NB_PROTOCOLS[] = {
    {"SparkplugB", sparkplug_b_driver_register, "spBv1.0/{group}/{kind}/{node}"},
    /* 第二个协议是**对照样本**：地址规则、载荷格式、BIRTH 语义都与 Sparkplug 不同，
     * 用来证明换协议不需要改编排层。只有一个协议时"可插拔"没法验证。 */
    {"MqttJson", mqtt_json_driver_register, "edge/{node}/{kind}"},
};

static const nb_protocol_entry_t* _find_protocol(const char* name) {
    if (!name) return NULL;
    for (size_t i = 0; i < sizeof(NB_PROTOCOLS) / sizeof(NB_PROTOCOLS[0]); i++) {
        if (strcmp(NB_PROTOCOLS[i].name, name) == 0) return &NB_PROTOCOLS[i];
    }
    return NULL;
}

/**
 * @brief 建驱动实例。
 *
 * 放在线程入口而不是 `init_ex`，是为了让 `set_protocol` 能在 `start` 之前随时调用 ——
 * 否则协议名只能在 init 那一刻定死，又变回硬编码。
 */
static int _create_nb_driver(northbound_mgr_t* mgr) {
    const nb_protocol_entry_t* p = _find_protocol(mgr->protocol_name);
    if (!p) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: unknown protocol '%s'", mgr->protocol_name);
        return -1;
    }

    mgr->drv_mgr = driver_manager_create();
    if (!mgr->drv_mgr) return -1;

    if (p->reg(mgr->drv_mgr) != 0) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: protocol '%s' failed to register", p->name);
        driver_manager_destroy(mgr->drv_mgr);
        mgr->drv_mgr = NULL;
        return -1;
    }

    mgr->nb_driver = driver_instance_create(mgr->drv_mgr, p->name, 1);
    if (!mgr->nb_driver) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: cannot create driver instance '%s'", p->name);
        driver_manager_destroy(mgr->drv_mgr);
        mgr->drv_mgr = NULL;
        return -1;
    }

    mgr->nb_driver->pool = net_buf_pool_new(32, 1024, 0);
    return 0;
}

static void north_thread_entry(void* arg) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)arg;
    if (!mgr) return;

    if (_create_nb_driver(mgr) != 0) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: no driver, thread will not run");
        return;
    }

    ((edge_driver_ctx_t*)mgr->nb_driver)->user_data = mgr->source.ctx;

    mgr->loop = &mgr->loop_storage;
    uv_loop_init(mgr->loop);

    /* 兜底轮询：数据靠 data_async 唤醒，这个 timer 只负责那些"没有事件"的事
     * —— 在线状态、会话代数、下行惰性订阅、BIRTH。
     * 从 100ms 放到 500ms：端到端延迟不再由它决定，没必要那么密。 */
    uv_timer_init(mgr->loop, &mgr->poll_timer);
    mgr->poll_timer.data = mgr;
    uv_timer_start(&mgr->poll_timer, on_data_notify_timer, 500, 500);

    uv_async_init(mgr->loop, &mgr->stop_async, on_north_stop);
    mgr->stop_async.data = mgr;

    uv_async_init(mgr->loop, &mgr->req_async, _on_req_notify);
    mgr->req_async.data = mgr;

    /* 数据到达唤醒。source 不支持就纯轮询（行为退化成老样子，但不报错）。 */
    uv_async_init(mgr->loop, &mgr->data_async, _on_data_async);
    mgr->data_async.data = mgr;
    if (mgr->source.subscribe_notify) {
        if (mgr->source.subscribe_notify(mgr->source.ctx, _nb_wake, mgr) == 0) {
            LOG_INFO(LOG_CAT_NB, "Northbound: event-driven drain enabled (poll fallback 500ms)");
        } else {
            LOG_WARN(LOG_CAT_NB, "Northbound: source notify failed, falling back to polling");
        }
    }

    // [SnF] Initialize Store-and-Forward
    if (mgr->db) {
        snf_config_t snf_cfg = {.db = mgr->db,
                                .loop = mgr->loop,
                                .publish_fn = _snf_publish_wrapper,
                                .pending_fn = _snf_pending_wrapper,
                                .user_data = mgr,
                                .sync_batch_size = SNF_MAX_BATCH,
                                .sync_interval_ms = 5000};
        mgr->snf = snf_manager_create(&snf_cfg);
    }

    uv_run(mgr->loop, UV_RUN_DEFAULT);

    if (mgr->snf) snf_manager_destroy(mgr->snf);

    /* 句柄必须先关再 uv_loop_close，否则 loop 里还有活句柄，close 会返回 EBUSY
     * 且这些 handle 内存就一直挂着。关完再跑一轮让 close 回调落地。 */
    uv_close((uv_handle_t*)&mgr->poll_timer, NULL);
    uv_close((uv_handle_t*)&mgr->stop_async, NULL);
    uv_close((uv_handle_t*)&mgr->req_async, NULL);
    uv_close((uv_handle_t*)&mgr->data_async, NULL);
    uv_run(mgr->loop, UV_RUN_DEFAULT);

    if (uv_loop_close(mgr->loop) != 0) {
        LOG_WARN(LOG_CAT_NB, "Northbound: loop still has live handles at shutdown");
    }
    mgr->loop = NULL;
}

/* ------------------------------------------------------------------------------
 * datahub 的"有新数据"通知适配
 *
 * `datahub_subscribe(hub, cb, user_data)` 只回传一个 `user_data`，而我们的
 * `subscribe_notify` 要 (函数, 上下文) 两个。用一对静态变量桥接。
 *
 * **一个进程只有一个 northbound_mgr**（DCU/IEMS/SmartBox 各自是独立进程），
 * 所以静态变量是安全的。如果将来同进程要跑多个，这里得换成按 hub 索引的表。
 * -------------------------------------------------------------------------- */
static struct {
    void (*fn)(void*);
    void* user;
} g_hub_notify;

static void _hub_on_data(datahub_t* hub, void* user_data) {
    (void)hub;
    (void)user_data;
    if (g_hub_notify.fn) g_hub_notify.fn(g_hub_notify.user);
}

static int _hub_subscribe_notify(void* ctx, void (*on_data)(void* user), void* user) {
    if (!ctx || !on_data) return -1;
    g_hub_notify.fn = on_data;
    g_hub_notify.user = user;
    datahub_subscribe((datahub_t*)ctx, _hub_on_data, NULL);
    return 0;
}

nb_source_ops_t northbound_source_from_datahub(datahub_t* hub) {
    nb_source_ops_t src = {
        .ctx = hub,
        .read_next = (int (*)(void*, uint64_t*, pipe_msg_t*))datahub_read_ring,
        .subscribe_notify = _hub_subscribe_notify,
    };
    return src;
}

int northbound_mgr_init(northbound_mgr_t* mgr, uv_loop_t* loop, datahub_t* hub,
                        const char* product_name, struct sqlite_svc_s* db) {
    (void)loop;
    nb_source_ops_t src = northbound_source_from_datahub(hub);
    nb_transport_ops_t transport = {0};
    return northbound_mgr_init_ex(mgr, src, product_name, db, transport);
}

int northbound_mgr_init_ex(northbound_mgr_t* mgr, nb_source_ops_t source, const char* product_name,
                           struct sqlite_svc_s* db, nb_transport_ops_t transport) {
    memset(mgr, 0, sizeof(*mgr));
    mgr->source = source;
    mgr->db = db;
    mgr->last_epoch = UINT32_MAX; /* 首次采样只记基线，不触发会话复位 */
    strncpy(mgr->product_name, product_name ? product_name : "", sizeof(mgr->product_name) - 1);

    /* 缺省协议与地址规则。改这两个不用改代码。 */
    snprintf(mgr->protocol_name, sizeof(mgr->protocol_name), "%s", "SparkplugB");
    snprintf(mgr->address_template, sizeof(mgr->address_template), "%s",
             NB_DEFAULT_ADDRESS_TEMPLATE);

    /* 节点身份缺省取环境变量：台架并发跑两台设备时不能撞主题。 */
    const char* g = getenv("MQTT_GROUP_ID");
    const char* n = getenv("MQTT_NODE_ID");
    snprintf(mgr->group_id, sizeof(mgr->group_id), "%s", g ? g : "");
    snprintf(mgr->node_id, sizeof(mgr->node_id), "%s", n ? n : "");

    uv__queue_init(&mgr->req_queue);
    uv_mutex_init(&mgr->req_lock);

    /* 没有 publish 的传输等于没接（evse/hems/sg 走兼容 init），不进数组，
     * 这样 _any_connected 直接返回 false，数据老实走 SnF。 */
    if (transport.publish) {
        northbound_mgr_add_transport(mgr, transport);
    }

    return 0;
}

int northbound_mgr_add_transport(northbound_mgr_t* mgr, nb_transport_ops_t transport) {
    if (!mgr || !transport.publish) return -1;
    if (mgr->transport_count >= NB_MAX_TRANSPORTS) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: too many transports (max %d)", NB_MAX_TRANSPORTS);
        return -1;
    }
    mgr->transports[mgr->transport_count++] = transport;
    LOG_INFO(LOG_CAT_NB, "Northbound: transport #%u attached (%u total)",
             mgr->transport_count, mgr->transport_count);
    return 0;
}

void northbound_mgr_set_address(northbound_mgr_t* mgr, const char* address_template,
                                const char* group_id, const char* node_id) {
    if (!mgr) return;

    if (address_template && address_template[0]) {
        snprintf(mgr->address_template, sizeof(mgr->address_template), "%s", address_template);
        mgr->address_explicit = true; /* 显式配过就不再被协议缺省覆盖 */
    }
    if (group_id && group_id[0]) {
        snprintf(mgr->group_id, sizeof(mgr->group_id), "%s", group_id);
    }
    if (node_id && node_id[0]) {
        snprintf(mgr->node_id, sizeof(mgr->node_id), "%s", node_id);
    }

    /* 渲染一次给人看：模板写错（拼错占位符）在这里就能发现，不用等主站说收不到。 */
    char sample[256];
    _render_address(mgr, "NDATA", "VOLTAGE_A", sample, sizeof(sample));
    LOG_INFO(LOG_CAT_NB, "Northbound: address template '%s' -> '%s'", mgr->address_template,
             sample);
}

int northbound_mgr_set_protocol(northbound_mgr_t* mgr, const char* protocol_name) {
    if (!mgr || !protocol_name || !protocol_name[0]) return -1;
    if (!_find_protocol(protocol_name)) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: unknown protocol '%s'", protocol_name);
        return -1;
    }
    snprintf(mgr->protocol_name, sizeof(mgr->protocol_name), "%s", protocol_name);

    /* 换协议就把地址模板一起换掉（除非调用方显式配过）。
     * 不换的话：选了 JSON 驱动、却还在往 spBv1.0/... 上发 —— 主题和载荷对不上，
     * 主站两边都解析不了，而日志里一切正常。 */
    if (!mgr->address_explicit) {
        const nb_protocol_entry_t* p = _find_protocol(protocol_name);
        if (p && p->default_address) {
            snprintf(mgr->address_template, sizeof(mgr->address_template), "%s",
                     p->default_address);
        }
    }
    return 0;
}

int northbound_mgr_render_address(northbound_mgr_t* mgr, const char* kind, const char* point,
                                  char* out, size_t out_sz) {
    if (!mgr || !out || out_sz == 0) return -1;
    _render_address(mgr, kind, point, out, out_sz);
    return 0;
}

int northbound_mgr_start(northbound_mgr_t* mgr) {
    if (!mgr || mgr->loop || mgr->thread_started) return -1;
    if (uv_thread_create(&mgr->thread, north_thread_entry, mgr) != 0) return -1;
    mgr->thread_started = true;
    return 0;
}

void northbound_mgr_stop(northbound_mgr_t* mgr) {
    if (!mgr || !mgr->thread_started) return;
    /* loop 为 NULL 说明线程已自行退出（驱动建不起来），照样要 join。 */
    if (mgr->loop) uv_async_send(&mgr->stop_async);
    uv_thread_join(&mgr->thread);
    mgr->thread_started = false;
}

void northbound_mgr_destroy(northbound_mgr_t* mgr) {
    if (!mgr) return;
    if (mgr->nb_driver) {
        if (mgr->nb_driver->pool) net_buf_pool_unref(mgr->nb_driver->pool);
        driver_instance_destroy(mgr->nb_driver, mgr->protocol_name);
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

void northbound_mgr_set_north_source(northbound_mgr_t* mgr,
                                     const struct edge_north_source_ops_s* ops, void* ctx) {
    if (!mgr) return;
    mgr->north_src = ops;
    mgr->north_src_ctx = ctx;
    /* 端口换了就重新订：新端口可能带 on_downlink，也可能没带。 */
    mgr->downlink_subscribed = false;
    LOG_INFO(LOG_CAT_NB, "Northbound: source port %s (peek=%s name=%s downlink=%s)",
             ops ? "attached" : "detached", (ops && ops->peek) ? "yes" : "no",
             (ops && ops->name) ? "yes" : "no", (ops && ops->on_downlink) ? "yes" : "no");
}

int northbound_mgr_publish_ack(northbound_mgr_t* mgr, uint32_t tag_id, const char* name, bool ok) {
    if (!mgr) return -1;

    /* 名字加后缀而不是另开一条通道：这样回执与遥测共用同一套地址模板与编码路径，
     * 换协议不用改这里（Sparkplug 编成 metric、裸 JSON 编成 {p,v}）。 */
    char ack_name[96];
    if (name && name[0]) {
        snprintf(ack_name, sizeof(ack_name), "%s.ack", name);
    } else {
        snprintf(ack_name, sizeof(ack_name), "ack.%u", tag_id);
    }
    return _push_to_transport_direct_named(mgr, tag_id, ok ? 1.0 : 0.0, uv_hrtime(), ack_name);
}
