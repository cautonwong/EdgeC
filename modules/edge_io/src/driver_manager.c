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


struct driver_manager_s {
    const edge_driver_t *drivers[MAX_DRIVERS];
    int count;

    // Orchestrator 运行时状态
    bool orchestrator_running;
    edge_orchestrator_cfg_t cfg;
    net_buf_pool_t *fallback_pool;
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

    /* [ADR-002] 内置轮询器已删除：它是第三套轮询实现，与 dcu_poller /
     * edge_api 重复且直接调 legacy 槽位。会话与实例由各自引擎/组合根管理。 */
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
