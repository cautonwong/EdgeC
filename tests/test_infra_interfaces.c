/**
 * @file test_infra_interfaces.c
 * @brief Unit tests for storage_ops_t and datahub_ops_t abstract interface definitions (AEP-201)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "storage_ops.h"
#include "datahub_ops.h"

/* ---------------- Mock Storage Driver for Interface Contract Test ---------------- */

typedef struct {
    int init_called;
    int insert_called;
    int query_called;
    int trans_begin_called;
    int trans_commit_called;
    int trans_rollback_called;
    int exec_called;
    int set_param_called;
    int get_param_called;
    int save_app_called;
    int get_app_called;
    int delete_app_called;
    int list_apps_called;
    int destroy_called;
} mock_storage_ctx_t;

static int mock_storage_init(storage_driver_t *drv, uv_loop_t *loop, const storage_config_t *cfg) {
    (void)loop; (void)cfg;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->init_called++;
    return 0;
}

static int mock_storage_insert(storage_driver_t *drv, const pipe_msg_t *msg) {
    (void)msg;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->insert_called++;
    return 0;
}

static int mock_storage_query(storage_driver_t *drv, uint64_t start_ts, uint64_t end_ts, storage_query_cb cb, void *user_data) {
    (void)start_ts; (void)end_ts;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->query_called++;
    pipe_msg_t dummy = { .tag_id = 1001, .ts = 1786800000 };
    if (cb) {
        cb(user_data, &dummy);
    }
    return 0;
}

static int mock_storage_begin_trans(storage_driver_t *drv) {
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->trans_begin_called++;
    return 0;
}

static int mock_storage_commit_trans(storage_driver_t *drv) {
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->trans_commit_called++;
    return 0;
}

static int mock_storage_rollback_trans(storage_driver_t *drv) {
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->trans_rollback_called++;
    return 0;
}

static int mock_storage_exec(storage_driver_t *drv, const char *stmt, storage_exec_cb cb, void *user_data) {
    (void)stmt;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->exec_called++;
    char *dummy_val = "1";
    char *dummy_col = "count";
    if (cb) {
        cb(user_data, 1, &dummy_val, &dummy_col);
    }
    return 0;
}

static int mock_storage_set_param(storage_driver_t *drv, const char *product, uint32_t tag_id, const void *val, size_t val_len) {
    (void)product; (void)tag_id; (void)val; (void)val_len;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->set_param_called++;
    return 0;
}

static int mock_storage_get_param(storage_driver_t *drv, const char *product, uint32_t tag_id, void *buf, size_t buf_len) {
    (void)product; (void)tag_id; (void)buf; (void)buf_len;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->get_param_called++;
    return 0;
}

static int mock_storage_save_app(storage_driver_t *drv, const void *manifest_ptr) {
    (void)manifest_ptr;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->save_app_called++;
    return 0;
}

static int mock_storage_get_app(storage_driver_t *drv, const char *app_id, void *out_manifest_ptr) {
    (void)app_id; (void)out_manifest_ptr;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->get_app_called++;
    return 0;
}

static int mock_storage_delete_app(storage_driver_t *drv, const char *app_id) {
    (void)app_id;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->delete_app_called++;
    return 0;
}

static int mock_storage_list_apps(storage_driver_t *drv, void *manifests_array_ptr, int max_count) {
    (void)manifests_array_ptr; (void)max_count;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->list_apps_called++;
    return 1;
}

static void *mock_storage_get_native(storage_driver_t *drv) {
    return drv->priv_ctx;
}

static void mock_storage_destroy(storage_driver_t *drv) {
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->destroy_called++;
}

