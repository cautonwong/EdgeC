#ifndef WASM_HOST_BRIDGE_H
#define WASM_HOST_BRIDGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WASM_PERM_NONE           = 0,
    WASM_PERM_DATAHUB_READ   = (1 << 0),
    WASM_PERM_DATAHUB_WRITE  = (1 << 1),
    WASM_PERM_GPIO_ACCESS    = (1 << 2),
    WASM_PERM_NETWORK_ACCESS = (1 << 3)
} wasm_perm_flags_t;

typedef struct {
    uint32_t module_id;
    uint32_t perm_mask;
    uint32_t call_count;
    uint32_t denied_count;
} wasm_sandbox_ctx_t;

void wasm_bridge_init_ctx(wasm_sandbox_ctx_t *ctx, uint32_t module_id, uint32_t perm_mask);
int wasm_bridge_host_read_tag(wasm_sandbox_ctx_t *ctx, uint16_t tag_id, double *out_val);
int wasm_bridge_host_write_tag(wasm_sandbox_ctx_t *ctx, uint16_t tag_id, double val);
int wasm_bridge_host_gpio_write(wasm_sandbox_ctx_t *ctx, uint8_t pin, uint8_t state);
void wasm_bridge_get_stats(const wasm_sandbox_ctx_t *ctx, uint32_t *call_count, uint32_t *denied_count);

#ifdef __cplusplus
}
#endif

#endif /* WASM_HOST_BRIDGE_H */
