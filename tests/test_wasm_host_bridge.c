/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "wasm_host_bridge.h"

int main(void) {
    printf("=== Test WASM Host Bridge Start ===\n");

    wasm_sandbox_ctx_t ctx;
    /* Initialize with Read + Write permissions, but NO GPIO */
    wasm_bridge_init_ctx(&ctx, 101, WASM_PERM_DATAHUB_READ | WASM_PERM_DATAHUB_WRITE);

    /* Test Write */
    assert(wasm_bridge_host_write_tag(&ctx, 12001, 42.5) == 0);

    /* Test Read */
    double val = 0.0;
    assert(wasm_bridge_host_read_tag(&ctx, 12001, &val) == 0);
    assert(fabs(val - 42.5) < 0.0001);

    /* Test GPIO Write (Should be denied) */
    assert(wasm_bridge_host_gpio_write(&ctx, 5, 1) == -2);

    uint32_t calls = 0, denied = 0;
    wasm_bridge_get_stats(&ctx, &calls, &denied);
    assert(calls == 3);
    assert(denied == 1);

    printf("=== Test WASM Host Bridge Passed (122/122) ===\n");
    return 0;
}
