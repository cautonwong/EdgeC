#include "driver_manager.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "datahub.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "net_buf.h"

#define MAX_DRIVERS 32
#define MAX_POLL_TARGETS 32
#define MAX_DRIVER_INSTANCES 32
#define DRIVER_PRIV_SIZE 1024
#define MAX_POLL_TAGS 64
#define MAX_DISPATCH_MSGS 64

typedef struct {
    edge_driver_ctx_t ctx;
    uint8_t priv[DRIVER_PRIV_SIZE];
    bool in_use;
} driver_inst_slot_t;

// 内部单通道轮询目标结构体
typedef struct poll_target_s {
    uint32_t device_id;
    char protocol[64];
    edge_driver_ctx_t *ctx;
    io_handle_t *io_handle;
    uint32_t tag_ids[MAX_POLL_TAGS];
    size_t tag_count;
    edge_poll_policy_t policy;
    edge_poll_stats_t stats;
    bool active;
    bool in_use;

    // Libuv 调度与超时句柄
    uv_timer_t timer;
    uv_timer_t timeout_timer;
    bool timer_init;
    bool timeout_init;
    bool pending_response;

    driver_manager_t *mgr;
} poll_target_t;

struct driver_manager_s {
    const edge_driver_t *drivers[MAX_DRIVERS];
    int count;

    // Orchestrator 运行时状态
    bool orchestrator_running;
    edge_orchestrator_cfg_t cfg;
    net_buf_pool_t *fallback_pool;
    poll_target_t targets[MAX_POLL_TARGETS];
    uint32_t target_count;
    driver_inst_slot_t inst_slots[MAX_DRIVER_INSTANCES];
    uv_mutex_t lock;
    bool lock_init;
    bool is_static;
};

_Static_assert(sizeof(struct driver_manager_s) <= sizeof(driver_manager_storage_t),
               "driver_manager_s size exceeds driver_manager_storage_t");

static driver_inst_slot_t g_fallback_inst_slots[MAX_DRIVER_INSTANCES];

__attribute__((weak)) const driver_contract_injection_t *edge_model_find(const char *name) {
    (void)name;
    return NULL;
}

static uint64_t _now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static poll_target_t *_find_target_locked(driver_manager_t *mgr, uint32_t device_id) {
    if (!mgr) return NULL;
    for (int i = 0; i < MAX_POLL_TARGETS; i++) {
        if (mgr->targets[i].in_use && mgr->targets[i].device_id == device_id) {
            return &mgr->targets[i];
        }
    }
    return NULL;
}

static net_buf_t *_alloc_buf(driver_manager_t *mgr) {
    if (mgr->cfg.io_pool) {
        return net_buf_alloc(mgr->cfg.io_pool, 0);
    }
    if (!mgr->fallback_pool) {
        mgr->fallback_pool = net_buf_pool_new(32, 1024, 0);
    }
    if (mgr->fallback_pool) {
        return net_buf_alloc(mgr->fallback_pool, 0);
    }
    return NULL;
}

static void _target_timeout_cb(uv_timer_t *handle) {
    poll_target_t *t = (poll_target_t *)handle->data;
    if (t && t->pending_response) {
        t->pending_response = false;
        if (t->stats.in_flight > 0) t->stats.in_flight--;
        t->stats.timeout_count++;
        t->stats.consecutive_fails++;
        LOG_WARN(LOG_CAT_IO, "[Orchestrator] Target %u (%s) polling response timeout (fails: %lu)",
                 t->device_id, t->protocol, (unsigned long)t->stats.consecutive_fails);

        if (t->policy.circuit_break_threshold > 0 &&
            t->stats.consecutive_fails >= t->policy.circuit_break_threshold) {
            t->stats.is_circuit_broken = true;
            LOG_ERROR(LOG_CAT_IO, "🚨 [Orchestrator] Target %u circuit breaker TRIPPED (fails: %lu >= threshold: %u)",
                      t->device_id, (unsigned long)t->stats.consecutive_fails, t->policy.circuit_break_threshold);
        }
    }
}

