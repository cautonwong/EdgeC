/**
 * @file edge_params.c
 * @brief 运行期参数中心实现 (高质量同步版 - 归一化支持)
 */

#include "edge_params.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "edge_log.h"

// 前向声明外部持久化服务接口 (解耦 edge_infra/sqlite_service.h)
typedef void (*sqlite_exec_cb)(void *user_data, int status, char **errmsg);
int sqlite_svc_exec(sqlite_svc_t *svc, const char *sql, sqlite_exec_cb cb, void *user_data);

// 紧凑型缓存：仅存放当前产品激活的参数
static edge_param_value_t *g_compact_cache = NULL;
static int g_compact_count = 0;

struct edge_params_s {
    sqlite_svc_t *db;
    datahub_t *hub;
    const edge_param_info_t *meta_table;
    int meta_count;
};

static void _sync_to_datahub(edge_params_t *ctx, int idx) {
    if (!ctx || !ctx->hub || !g_compact_cache || idx >= g_compact_count) return;
    const edge_param_info_t *meta = &ctx->meta_table[idx];
    pipe_msg_t msg = {0};
    msg.ts = uv_hrtime();
    msg.tag_id = meta->id;
    if (meta->type == PARAM_TYPE_FLOAT) {
        msg.type = PIPEMSG_TYPE_SCALAR;
        msg.payload.val.f = g_compact_cache[idx].f;
    } else {
        msg.type = PIPEMSG_TYPE_SCALAR;  // 统一用标量简化处理，或按需完善类型
        msg.payload.val.i = g_compact_cache[idx].i;
    }
    datahub_publish(ctx->hub, &msg);
}

edge_params_t *edge_params_create(sqlite_svc_t *db, datahub_t *hub) {
    edge_params_t *ctx = calloc(1, sizeof(struct edge_params_s));
    ctx->db = db;
    ctx->hub = hub;

    const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS system_params ("
        "  id INTEGER PRIMARY KEY,"
        "  name TEXT UNIQUE,"
        "  val_float REAL,"
        "  val_int INTEGER"
        ");";
    sqlite_svc_exec(db, create_table_sql, NULL, NULL);

    return ctx;
}

typedef struct {
    edge_param_info_t m;
    edge_param_value_t *v;
} param_load_ctx_t;

static void _on_param_load(void *user_data, int cols, char **vals) {
    param_load_ctx_t *ctx = (param_load_ctx_t *)user_data;
    if (cols >= 2 && vals[0] && vals[1]) {
        if (ctx->m.type == PARAM_TYPE_FLOAT) {
            ctx->v->f = atof(vals[0]);
        } else {
            ctx->v->i = atoll(vals[1]);
        }
    }
}

void edge_params_register_metadata(edge_params_t *ctx, const edge_param_info_t *table, int count) {
    if (!ctx || count <= 0) return;
    ctx->meta_table = table;
    ctx->meta_count = count;

    if (g_compact_cache) {
        free(g_compact_cache);
    }
    g_compact_cache = calloc((size_t)count, sizeof(edge_param_value_t));
    g_compact_count = count;

    LOG_INFO(LOG_CAT_CORE, "Registering %d parameters into compact SHM cache...", count);

    for (int i = 0; i < count; i++) {
        g_compact_cache[i] = table[i].def_val;

        char insert_sql[512];
        if (table[i].type == PARAM_TYPE_FLOAT) {
            snprintf(insert_sql, sizeof(insert_sql),
                     "INSERT OR IGNORE INTO system_params (name, val_float) VALUES ('%s', %.4f);",
                     table[i].name, table[i].def_val.f);
        } else {
            snprintf(insert_sql, sizeof(insert_sql),
                     "INSERT OR IGNORE INTO system_params (name, val_int) VALUES ('%s', %ld);",
                     table[i].name, (long)table[i].def_val.i);
        }
        sqlite_svc_exec(ctx->db, insert_sql, NULL, NULL);
    }

    // [核心修复] 从数据库回读真实值 (覆盖默认值)
    for (int i = 0; i < count; i++) {
        char select_sql[256];
        snprintf(select_sql, sizeof(select_sql),
                 "SELECT val_float, val_int FROM system_params WHERE name='%s';", table[i].name);

        param_load_ctx_t lctx = {.m = table[i], .v = &g_compact_cache[i]};
        sqlite_svc_exec(ctx->db, select_sql, _on_param_load, &lctx);

        // 同步到 DataHub Shadow
        _sync_to_datahub(ctx, i);
    }
}

float edge_params_get_float(edge_param_id_t id) {
    if (!g_compact_cache || (int)id >= g_compact_count) return 0.0f;
    return (float)g_compact_cache[id].f;
}

int64_t edge_params_get_int(edge_param_id_t id) {
    if (!g_compact_cache || (int)id >= g_compact_count) return 0;
    return g_compact_cache[id].i;
}

double edge_param_get_float(edge_params_t *ctx, const char *key, double def_val) {
    if (!ctx || !key || !g_compact_cache) return def_val;
    for (int i = 0; i < ctx->meta_count; i++) {
        if (strcmp(ctx->meta_table[i].name, key) == 0) return g_compact_cache[i].f;
    }
    return def_val;
}

edge_status_t edge_param_set_float(edge_params_t *ctx, const char *key, double val) {
    if (!ctx || !key || !g_compact_cache) return EDGE_ERR_INVALID_PARAM;
    for (int i = 0; i < ctx->meta_count; i++) {
        if (strcmp(ctx->meta_table[i].name, key) == 0) {
            if (val < ctx->meta_table[i].min || val > ctx->meta_table[i].max) {
                return EDGE_ERR_INVALID_PARAM;
            }
            g_compact_cache[i].f = val;
            char sql[256];
            snprintf(sql, sizeof(sql), "UPDATE system_params SET val_float=%.4f WHERE name='%s';",
                     val, key);
            sqlite_svc_exec(ctx->db, sql, NULL, NULL);
            _sync_to_datahub(ctx, i);
            return EDGE_OK;
        }
    }
    return EDGE_ERR_NOT_FOUND;
}

int edge_params_restore_factory(edge_params_t *ctx) {
    if (!ctx || !ctx->meta_table || !g_compact_cache) return -1;
    for (int i = 0; i < ctx->meta_count; i++) {
        g_compact_cache[i] = ctx->meta_table[i].def_val;
    }
    return 0;
}

int edge_params_restore_single(edge_params_t *ctx, edge_param_id_t id) {
    if (!ctx || (int)id >= ctx->meta_count || !g_compact_cache) return -1;
    g_compact_cache[id] = ctx->meta_table[id].def_val;
    return 0;
}

void edge_params_destroy(edge_params_t *ctx) {
    if (ctx) {
        if (g_compact_cache) {
            free(g_compact_cache);
            g_compact_cache = NULL;
            g_compact_count = 0;
        }
        free(ctx);
    }
}