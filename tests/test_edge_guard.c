#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "edge_guard.h"
#include "contract_registry.h"
#include "arbiter.h"

// Mock 契约元数据
static const edge_tag_meta_t mock_meta = {
    .id = 5001,
    .name = "CRITICAL_VALVE_CONTROL",
    .min = 0.0,
    .max = 100.0,
    .unit = "%"
};

// 模拟 contract_tag_get_meta
const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t id) {
    if (id == 5001) return &mock_meta;
    return NULL;
}

// 模拟 mbedtls_ops (由于我们没有链接真实 mbedtls，此处使用 Mock)
#include "hal_crypto.h"
static int mock_hmac(crypto_hash_algo_t algo, const crypto_key_t *key, const uint8_t *input,
                     size_t ilen, uint8_t *output) {
    (void)algo; (void)key; (void)input; (void)ilen;
    memset(output, 0xAA, 32); // 返回固定 Fake 签名
    return 0;
}

static crypto_ops_t mbedtls_ops = {
    .name = "mock_crypto",
    .hmac_calculate = mock_hmac
};

static void test_dpi_range_protection(void **state) {
    (void)state;
    dpi_engine_init(NULL);

    // 1. 合法范围内写入 (50%)
    cmd_msg_t *cmd_ok = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmd_ok, 5001, 50.0, ARBITER_PRIO_NORMAL);
    assert_int_equal(dpi_validate_command(cmd_ok), DPI_PASS);
    free(cmd_ok);

    // 2. 超出上限写入 (150%)
    cmd_msg_t *cmd_bad = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmd_bad, 5001, 150.0, ARBITER_PRIO_NORMAL);
    assert_int_equal(dpi_validate_command(cmd_bad), DPI_BLOCK_RANGE);
    free(cmd_bad);
}

static void test_dpi_rate_of_change(void **state) {
    (void)state;
    dpi_engine_init(NULL);

    // 第一次写入 10%
    assert_int_equal(dpi_validate_telemetry(5001, 10.0), DPI_PASS);
    
    // 我们需要模拟时间流逝。由于 dpi_validate_telemetry 内部使用 uv_now()，
    // 我们无法直接控制它。但我们可以等待 10ms。
    // 但是 10ms 内跳变 80% 依然是巨大的变化率 (8000%/s)
    usleep(10000); 
    
    // 立即写入 90% (瞬间跳变 80%，超过 50%/s 的默认逻辑阈值)
    assert_int_equal(dpi_validate_telemetry(5001, 90.0), DPI_BLOCK_RATE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dpi_range_protection),
        cmocka_unit_test(test_dpi_rate_of_change),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