static int _execute_poll(poll_target_t *t) {
    if (!t || !t->active || !t->ctx || !t->ctx->driver) return -1;
    if (!t->ctx->driver->encode_request) return -1;

    // 1. 熔断保护检查 (Circuit Breaker)
    if (t->stats.is_circuit_broken) {
        LOG_WARN(LOG_CAT_IO, "[Orchestrator] Target %u is circuit-broken, skipping poll cycle", t->device_id);
        return -4;
    }

    // 2. In-flight 并发与背压控制 (Backpressure)
    uint32_t max_in_flight = t->policy.max_in_flight > 0 ? t->policy.max_in_flight : 1;
    if (t->stats.in_flight >= max_in_flight || t->pending_response) {
        t->stats.queue_depth++;
        LOG_WARN(LOG_CAT_IO, "[Orchestrator] Target %u in-flight limit reached (%u >= %u, queue_depth=%u), backpressure active",
                 t->device_id, t->stats.in_flight, max_in_flight, t->stats.queue_depth);
        return -5;
    }

    t->stats.poll_count++;
    t->stats.last_poll_ts = _now_ms();

    net_buf_t *tx_buf = _alloc_buf(t->mgr);
    if (!tx_buf) {
        t->stats.error_count++;
        t->stats.consecutive_fails++;
        LOG_ERROR(LOG_CAT_IO, "[Orchestrator] Failed to allocate tx_buf for device %u", t->device_id);
        return -2;
    }

    int rc = t->ctx->driver->encode_request(t->ctx, t->tag_ids, (int)t->tag_count, tx_buf);
    if (rc != 0) {
        net_buf_unref(tx_buf);
        t->stats.error_count++;
        t->stats.consecutive_fails++;
        LOG_WARN(LOG_CAT_IO, "[Orchestrator] Failed to encode request for device %u (rc=%d)", t->device_id, rc);
        return -3;
    }

    if (t->io_handle && t->io_handle->ops && t->io_handle->ops->write_async) {
        t->pending_response = true;
        t->stats.in_flight++;
        if (t->stats.queue_depth > 0) t->stats.queue_depth--;
        if (t->timeout_init && t->policy.timeout_ms > 0) {
            uv_timer_start(&t->timeout_timer, _target_timeout_cb, t->policy.timeout_ms, 0);
        }
        t->io_handle->ops->write_async(t->io_handle, tx_buf);
    } else {
        // 无真实传输层 (模拟/测试模式)，安全归还缓冲区
        net_buf_unref(tx_buf);
    }

    return 0;
}

static void _target_poll_timer_cb(uv_timer_t *handle) {
    poll_target_t *t = (poll_target_t *)handle->data;
    if (t && t->active) {
        _execute_poll(t);
    }
}

static void _start_target_timer(driver_manager_t *mgr, poll_target_t *t) {
    if (!mgr || !mgr->cfg.loop || !t || !t->active || !t->timer_init) return;

    uint64_t interval = t->policy.interval_ms > 0 ? t->policy.interval_ms : 1000;
    uint64_t initial_delay = interval;

    if (t->policy.mtt_wall_clock_align) {
        uint64_t now = _now_ms();
        uint64_t next_boundary = ((now / interval) + 1) * interval;
        initial_delay = next_boundary - now;
        if (initial_delay == 0) initial_delay = interval;
    }

    uv_timer_start(&t->timer, _target_poll_timer_cb, initial_delay, interval);
}

static void _target_io_event_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    poll_target_t *t = (poll_target_t *)user_data;
    if (!t || !t->mgr) return;

    if (evt == IO_EVENT_RX && payload) {
        driver_manager_feed_response(t->mgr, t->device_id, (net_buf_t *)payload);
    } else if (evt == IO_EVENT_ERROR) {
        t->stats.error_count++;
        t->stats.consecutive_fails++;
    }
}

driver_manager_t *driver_manager_create_in(driver_manager_storage_t *storage) {
    if (!storage) return NULL;
    driver_manager_t *mgr = (driver_manager_t *)storage->raw;
    memset(mgr, 0, sizeof(*mgr));
    uv_mutex_init(&mgr->lock);
    mgr->lock_init = true;
    mgr->is_static = true;
    return mgr;
}

driver_manager_t *driver_manager_create(void) {
    driver_manager_t *mgr = calloc(1, sizeof(driver_manager_t));
    if (!mgr) return NULL;
    uv_mutex_init(&mgr->lock);
    mgr->lock_init = true;
    mgr->is_static = false;
    return mgr;
}

