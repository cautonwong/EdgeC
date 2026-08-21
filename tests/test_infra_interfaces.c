/**
 * @file test_infra_interfaces.c
 * @brief Unit tests for storage_ops_t, datahub_ops_t, SQLite driver, and DDS/MQTT bus drivers (AEP-201 / AEP-202 / AEP-203)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <uv.h>

#include "storage_ops.h"
#include "datahub_ops.h"
#include "datahub_dds.h"
#include "datahub_mqtt.h"
#include "shmring.h"

extern const storage_ops_t g_sqlite_storage_ops;
extern storage_driver_t *storage_driver_sqlite_create(uv_loop_t *loop, const storage_config_t *cfg);

/* ---------------- Mock Storage Driver for Interface Contract Test ---------------- */

typedef struct {
    int init_called;
    int insert_called;
    int query_called;
    int trans_begin_called;
    int trans_commit_called;
    int trans_rollback_called;
    int exec_called;
    int checkpoint_called;
    int flush_called;
    int compact_called;
    int get_wal_size_called;
    int partition_gc_called;
    int purge_old_data_called;
    int snf_enqueue_called;
    int snf_dequeue_called;
    int snf_get_pending_called;
    int snf_flush_called;
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

static int mock_storage_insert(storage_driver_t *drv, const void *msg_ptr) {
    (void)msg_ptr;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->insert_called++;
    return 0;
}

static int mock_storage_query(storage_driver_t *drv, uint64_t start_ts, uint64_t end_ts, storage_query_cb cb, void *user_data) {
    (void)start_ts; (void)end_ts;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->query_called++;
    PipeMsg dummy = { .tag_id = 1001, .ts = 1786800000 };
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

static int mock_storage_checkpoint(storage_driver_t *drv, int mode) {
    (void)mode;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->checkpoint_called++;
    return 0;
}

static int mock_storage_flush(storage_driver_t *drv) {
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->flush_called++;
    return 0;
}

static int mock_storage_compact(storage_driver_t *drv) {
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->compact_called++;
    return 0;
}

static int mock_storage_get_wal_size(storage_driver_t *drv, uint64_t *out_bytes) {
    if (out_bytes) *out_bytes = 4096;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->get_wal_size_called++;
    return 0;
}

static int mock_storage_partition_gc(storage_driver_t *drv, uint64_t cutoff_ts) {
    (void)cutoff_ts;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->partition_gc_called++;
    return 0;
}

static int mock_storage_purge_old_data(storage_driver_t *drv, uint32_t max_size_mb) {
    (void)max_size_mb;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->purge_old_data_called++;
    return 0;
}

static int mock_storage_snf_enqueue(storage_driver_t *drv, const void *msg_ptr) {
    (void)msg_ptr;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->snf_enqueue_called++;
    return 0;
}

static int mock_storage_snf_dequeue(storage_driver_t *drv, void *out_msg_ptr) {
    (void)out_msg_ptr;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->snf_dequeue_called++;
    return 0;
}

static int mock_storage_snf_get_pending_count(storage_driver_t *drv, uint32_t *out_count) {
    if (out_count) *out_count = 0;
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->snf_get_pending_called++;
    return 0;
}

static int mock_storage_snf_flush(storage_driver_t *drv) {
    mock_storage_ctx_t *ctx = (mock_storage_ctx_t *)drv->priv_ctx;
    ctx->snf_flush_called++;
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
    .destroy = mock_storage_destroy,
    .insert = mock_storage_insert,
    .query = mock_storage_query,
    .begin_trans = mock_storage_begin_trans,
    .commit_trans = mock_storage_commit_trans,
    .rollback_trans = mock_storage_rollback_trans,
    .exec = mock_storage_exec,
    .checkpoint = mock_storage_checkpoint,
    .flush = mock_storage_flush,
    .compact = mock_storage_compact,
    .get_wal_size = mock_storage_get_wal_size,
    .partition_gc = mock_storage_partition_gc,
    .purge_old_data = mock_storage_purge_old_data,
    .snf_enqueue = mock_storage_snf_enqueue,
    .snf_dequeue = mock_storage_snf_dequeue,
    .snf_get_pending_count = mock_storage_snf_get_pending_count,
    .snf_flush = mock_storage_snf_flush,
    .set_param = mock_storage_set_param,
    .get_param = mock_storage_get_param,
    .save_app = mock_storage_save_app,
    .get_app = mock_storage_get_app,
    .delete_app = mock_storage_delete_app,
    .list_apps = mock_storage_list_apps,
    .get_native_handle = mock_storage_get_native,
};

static int dummy_query_callback(void *user_data, const void *msg_ptr) {
    const PipeMsg *msg = (const PipeMsg *)msg_ptr;
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
        .enable_snf = true,
        .max_db_size_mb = 10
    };

    /* 1. Init */
    assert(drv.ops->init(&drv, NULL, &cfg) == 0);
    assert(ctx.init_called == 1);

    /* 2. Insert */
    PipeMsg msg = { .tag_id = 42, .ts = 1000 };
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

    /* 6. WAL / Compaction / SnF Ops (notes 3) */
    assert(drv.ops->checkpoint(&drv, 1) == 0);
    assert(ctx.checkpoint_called == 1);
    assert(drv.ops->flush(&drv) == 0);
    assert(ctx.flush_called == 1);
    assert(drv.ops->compact(&drv) == 0);
    assert(ctx.compact_called == 1);
    uint64_t wal_size = 0;
    assert(drv.ops->get_wal_size(&drv, &wal_size) == 0 && wal_size == 4096);
    assert(ctx.get_wal_size_called == 1);
    assert(drv.ops->partition_gc(&drv, 500) == 0);
    assert(ctx.partition_gc_called == 1);
    assert(drv.ops->purge_old_data(&drv, 50) == 0);
    assert(ctx.purge_old_data_called == 1);
    assert(drv.ops->snf_enqueue(&drv, &msg) == 0);
    assert(ctx.snf_enqueue_called == 1);
    assert(drv.ops->snf_dequeue(&drv, &msg) == 0);
    assert(ctx.snf_dequeue_called == 1);
    uint32_t snf_pending = 0;
    assert(drv.ops->snf_get_pending_count(&drv, &snf_pending) == 0 && snf_pending == 0);
    assert(ctx.snf_get_pending_called == 1);
    assert(drv.ops->snf_flush(&drv) == 0);
    assert(ctx.snf_flush_called == 1);

    /* 7. Params */
    double param_val = 3.14;
    assert(drv.ops->set_param(&drv, "DCU", 5001, &param_val, sizeof(param_val)) == 0);
    assert(ctx.set_param_called == 1);
    double out_val = 0;
    assert(drv.ops->get_param(&drv, "DCU", 5001, &out_val, sizeof(out_val)) == 0);
    assert(ctx.get_param_called == 1);

    /* 8. Apps */
    assert(drv.ops->save_app(&drv, NULL) == 0);
    assert(ctx.save_app_called == 1);
    assert(drv.ops->get_app(&drv, "app_01", NULL) == 0);
    assert(ctx.get_app_called == 1);
    assert(drv.ops->list_apps(&drv, NULL, 10) == 1);
    assert(ctx.list_apps_called == 1);
    assert(drv.ops->delete_app(&drv, "app_01") == 0);
    assert(ctx.delete_app_called == 1);

    /* 9. Native handle & Destroy */
    assert(drv.ops->get_native_handle(&drv) == &ctx);
    drv.ops->destroy(&drv);
    assert(ctx.destroy_called == 1);

    printf("  --> storage_ops_t VTable interface contract verified successfully!\n");
}

static void test_sqlite_storage_driver_live(void) {
    printf("[TEST] Testing sqlite_storage_driver live operations...\n");

    uv_loop_t *loop = uv_default_loop();
    storage_config_t cfg = {
        .db_path = "/tmp/test_edge_infra_sqlite.db",
        .flush_interval_ms = 50,
        .batch_size = 10,
        .enable_wal = true,
        .enable_snf = true,
        .max_db_size_mb = 50
    };

    remove(cfg.db_path);

    storage_driver_t *drv = storage_driver_sqlite_create(loop, &cfg);
    assert(drv != NULL);
    assert(drv->ops == &g_sqlite_storage_ops);
    assert(strcmp(drv->name, "sqlite-wal") == 0);

    /* 1. Insert & Query */
    PipeMsg m1 = { .tag_id = 2001, .ts = 100000, .payload.val.f = 220.5 };
    assert(drv->ops->insert(drv, &m1) == 0);

    /* 2. Params */
    double set_val = 48.5;
    assert(drv->ops->set_param(drv, "IEMS", 3001, &set_val, sizeof(set_val)) == 0);
    double get_val = 0;
    assert(drv->ops->get_param(drv, "IEMS", 3001, &get_val, sizeof(get_val)) == sizeof(double));
    assert(get_val == 48.5);

    /* 3. Checkpoint & Compact */
    assert(drv->ops->checkpoint(drv, 0) == 0);
    assert(drv->ops->compact(drv) == 0);

    /* 4. WAL Size & Partition GC & Purge */
    uint64_t wal_size = 0;
    assert(drv->ops->get_wal_size(drv, &wal_size) == 0);
    assert(drv->ops->partition_gc(drv, 50000) == 0);
    assert(drv->ops->purge_old_data(drv, 50) == 0);

    /* 5. SnF operations */
    PipeMsg m2 = { .tag_id = 2002, .ts = 100050, .payload.val.f = 50.02 };
    assert(drv->ops->snf_enqueue(drv, &m2) == 0);
    uint32_t snf_pending = 0;
    assert(drv->ops->snf_get_pending_count(drv, &snf_pending) == 0);
    assert(drv->ops->snf_flush(drv) == 0);

    /* 6. Destroy */
    drv->ops->destroy(drv);
    free(drv);
    remove(cfg.db_path);

    printf("  --> sqlite_storage_driver live test verified successfully!\n");
}

static void test_datahub_bus_drivers(void) {
    printf("[TEST] Testing datahub DDS & MQTT bus drivers in edge_infra...\n");

    /* 1. DDS Driver VTable */
    assert(g_datahub_dds_ops.init != NULL);
    assert(g_datahub_dds_ops.publish != NULL);
    assert(g_datahub_dds_ops.read_shadow != NULL);
    assert(g_datahub_dds_ops.write_shadow != NULL);
    assert(g_datahub_dds_ops.destroy != NULL);

    /* 2. MQTT Driver VTable */
    assert(g_datahub_mqtt_ops.init != NULL);
    assert(g_datahub_mqtt_ops.publish != NULL);
    assert(g_datahub_mqtt_ops.read_shadow != NULL);
    assert(g_datahub_mqtt_ops.write_shadow != NULL);
    assert(g_datahub_mqtt_ops.destroy != NULL);

    /* 3. DataHub instance with DDS ops */
    datahub_t dds_hub = {0};
    dds_hub.ops = &g_datahub_dds_ops;
    datahub_config_t cfg = {0};
    assert(datahub_dds_init(&dds_hub, &cfg) == 0);
    assert(dds_hub.backend_type == DATAHUB_BACKEND_DDS);

    PipeMsg msg = { .tag_id = 9999, .ts = 5000 };
    assert(dds_hub.ops->publish(&dds_hub, &msg) == 0);
    assert(dds_hub.ops->write_shadow(&dds_hub, &msg) == 0);

    dds_hub.ops->destroy(&dds_hub);

    printf("  --> datahub DDS & MQTT bus drivers in edge_infra verified successfully!\n");
}

int main(void) {
    test_storage_ops_vtable();
    test_sqlite_storage_driver_live();
    test_datahub_bus_drivers();
    printf("=== All AEP-201 / AEP-202 / AEP-203 Infra Interface & Bus Tests PASSED ===\n");
    return 0;
}
