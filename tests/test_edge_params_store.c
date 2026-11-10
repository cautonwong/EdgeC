#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <uv.h>

#include "edge_params.h"
#include "edge_params_sqlite_adapter.h"
#include "sqlite_service.h"

static const edge_param_info_t k_params[] = {
    {
        .id = 0,
        .name = "voltage_a",
        .type = PARAM_TYPE_FLOAT,
        .def_val = {.f = 220.0},
        .min = 200.0,
        .max = 240.0,
    },
};

static void pump(uv_loop_t *loop) {
    for (int i = 0; i < 20; i++) uv_run(loop, UV_RUN_NOWAIT);
}

static void test_null_float_keeps_declared_default(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    sqlite_config_t cfg = {.db_path = ":memory:", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);

    edge_params_store_t store = edge_params_sqlite_store(db);
    edge_params_t *params = edge_params_create(&store, NULL);
    assert_non_null(params);
    assert_int_equal(store.insert_default_int(store.ctx, "voltage_a", 999), 0);

    edge_params_register_metadata(params, k_params, 1);
    pump(&loop);
    assert_float_equal(edge_params_get_float(0), 220.0f, 0.001);
    edge_params_destroy(params);

    sqlite_svc_destroy(db);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

static void test_params_roundtrip_through_store(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    sqlite_config_t cfg = {.db_path = ":memory:", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);

    edge_params_store_t store = edge_params_sqlite_store(db);
    edge_params_t *params = edge_params_create(&store, NULL);
    assert_non_null(params);
    edge_params_register_metadata(params, k_params, 1);
    pump(&loop);
    assert_float_equal(edge_params_get_float(0), 220.0f, 0.001);
    assert_int_equal(edge_param_set_float(params, "voltage_a", 231.0), EDGE_OK);
    pump(&loop);
    assert_float_equal(edge_params_get_float(0), 231.0f, 0.001);
    edge_params_destroy(params);

    edge_params_t *reloaded = edge_params_create(&store, NULL);
    assert_non_null(reloaded);
    edge_params_register_metadata(reloaded, k_params, 1);
    pump(&loop);
    assert_float_equal(edge_params_get_float(0), 231.0f, 0.001);
    edge_params_destroy(reloaded);

    sqlite_svc_destroy(db);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_params_roundtrip_through_store),
        cmocka_unit_test(test_null_float_keeps_declared_default),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