void driver_manager_destroy(driver_manager_t *mgr) {
    if (!mgr) return;

    driver_manager_stop_auto_polling(mgr);

    for (int i = 0; i < MAX_POLL_TARGETS; i++) {
        if (mgr->targets[i].in_use) {
            if (mgr->targets[i].ctx) {
                driver_instance_destroy(mgr->targets[i].ctx, mgr->targets[i].protocol);
                mgr->targets[i].ctx = NULL;
            }
            mgr->targets[i].tag_count = 0;
            mgr->targets[i].in_use = false;
        }
    }

    if (mgr->fallback_pool) {
        net_buf_pool_destroy(mgr->fallback_pool);
        mgr->fallback_pool = NULL;
    }

    if (mgr->lock_init) {
        uv_mutex_destroy(&mgr->lock);
        mgr->lock_init = false;
    }

    if (!mgr->is_static) {
        free(mgr);
    }
}

int driver_manager_register(driver_manager_t *mgr, const edge_driver_t *driver) {
    if (!mgr || !driver) return -1;
    if (mgr->count >= MAX_DRIVERS) return -2;

    /* [ADR-002] 两套接口互斥：要么 legacy 四件套，要么 request-native。
     * 混用会让"这个驱动到底认 TagID 还是认点位"变成不可判定，
     * 迁移期最容易出的就是这种半新半旧。 */
    bool legacy = driver->encode_request || driver->decode_response || driver->handle_rx;
    bool native = driver->req_prepare || driver->req_encode || driver->req_decode;
    if (legacy && native) {
        LOG_ERROR(LOG_CAT_IO, "Driver %s mixes legacy and request-native slots (ADR-002)",
                  driver->name ? driver->name : "?");
        return -3;
    }
    if (native && (!driver->req_prepare || !driver->req_encode || !driver->req_decode)) {
        LOG_ERROR(LOG_CAT_IO, "Driver %s: request-native needs prepare+encode+decode",
                  driver->name ? driver->name : "?");
        return -4;
    }
    if (native && (driver->req_priv_size == 0 || driver->req_priv_size > REQ_DRV_PRIV_MAX)) {
        LOG_ERROR(LOG_CAT_IO, "Driver %s: req_priv_size %zu out of range (max %d)",
                  driver->name ? driver->name : "?", driver->req_priv_size, REQ_DRV_PRIV_MAX);
        return -5;
    }

    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->drivers[i]->name, driver->name) == 0) return 0;
    }

    mgr->drivers[mgr->count++] = driver;
    LOG_INFO(LOG_CAT_IO, "Registered Protocol Template: %s", driver->name);
    return 0;
}

int driver_manager_register_table(driver_manager_t *mgr, const edge_driver_reg_entry_t *registry, size_t count) {
    if (!mgr || !registry) return -1;
    int error_count = 0;
    for (size_t i = 0; i < count; i++) {
        if (!registry[i].driver) {
            LOG_ERROR(LOG_CAT_IO, "Driver registry entry [%zu] (%s) has NULL driver pointer",
                      i, registry[i].protocol_name ? registry[i].protocol_name : "UNKNOWN");
            error_count++;
            continue;
        }
        int rc = driver_manager_register(mgr, registry[i].driver);
        if (rc != 0) {
            LOG_ERROR(LOG_CAT_IO, "Failed to register driver '%s' (rc=%d)",
                      registry[i].driver->name ? registry[i].driver->name : "UNKNOWN", rc);
            error_count++;
        }
    }
    return (error_count == 0) ? 0 : -1;
}

const edge_driver_t *driver_manager_find(driver_manager_t *mgr, const char *name) {
    if (!mgr || !name) return NULL;
    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->drivers[i]->name, name) == 0) return mgr->drivers[i];
    }
    if (strcmp(name, "MODBUS") == 0 || strcmp(name, "Modbus") == 0) {
        for (int i = 0; i < mgr->count; i++) {
            if (strcmp(mgr->drivers[i]->name, "ModbusRTU") == 0) return mgr->drivers[i];
        }
    }
    return NULL;
}

static int _driver_port_read_shadow(edge_driver_ctx_t *ctx, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (!ctx || !out_msg) return -1;
    datahub_t *hub = (datahub_t *)ctx->user_data;
    if (!hub) return -1;
    return datahub_read_shadow(hub, tag_id, out_msg);
}

static int _driver_port_publish(edge_driver_ctx_t *ctx, const pipe_msg_t *msg) {
    if (!ctx || !msg) return -1;
    datahub_t *hub = (datahub_t *)ctx->user_data;
    if (!hub) return -1;
    return datahub_publish(hub, (pipe_msg_t *)msg);
}

static uint32_t _driver_port_get_tag_count(edge_driver_ctx_t *ctx) {
    if (!ctx) return 0;
    datahub_t *hub = (datahub_t *)ctx->user_data;
    if (!hub) return 0;
    return datahub_get_metadata_count(hub);
}

