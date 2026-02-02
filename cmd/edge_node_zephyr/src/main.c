#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* 注册模块日志，类别对齐 EdgeOS 规范 */
LOG_MODULE_REGISTER(edge_node, LOG_LEVEL_INF);

int main(void) {
    LOG_INF("EdgeOS Zephyr Node starting...");
    LOG_INF("Board: %s", CONFIG_BOARD);

    while (1) {
        LOG_DBG("Heartbeat pulse...");

        /* 逻辑占位：未来可以从这里调用 WASM 算子或同步 DataHub */

        k_sleep(K_MSEC(1000));
    }

    return 0;
}
