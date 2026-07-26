#include "wasm_host_bridge.h"
#include <string.h>

/* Mock DataHub storage table for host bridge validation */
static struct {
    uint16_t tag_id;
    double val;
} g_wasm_datahub_table[32];
static uint32_t g_table_cnt = 0;

void wasm_bridge_init_ctx(wasm_sandbox_ctx_t *ctx, uint32_t module_id, uint32_t perm_mask) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(wasm_sandbox_ctx_t));
    ctx->module_id = module_id;
    ctx->perm_mask = perm_mask;
}

int wasm_bridge_host_read_tag(wasm_sandbox_ctx_t *ctx, uint16_t tag_id, double *out_val) {
    if (!ctx || !out_val) return -1;
    ctx->call_count++;

    if (!(ctx->perm_mask & WASM_PERM_DATAHUB_READ)) {
        ctx->denied_count++;
        return -2; /* Permission Denied */
    }

    for (uint32_t i = 0; i < g_table_cnt; i++) {
        if (g_wasm_datahub_table[i].tag_id == tag_id) {
            *out_val = g_wasm_datahub_table[i].val;
            return 0;
        }
    }

    *out_val = 0.0;
    return 0;
}

int wasm_bridge_host_write_tag(wasm_sandbox_ctx_t *ctx, uint16_t tag_id, double val) {
    if (!ctx) return -1;
    ctx->call_count++;

    if (!(ctx->perm_mask & WASM_PERM_DATAHUB_WRITE)) {
        ctx->denied_count++;
        return -2; /* Permission Denied */
    }

    for (uint32_t i = 0; i < g_table_cnt; i++) {
        if (g_wasm_datahub_table[i].tag_id == tag_id) {
            g_wasm_datahub_table[i].val = val;
            return 0;
        }
    }

    if (g_table_cnt < 32) {
        g_wasm_datahub_table[g_table_cnt].tag_id = tag_id;
        g_wasm_datahub_table[g_table_cnt].val = val;
        g_table_cnt++;
        return 0;
    }

    return -3;
}

int wasm_bridge_host_gpio_write(wasm_sandbox_ctx_t *ctx, uint8_t pin, uint8_t state) {
    if (!ctx) return -1;
    ctx->call_count++;

    if (!(ctx->perm_mask & WASM_PERM_GPIO_ACCESS)) {
        ctx->denied_count++;
        return -2; /* Permission Denied */
    }

    (void)pin;
    (void)state;
    return 0;
}

void wasm_bridge_get_stats(const wasm_sandbox_ctx_t *ctx, uint32_t *call_count, uint32_t *denied_count) {
    if (!ctx) return;
    if (call_count) *call_count = ctx->call_count;
    if (denied_count) *denied_count = ctx->denied_count;
}