static const char *_driver_port_get_tag_name(edge_driver_ctx_t *ctx, uint32_t tag_id) {
    if (!ctx) return NULL;
    datahub_t *hub = (datahub_t *)ctx->user_data;
    if (!hub) return NULL;
    const edge_tag_meta_t *meta = datahub_get_metadata_by_tag(hub, tag_id);
    return meta ? meta->name : NULL;
}

edge_driver_ctx_t *driver_instance_create(driver_manager_t *mgr, const char *protocol,
                                          uint32_t device_id) {
    const edge_driver_t *tmpl = driver_manager_find(mgr, protocol);
    if (!tmpl) {
        printf("[DRV_INST_FAIL] driver_manager_find failed for '%s'! mgr=%p, count=%d\n",
               protocol, (void*)mgr, mgr ? mgr->count : -1);
        if (mgr) {
            for (int i = 0; i < mgr->count; i++) {
                printf("  registered driver[%d]: '%s'\n", i, mgr->drivers[i] ? mgr->drivers[i]->name : "null");
            }
        }
        return NULL;
    }

    driver_inst_slot_t *slot = NULL;
    if (mgr) {
        for (int i = 0; i < MAX_DRIVER_INSTANCES; i++) {
            if (!mgr->inst_slots[i].in_use) {
                slot = &mgr->inst_slots[i];
                break;
            }
        }
    }
    if (!slot) {
        for (int i = 0; i < MAX_DRIVER_INSTANCES; i++) {
            if (!g_fallback_inst_slots[i].in_use) {
                slot = &g_fallback_inst_slots[i];
                break;
            }
        }
    }
    if (!slot) return NULL;

    memset(slot, 0, sizeof(*slot));
    slot->in_use = true;
    slot->ctx.driver = tmpl;
    slot->ctx.device_id = device_id;
    slot->ctx.read_shadow = _driver_port_read_shadow;
    slot->ctx.publish = _driver_port_publish;
    slot->ctx.get_tag_count = _driver_port_get_tag_count;
    slot->ctx.get_tag_name = _driver_port_get_tag_name;

    const driver_contract_injection_t *inj = edge_model_find(protocol);
    if (inj) {
        slot->ctx.lookup_by_key = inj->lookup_by_key;
        slot->ctx.lookup_by_tag = inj->lookup_by_tag;
        slot->ctx.meta_list = inj->meta_list;
        slot->ctx.meta_list_len = inj->meta_list_len;
    }

    if (mgr && mgr->cfg.hub) {
        slot->ctx.user_data = mgr->cfg.hub;
    }
    if (tmpl->ctx_size > 0) {
        if (tmpl->ctx_size > DRIVER_PRIV_SIZE) {
            slot->in_use = false;
            return NULL;
        }
        slot->ctx.priv = slot->priv;
        memset(slot->priv, 0, DRIVER_PRIV_SIZE);
    }
    if (tmpl->on_init) tmpl->on_init(&slot->ctx);

    return &slot->ctx;
}

void driver_instance_destroy(edge_driver_ctx_t *ctx, const char *protocol) {
    (void)protocol;
    if (!ctx) return;
    if (ctx->driver && ctx->driver->on_cleanup) ctx->driver->on_cleanup(ctx);
    driver_inst_slot_t *slot = (driver_inst_slot_t *)ctx;
    slot->in_use = false;
    ctx->priv = NULL;
}

int driver_manager_start_auto_polling(driver_manager_t *mgr, const edge_orchestrator_cfg_t *cfg) {
    if (!mgr || !cfg) return -1;

    uv_mutex_lock(&mgr->lock);
    mgr->cfg = *cfg;
    mgr->orchestrator_running = true;

    for (int i = 0; i < MAX_POLL_TARGETS; i++) {
        poll_target_t *t = &mgr->targets[i];
        if (t->in_use && t->active) {
            if (mgr->cfg.loop) {
                if (!t->timer_init) {
                    uv_timer_init(mgr->cfg.loop, &t->timer);
                    t->timer.data = t;
                    t->timer_init = true;
                }
                if (!t->timeout_init) {
                    uv_timer_init(mgr->cfg.loop, &t->timeout_timer);
                    t->timeout_timer.data = t;
                    t->timeout_init = true;
                }
                _start_target_timer(mgr, t);
            }
        }
    }

    uv_mutex_unlock(&mgr->lock);
    LOG_INFO(LOG_CAT_IO, "[Orchestrator] Southbound Auto-Polling Orchestrator Started");
    return 0;
}

