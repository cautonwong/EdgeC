#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "dcu_inventory.h"
#include "edge_service.h"
#include "sqlite_service.h"

typedef struct {
    service_t *svc;
    datahub_t *hub;
    sqlite_svc_t *db;
    arbiter_t *arb;
} dcu_test_ctx_t;

static int setup(void **state) {
    dcu_test_ctx_t *ctx = calloc(1, sizeof(dcu_test_ctx_t));
    uv_loop_t *loop = uv_default_loop();

    ctx->hub = calloc(1, sizeof(datahub_t));
    datahub_init(ctx->hub, loop, "dcu_test", 128, 1024, NULL);
    arbiter_init(&ctx->arb, loop);
    sqlite_config_t db_conf = {.db_path = ":memory:", .strategy = STRATEGY_LITE};
    ctx->db = sqlite_svc_create(loop, &db_conf);

    ctx->svc = service_new(loop, ctx->hub, ctx->arb, ctx->db, NULL, NULL, NULL);
    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    dcu_test_ctx_t *ctx = *state;
    service_delete(ctx->svc);
    sqlite_svc_destroy(ctx->db);
    arbiter_destroy(ctx->arb);
    datahub_destroy(ctx->hub);
    free(ctx->hub);
    free(ctx);
    return 0;
}

static void test_dcu_inventory_load(void **state) {
    dcu_test_ctx_t *ctx = *state;
    assert_non_null(ctx->svc);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_inventory_load),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}