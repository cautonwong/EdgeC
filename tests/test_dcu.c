#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "bus_orchestrator.h"
#include "dcu_inventory.h"
#include "edge_log.h"
#include "edge_service.h"
#include "sched_time.h"
#include "sqlite_service.h"

typedef struct {
    uv_loop_t loop;
    service_t *svc;
    arbiter_t *arb;
    sqlite_svc_t *db;
} dcu_test_ctx_t;

static int setup(void **state) {
    dcu_test_ctx_t *ctx = calloc(1, sizeof(dcu_test_ctx_t));
    uv_loop_init(&ctx->loop);

    arbiter_init(&ctx->arb, &ctx->loop);

    // 使用非内存 DB 以避免 WAL 锁定竞争
    const char *db_path = "tdd_dcu.db";
    unlink(db_path);
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    ctx->db = sqlite_svc_create(&ctx->loop, &db_cfg);

    service_options_t opts = {.mode = "DCU_TDD"};
    ctx->svc = service_new(&opts);
    service_set_arbiter(ctx->svc, ctx->arb);
    service_set_storage(ctx->svc, ctx->db);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    datahub_init(hub, &ctx->loop, "tdd_dcu_hub", 128);
    service_set_datahub(ctx->svc, hub);

    *state = ctx;
    return 0;
}

static void on_walk_close(uv_handle_t *h, void *arg) {
    (void)arg;
    if (!uv_is_closing(h)) uv_close(h, NULL);
}

static int teardown(void **state) {
    dcu_test_ctx_t *ctx = (dcu_test_ctx_t *)*state;

    datahub_t *hub = service_get_datahub(ctx->svc);
    service_delete(ctx->svc);
    if (hub) {
        datahub_destroy(hub);
        free(hub);
    }
    arbiter_destroy(ctx->arb);
    sqlite_svc_destroy(ctx->db);

    uv_walk(&ctx->loop, on_walk_close, NULL);
    for (int i = 0; i < 50; i++) {
        if (uv_run(&ctx->loop, UV_RUN_NOWAIT) == 0) break;
        usleep(1000);
    }

    uv_loop_close(&ctx->loop);
    unlink("tdd_dcu.db");
    free(ctx);
    return 0;
}

static void test_dcu_wallclock_alignment(void **state) {
    (void)state;
    // 08:07 -> 08:15
    uint64_t next = sched_calc_next_align_ms(1767254820000ULL, 900, 0);
    assert_true(next == 1767255300000ULL);
}

static void test_dcu_inventory_persistence(void **state) {
    dcu_test_ctx_t *ctx = (dcu_test_ctx_t *)*state;

    // 1. 添加档案 (物理写入 SQLite)
    dcu_inventory_add_meter(ctx->svc, "MET_001", 0, 12345, "DLT645");

    // 2. 强制驱动 Loop 处理 SQLite 插入
    for (int i = 0; i < 10; i++) uv_run(&ctx->loop, UV_RUN_NOWAIT);

    // 3. 加载并运行
    dcu_inventory_load_and_run(ctx->svc);

    // 4. 验证任务分发 (给 Arbiter 一轮 Tick 时间)
    // Arbiter 的 timer 是 10ms，我们需要确保 loop 时间推进
    for (int i = 0; i < 10; i++) {
        uv_run(&ctx->loop, UV_RUN_ONCE);
    }

    arbiter_bus_status_t stats[1];
    arbiter_get_statuses(ctx->arb, stats, 1);
    assert_true(stats[0].queue_len > 0 || stats[0].is_busy);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_wallclock_alignment),
        cmocka_unit_test_setup_teardown(test_dcu_inventory_persistence, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
