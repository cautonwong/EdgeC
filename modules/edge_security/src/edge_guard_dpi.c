/**
 * @file edge_guard_dpi.c
 * @brief Edge-Guard DPI 校验引擎实现
 */

#include "edge_guard.h"
#include "contract_registry.h"
#include "edge_log.h"
#include "edge_os_port.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_DPI_TRACKED_TAGS 1024

typedef struct {
    uint32_t tag_id;
    double last_value;
    uint64_t last_ts;
} dpi_state_t;

struct dpi_engine_ctx_s {
    datahub_t *hub;
    dpi_state_t states[MAX_DPI_TRACKED_TAGS];
    edge_mutex_t lock;
};
typedef struct dpi_engine_ctx_s dpi_engine_ctx_t;

_Static_assert(sizeof(struct dpi_engine_ctx_s) <= sizeof(edge_guard_dpi_storage_t),
               "dpi_engine_ctx_s size exceeds edge_guard_dpi_storage_t");

static edge_guard_dpi_storage_t g_default_dpi_storage;
static dpi_engine_ctx_t *g_dpi = NULL;

int dpi_engine_init_in(edge_guard_dpi_storage_t *storage, datahub_t *hub) {
    if (!storage) return -1;
    if (g_dpi) return 0;

    g_dpi = (dpi_engine_ctx_t *)storage->raw;
    memset(g_dpi, 0, sizeof(*g_dpi));
    g_dpi->hub = hub;
    edge_mutex_init(&g_dpi->lock);

    LOG_INFO(LOG_CAT_CORE, "Edge-Guard: DPI Validation Engine active.");
    return 0;
}

int dpi_engine_init(datahub_t *hub) {
    return dpi_engine_init_in(&g_default_dpi_storage, hub);
}

dpi_result_t dpi_validate_telemetry(uint32_t tag_id, double val) {
    if (!g_dpi) return DPI_PASS;

    const edge_tag_meta_t *meta = contract_tag_get_meta(tag_id);
    if (!meta) return DPI_PASS;

    // 1. Range Check (Story 1)
    if (val < meta->min || val > meta->max) {
        dpi_audit_log(DPI_BLOCK_RANGE, tag_id, val, "Value out of contract bounds");
        return DPI_BLOCK_RANGE;
    }

    // 2. Rate of Change Check (Story 2 - Detect Logic Bombs)
    edge_mutex_lock(&g_dpi->lock);
    uint32_t idx = tag_id % MAX_DPI_TRACKED_TAGS;
    dpi_state_t *st = &g_dpi->states[idx];
    
    if (st->tag_id == tag_id && st->last_ts != 0) {
        uint64_t now = uv_hrtime() / 1000000;
        double dt = (double)(now - st->last_ts) / 1000.0;
        if (dt > 0.001) {
            double rate = fabs(val - st->last_value) / dt;
            // 硬编码变化率阈值 (未来应从契约扩展字段读取)
            // 假设：单秒变化不得超过量程的 50%
            double max_rate = (meta->max - meta->min) * 0.5;
            if (max_rate > 0 && rate > max_rate) {
                edge_mutex_unlock(&g_dpi->lock);
                dpi_audit_log(DPI_BLOCK_RATE, tag_id, val, "Unphysical rate of change detected");
                return DPI_BLOCK_RATE;
            }
        }
    }
    
    st->tag_id = tag_id;
    st->last_value = val;
    st->last_ts = uv_hrtime() / 1000000;
    edge_mutex_unlock(&g_dpi->lock);

    return DPI_PASS;
}

dpi_result_t dpi_validate_command(cmd_msg_t *cmd) {
    if (!cmd) return DPI_PASS;

    // 目前仅校验写操作
    if (cmd->action == CMD_ACTION_WRITE) {
        return dpi_validate_telemetry(cmd->tag_id, cmd->value);
    }
    
    return DPI_PASS;
}

extern void dpi_audit_signed_log(dpi_result_t res, uint32_t tag_id, double val, const char *reason);

void dpi_audit_log(dpi_result_t res, uint32_t tag_id, double val, const char *reason) {
    if (res == DPI_PASS) return;

    // 生产级要求：加密签名审计日志
    dpi_audit_signed_log(res, tag_id, val, reason);
}
