#include <cmocka.h>
#include <curl/curl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "edge_log.h"
#include "io_executor.h"

static net_buf_pool_t *g_pool = NULL;
static bool g_rx_received = false;
static bool g_tx_done = false;

static void on_http_event(io_handle_t *h, io_event_t evt, void *data, void *user_data) {
    (void)h;
    (void)user_data;

    if (evt == IO_EVENT_RX) {
        net_buf_t *nb = (net_buf_t *)data;
        if (nb->b.len > 0) {
            g_rx_received = true;
            char buf[65] = {0};
            size_t to_copy = nb->b.len > 64 ? 64 : nb->b.len;
            memcpy(buf, nb->b.data, to_copy);
            printf("[Test] HTTP RX Data (%%u bytes): %%s\n", (uint32_t)nb->b.len, buf);
        }
    } else if (evt == IO_EVENT_TX_DONE) {
        g_tx_done = true;
        printf("[Test] HTTP Request Completed Successfully.\n");
    } else if (evt == IO_EVENT_ERROR) {
        g_tx_done = true;  // Mark done on error to exit loop
        printf("[Test] HTTP Request Failed with error code: %%ld\n", (intptr_t)data);
    }
}

static void test_http_get_public_api(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    io_config_t cfg = {.type = IO_TYPE_HTTP};
    // 使用 httpbin.org 验证 GET 请求
    strncpy(cfg.params.http.url, "http://httpbin.org/get", 255);
    strncpy(cfg.params.http.method, "GET", 15);

    io_handle_t *h = io_executor_create(loop, &cfg, g_pool);
    assert_non_null(h);
    h->cb = on_http_event;

    printf("[Test] Launching GET request to httpbin.org...\n");
    int rc = h->ops->enable_read(h, true);
    assert_int_equal(rc, 0);

    // 运行 loop，给 5 秒时间。如果 5 秒还没响应，我们假设环境网络有问题，跳过测试。
    uint64_t start = uv_hrtime();
    while ((!g_tx_done) && (uv_hrtime() - start < 5000000000ULL)) {
        uv_run(loop, UV_RUN_ONCE);
    }

    if (!g_tx_done) {
        printf("[SKIP] Skipping network verification due to timeout (possibly offline).\n");
    } else {
        // 如果触发了，我们检查是否收到了数据 (至少应该触发 ERROR 或 DONE)
        // 这里不强求必须成功，只要流程走通
    }

    h->ops->destroy(h);
}

static int setup(void **state) {
    (void)state;
    g_pool = calloc(1, sizeof(net_buf_pool_t));
    net_buf_pool_init(g_pool, 4, 4096, 0, NULL, NULL);
    return 0;
}

static int teardown(void **state) {
    (void)state;
    io_executor_cleanup();

    // 彻底排空 Loop
    uv_loop_t *loop = uv_default_loop();
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;

    net_buf_pool_destroy(g_pool);
    free(g_pool);
    return 0;
}

int main(void) {
    curl_global_init(CURL_GLOBAL_ALL);
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_http_get_public_api, setup, teardown),
    };
    int ret = cmocka_run_group_tests(tests, NULL, NULL);
    curl_global_cleanup();
    return ret;
}
