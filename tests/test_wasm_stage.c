#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "stream_engine.h"

// 声明外部创建函数 (由 stage_wasm.c 提供)
extern pipeline_stage_t *stage_wasm_create(const char *wasm_path, const char *entry_func);

static void test_wasm_full_chain(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;

    // 1. 初始化 DataHub (128 槽位，10000 影子表以容纳 9999 号点位)
    int rc = datahub_init(&hub, loop, "wasm_test", 128, 10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    // 2. 创建流引擎
    stream_engine_t *se = stream_engine_create(&hub, loop, NULL, NULL);
    assert_non_null(se);

    // 3. 注册 WASM Pipeline
    // Trigger: 1001 (VOLTAGE_PHASE_A)
    pipeline_t *p = stream_engine_add_pipeline(se, "wasm_test_pipe", 1001);

    // 获取当前工作目录，并构造 WASM 文件绝对路径
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    char wasm_path[512];
    // 假设测试在项目根目录运行或从 build 运行
    // 我们尝试寻找 algo/wasm/template_c/moving_average.wasm
    snprintf(wasm_path, sizeof(wasm_path), "%s/../../algo/wasm/template_c/moving_average.wasm",
             cwd);

    // 如果上述路径不存在，尝试从根目录
    if (access(wasm_path, F_OK) != 0) {
        snprintf(wasm_path, sizeof(wasm_path), "algo/wasm/template_c/moving_average.wasm");
    }

    printf("[TEST] Loading WASM from: %s\n", wasm_path);
    pipeline_stage_t *ws = stage_wasm_create(wasm_path, "on_event");
    assert_non_null(ws);
    pipeline_add_stage(p, ws->name, ws->process, ws->priv_state, ws->cleanup_state, ws->priv_cfg,
                       ws->cleanup_cfg);
    free(ws);

    // 4. 注入触发数据 (Tag 1001 = 245.5V)
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 245.5, uv_hrtime());

    // 重要：必须先发布到 Hub，因为 WASM 是通过 ext_datahub_read (shadow table) 读取的
    datahub_publish(&hub, &msg);
    // 直接推送到引擎
    stream_engine_push(se, &msg);

    // 5. 验证结果 (Tag 9999 应该被 WASM 设为 1.0)
    pipe_msg_t result;
    rc = datahub_read_shadow(&hub, 9999, &result);
    assert_int_equal(rc, 0);
    assert_float_equal(result.payload.val.f, 1.0, 0.001);
    printf("🔥 [SUCCESS] WASM calculated: Tag 9999 = %.2f\n", result.payload.val.f);

    // 6. 再次注入低电压 (Tag 1001 = 220.0V)
    pipe_msg_init_scalar(&msg, 1001, 220.0, uv_hrtime());
    datahub_publish(&hub, &msg);
    stream_engine_push(se, &msg);

    rc = datahub_read_shadow(&hub, 9999, &result);
    assert_int_equal(rc, 0);
    assert_float_equal(result.payload.val.f, 0.0, 0.001);
    printf("🔥 [SUCCESS] WASM reset: Tag 9999 = %.2f\n", result.payload.val.f);

    // 清理
    stream_engine_destroy(se);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wasm_full_chain),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