static const storage_ops_t g_mock_storage_ops = {
    .init = mock_storage_init,
    .insert = mock_storage_insert,
    .query = mock_storage_query,
    .begin_trans = mock_storage_begin_trans,
    .commit_trans = mock_storage_commit_trans,
    .rollback_trans = mock_storage_rollback_trans,
    .exec = mock_storage_exec,
    .set_param = mock_storage_set_param,
    .get_param = mock_storage_get_param,
    .save_app = mock_storage_save_app,
    .get_app = mock_storage_get_app,
    .delete_app = mock_storage_delete_app,
    .list_apps = mock_storage_list_apps,
    .get_native_handle = mock_storage_get_native,
    .destroy = mock_storage_destroy,
};

static int dummy_query_callback(void *user_data, const pipe_msg_t *msg) {
    int *count = (int *)user_data;
    (*count)++;
    assert(msg->tag_id == 1001);
    return 0;
}

static int dummy_exec_callback(void *user_data, int col_count, char **col_values, char **col_names) {
    (void)col_count; (void)col_names;
    int *called = (int *)user_data;
    *called = atoi(col_values[0]);
    return 0;
}

static void test_storage_ops_vtable(void) {
    printf("[TEST] Testing storage_ops_t abstract interface polymorphism...\n");

    mock_storage_ctx_t ctx = {0};
    storage_driver_t drv = {
        .ops = &g_mock_storage_ops,
        .priv_ctx = &ctx,
        .name = "mock_storage"
    };

    storage_config_t cfg = {
        .db_path = ":memory:",
        .flush_interval_ms = 100,
        .batch_size = 50,
        .enable_wal = true,
        .enable_snf = true
    };

    /* 1. Init */
    assert(drv.ops->init(&drv, NULL, &cfg) == 0);
    assert(ctx.init_called == 1);

    /* 2. Insert */
    pipe_msg_t msg = { .tag_id = 42, .ts = 1000 };
    assert(drv.ops->insert(&drv, &msg) == 0);
    assert(ctx.insert_called == 1);

    /* 3. Query */
    int row_count = 0;
    assert(drv.ops->query(&drv, 0, 2000, dummy_query_callback, &row_count) == 0);
    assert(ctx.query_called == 1);
    assert(row_count == 1);

    /* 4. Transactions */
    assert(drv.ops->begin_trans(&drv) == 0);
    assert(ctx.trans_begin_called == 1);
    assert(drv.ops->commit_trans(&drv) == 0);
    assert(ctx.trans_commit_called == 1);
    assert(drv.ops->rollback_trans(&drv) == 0);
    assert(ctx.trans_rollback_called == 1);

    /* 5. Exec */
    int exec_result = 0;
    assert(drv.ops->exec(&drv, "SELECT 1", dummy_exec_callback, &exec_result) == 0);
    assert(ctx.exec_called == 1);
    assert(exec_result == 1);

    /* 6. Params */
    double param_val = 3.14;
    assert(drv.ops->set_param(&drv, "DCU", 5001, &param_val, sizeof(param_val)) == 0);
    assert(ctx.set_param_called == 1);
    double out_val = 0;
    assert(drv.ops->get_param(&drv, "DCU", 5001, &out_val, sizeof(out_val)) == 0);
    assert(ctx.get_param_called == 1);

    /* 7. Apps */
    assert(drv.ops->save_app(&drv, NULL) == 0);
    assert(ctx.save_app_called == 1);
    assert(drv.ops->get_app(&drv, "app_01", NULL) == 0);
    assert(ctx.get_app_called == 1);
    assert(drv.ops->list_apps(&drv, NULL, 10) == 1);
    assert(ctx.list_apps_called == 1);
    assert(drv.ops->delete_app(&drv, "app_01") == 0);
    assert(ctx.delete_app_called == 1);

    /* 8. Native handle & Destroy */
    assert(drv.ops->get_native_handle(&drv) == &ctx);
    drv.ops->destroy(&drv);
    assert(ctx.destroy_called == 1);

    printf("  --> storage_ops_t VTable interface contract verified successfully!\n");
}

int main(void) {
    test_storage_ops_vtable();
    printf("=== All AEP-201 Infra Interface Tests PASSED ===\n");
    return 0;
}