int driver_manager_add_poll_target(driver_manager_t *mgr, uint32_t device_id,
                                   const char *protocol, io_handle_t *io_handle,
                                   const uint32_t *tag_ids, size_t tag_count,
                                   const edge_poll_policy_t *policy) {
    if (!mgr || !protocol) return -1;

    uv_mutex_lock(&mgr->lock);

    poll_target_t *t = _find_target_locked(mgr, device_id);
    if (!t) {
        for (int i = 0; i < MAX_POLL_TARGETS; i++) {
            if (!mgr->targets[i].in_use) {
                t = &mgr->targets[i];
                break;
            }
        }
    }

    if (!t) {
        uv_mutex_unlock(&mgr->lock);
        return -2; // 容量已满
    }

    // 清理旧资源
    if (t->in_use) {
        if (t->timer_init && !uv_is_closing((uv_handle_t *)&t->timer)) {
            uv_timer_stop(&t->timer);
        }
        if (t->timeout_init && !uv_is_closing((uv_handle_t *)&t->timeout_timer)) {
            uv_timer_stop(&t->timeout_timer);
        }
        if (t->ctx) {
            driver_instance_destroy(t->ctx, t->protocol);
            t->ctx = NULL;
        }
        t->tag_count = 0;
    }

    memset(t, 0, sizeof(poll_target_t));
    t->in_use = true;
    t->active = true;
    t->device_id = device_id;
    t->mgr = mgr;
    strncpy(t->protocol, protocol, sizeof(t->protocol) - 1);

    t->ctx = driver_instance_create(mgr, protocol, device_id);
    if (!t->ctx) {
        t->in_use = false;
        uv_mutex_unlock(&mgr->lock);
        LOG_ERROR(LOG_CAT_IO, "[Orchestrator] Failed to instantiate driver for %s", protocol);
        return -3;
    }

    if (tag_count > 0 && tag_ids) {
        size_t copy_cnt = tag_count > MAX_POLL_TAGS ? MAX_POLL_TAGS : tag_count;
        memcpy(t->tag_ids, tag_ids, copy_cnt * sizeof(uint32_t));
        t->tag_count = copy_cnt;
    }

    if (policy) {
        t->policy = *policy;
    } else {
        t->policy.interval_ms = 1000;
        t->policy.timeout_ms = 500;
        t->policy.max_retries = 3;
        t->policy.mtt_wall_clock_align = true;
    }

    t->io_handle = io_handle;
    if (t->io_handle) {
        t->io_handle->cb = _target_io_event_cb;
        t->io_handle->user_data = t;
        if (t->io_handle->ops && t->io_handle->ops->enable_read) {
            t->io_handle->ops->enable_read(t->io_handle, true);
        }
    }

    if (mgr->orchestrator_running && mgr->cfg.loop) {
        uv_timer_init(mgr->cfg.loop, &t->timer);
        t->timer.data = t;
        t->timer_init = true;

        uv_timer_init(mgr->cfg.loop, &t->timeout_timer);
        t->timeout_timer.data = t;
        t->timeout_init = true;

        _start_target_timer(mgr, t);
    }

    uv_mutex_unlock(&mgr->lock);
    LOG_INFO(LOG_CAT_IO, "[Orchestrator] Added poll target Dev=%u Proto=%s Interval=%lums",
             device_id, protocol, (unsigned long)t->policy.interval_ms);
    return 0;
}

int driver_manager_set_target_active(driver_manager_t *mgr, uint32_t device_id, bool active) {
    if (!mgr) return -1;
    uv_mutex_lock(&mgr->lock);
    poll_target_t *t = _find_target_locked(mgr, device_id);
    if (!t) {
        uv_mutex_unlock(&mgr->lock);
        return -2;
    }

    t->active = active;
    if (!active) {
        if (t->timer_init && !uv_is_closing((uv_handle_t *)&t->timer)) {
            uv_timer_stop(&t->timer);
        }
        if (t->timeout_init && !uv_is_closing((uv_handle_t *)&t->timeout_timer)) {
            uv_timer_stop(&t->timeout_timer);
        }
        t->pending_response = false;
    } else {
        if (mgr->orchestrator_running && mgr->cfg.loop) {
            _start_target_timer(mgr, t);
        }
    }

    uv_mutex_unlock(&mgr->lock);
    return 0;
}

int driver_manager_get_target_stats(driver_manager_t *mgr, uint32_t device_id, edge_poll_stats_t *stats) {
    if (!mgr || !stats) return -1;
    uv_mutex_lock(&mgr->lock);
    poll_target_t *t = _find_target_locked(mgr, device_id);
    if (!t) {
        uv_mutex_unlock(&mgr->lock);
        return -2;
    }
    *stats = t->stats;
    uv_mutex_unlock(&mgr->lock);
    return 0;
}

int driver_manager_trigger_poll_now(driver_manager_t *mgr, uint32_t device_id) {
    if (!mgr) return -1;
    uv_mutex_lock(&mgr->lock);
    poll_target_t *t = _find_target_locked(mgr, device_id);
    if (!t) {
        uv_mutex_unlock(&mgr->lock);
        return -2;
    }
    int rc = _execute_poll(t);
    uv_mutex_unlock(&mgr->lock);
    return rc;
}

int driver_manager_feed_response(driver_manager_t *mgr, uint32_t device_id, net_buf_t *rx_buf) {
    if (!mgr || !rx_buf) return -1;

    uv_mutex_lock(&mgr->lock);
    poll_target_t *t = _find_target_locked(mgr, device_id);
    if (!t || !t->ctx || !t->ctx->driver || !t->ctx->driver->decode_response) {
        uv_mutex_unlock(&mgr->lock);
        return -2;
    }

    if (t->timeout_init && !uv_is_closing((uv_handle_t *)&t->timeout_timer)) {
        uv_timer_stop(&t->timeout_timer);
    }
    if (t->pending_response) {
        t->pending_response = false;
        if (t->stats.in_flight > 0) t->stats.in_flight--;
    }

    pipe_msg_t msgs[MAX_DISPATCH_MSGS];
    int msg_count = 0;
    size_t consumed = 0;

    int rc = t->ctx->driver->decode_response(t->ctx, rx_buf, NULL, msgs, &msg_count, &consumed);
    if (rc == 0 && msg_count > 0) {
        if (mgr->cfg.hub) {
            for (int i = 0; i < msg_count; i++) {
                datahub_write_shadow(mgr->cfg.hub, &msgs[i]);
            }
        }
        t->stats.consecutive_fails = 0;
        t->stats.success_count++;
        t->stats.last_success_ts = _now_ms();
        t->stats.is_circuit_broken = false;
        uv_mutex_unlock(&mgr->lock);
        return 0;
    }

    t->stats.error_count++;
    t->stats.consecutive_fails++;
    uv_mutex_unlock(&mgr->lock);
    return -3;
}

int driver_manager_reset_circuit_breaker(driver_manager_t *mgr, uint32_t device_id) {
    if (!mgr) return -1;
    uv_mutex_lock(&mgr->lock);
    poll_target_t *t = _find_target_locked(mgr, device_id);
    if (!t) {
        uv_mutex_unlock(&mgr->lock);
        return -2;
    }
    t->stats.is_circuit_broken = false;
    t->stats.consecutive_fails = 0;
    t->stats.in_flight = 0;
    t->pending_response = false;
    t->active = true;
    if (mgr->orchestrator_running && mgr->cfg.loop) {
        _start_target_timer(mgr, t);
    }
    uv_mutex_unlock(&mgr->lock);
    LOG_INFO(LOG_CAT_IO, "[Orchestrator] Target %u circuit breaker reset and restored", device_id);
    return 0;
}

int driver_manager_stop_auto_polling(driver_manager_t *mgr) {
    if (!mgr || !mgr->orchestrator_running) return 0;

    uv_mutex_lock(&mgr->lock);
    mgr->orchestrator_running = false;

    for (int i = 0; i < MAX_POLL_TARGETS; i++) {
        poll_target_t *t = &mgr->targets[i];
        if (t->in_use) {
            if (t->timer_init && !uv_is_closing((uv_handle_t *)&t->timer)) {
                uv_timer_stop(&t->timer);
                uv_close((uv_handle_t *)&t->timer, NULL);
                t->timer_init = false;
            }
            if (t->timeout_init && !uv_is_closing((uv_handle_t *)&t->timeout_timer)) {
                uv_timer_stop(&t->timeout_timer);
                uv_close((uv_handle_t *)&t->timeout_timer, NULL);
                t->timeout_init = false;
            }
            t->pending_response = false;
        }
    }

    uv_mutex_unlock(&mgr->lock);
    LOG_INFO(LOG_CAT_IO, "[Orchestrator] Southbound Auto-Polling Orchestrator Stopped");
    return 0;
}
