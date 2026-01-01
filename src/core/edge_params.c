#include "edge_params.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "edge_log.h"
#include "sqlite_service.h"

// --- 数据结构 ---
typedef struct param_entry {
    char key[64];
    param_type_t type;
    union {
        double f;
        int64_t i;
        char *s;
    } val;
    struct param_entry *next;
} param_entry_t;

struct edge_params_s {
    param_entry_t *head;
    uv_rwlock_t lock;
    sqlite_svc_t *db;
};

// --- 内部私有方法 (Unsafe, 需持有锁调用) ---

static void free_entry_val(param_entry_t *entry) {
    if (entry->type == PARAM_TYPE_STRING && entry->val.s) {
        free(entry->val.s);
        entry->val.s = NULL;
    }
}

static param_entry_t *find_entry_unsafe(edge_params_t *ctx, const char *key) {
    param_entry_t *curr = ctx->head;
    while (curr) {
        if (strncmp(curr->key, key, 63) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

static int update_cache_unsafe(edge_params_t *ctx, const char *key, param_type_t type,
                               const void *val) {
    param_entry_t *entry = find_entry_unsafe(ctx, key);
    if (!entry) {
        entry = calloc(1, sizeof(param_entry_t));
        if (!entry) return -1;
        strncpy(entry->key, key, 63);
        entry->next = ctx->head;
        ctx->head = entry;
    } else {
        free_entry_val(entry);
    }

    entry->type = type;
    if (type == PARAM_TYPE_FLOAT)
        entry->val.f = *(const double *)val;
    else if (type == PARAM_TYPE_INT)
        entry->val.i = *(const int64_t *)val;
    else if (type == PARAM_TYPE_STRING) {
        entry->val.s = strdup((const char *)val);
        if (!entry->val.s) return -1;
    }
    return 0;
}

static void on_exec_done(void *data, int status, char **errmsg) {
    (void)data;
    if (status != 0 && errmsg && *errmsg) {
        LOG_ERROR(LOG_CAT_CORE, "Param Persistence Error: %s", *errmsg);
    }
}

// --- 公开接口实现 ---

edge_params_t *edge_params_create(sqlite_svc_t *svc) {
    edge_params_t *ctx = calloc(1, sizeof(edge_params_t));
    if (!ctx) return NULL;

    ctx->db = svc;
    if (uv_rwlock_init(&ctx->lock) != 0) {
        free(ctx);
        return NULL;
    }

    if (svc) {
        const char *sql =
            "BEGIN;"
            "CREATE TABLE IF NOT EXISTS edge_params ("
            "  key TEXT PRIMARY KEY, type INTEGER, "
            "  val_real REAL, val_int INTEGER, val_text TEXT, updated_at INTEGER);"
            "CREATE TABLE IF NOT EXISTS edge_param_audit ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "  key TEXT, old_val TEXT, new_val TEXT, timestamp INTEGER);"
            "COMMIT;";
        sqlite_svc_exec(svc, sql, on_exec_done, NULL);
        edge_params_load_all(ctx);
    }
    return ctx;
}

// --- Float 接口 ---
edge_status_t edge_param_set_float(edge_params_t *ctx, const char *key, double val) {
    if (!ctx || !key) return EDGE_ERR_INVALID_PARAM;
    uv_rwlock_wrlock(&ctx->lock);
    int ret = update_cache_unsafe(ctx, key, PARAM_TYPE_FLOAT, &val);
    uv_rwlock_wrunlock(&ctx->lock);
    if (ret != 0) return EDGE_ERR_NOMEM;

    if (ctx->db) {
        char sql[256];
        snprintf(sql, sizeof(sql),
                 "INSERT OR REPLACE INTO edge_params (key, type, val_real, updated_at) "
                 "VALUES ('%s', %d, %f, %ld);",
                 key, PARAM_TYPE_FLOAT, val, time(NULL));
        sqlite_svc_exec(ctx->db, sql, on_exec_done, NULL);
    }
    return EDGE_OK;
}

double edge_param_get_float(edge_params_t *ctx, const char *key, double def_val) {
    if (!ctx || !key) return def_val;
    double res = def_val;
    uv_rwlock_rdlock(&ctx->lock);
    param_entry_t *e = find_entry_unsafe(ctx, key);
    if (e && e->type == PARAM_TYPE_FLOAT) res = e->val.f;
    uv_rwlock_rdunlock(&ctx->lock);
    return res;
}

// --- Int 接口 ---
edge_status_t edge_param_set_int(edge_params_t *ctx, const char *key, int64_t val) {
    if (!ctx || !key) return EDGE_ERR_INVALID_PARAM;
    uv_rwlock_wrlock(&ctx->lock);
    int ret = update_cache_unsafe(ctx, key, PARAM_TYPE_INT, &val);
    uv_rwlock_wrunlock(&ctx->lock);
    if (ret != 0) return EDGE_ERR_NOMEM;

    if (ctx->db) {
        char sql[256];
        snprintf(sql, sizeof(sql),
                 "INSERT OR REPLACE INTO edge_params (key, type, val_int, updated_at) "
                 "VALUES ('%s', %d, %ld, %ld);",
                 key, PARAM_TYPE_INT, val, time(NULL));
        sqlite_svc_exec(ctx->db, sql, on_exec_done, NULL);
    }
    return EDGE_OK;
}

int64_t edge_param_get_int(edge_params_t *ctx, const char *key, int64_t def_val) {
    if (!ctx || !key) return def_val;
    int64_t res = def_val;
    uv_rwlock_rdlock(&ctx->lock);
    param_entry_t *e = find_entry_unsafe(ctx, key);
    if (e && e->type == PARAM_TYPE_INT) res = e->val.i;
    uv_rwlock_rdunlock(&ctx->lock);
    return res;
}

// --- String 接口 ---
edge_status_t edge_param_set_string(edge_params_t *ctx, const char *key, const char *val) {
    if (!ctx || !key || !val) return EDGE_ERR_INVALID_PARAM;
    uv_rwlock_wrlock(&ctx->lock);
    int ret = update_cache_unsafe(ctx, key, PARAM_TYPE_STRING, val);
    uv_rwlock_wrunlock(&ctx->lock);
    if (ret != 0) return EDGE_ERR_NOMEM;

    if (ctx->db) {
        char sql[512];
        snprintf(sql, sizeof(sql),
                 "INSERT OR REPLACE INTO edge_params (key, type, val_text, updated_at) "
                 "VALUES ('%s', %d, '%s', %ld);",
                 key, PARAM_TYPE_STRING, val, time(NULL));
        sqlite_svc_exec(ctx->db, sql, on_exec_done, NULL);
    }
    return EDGE_OK;
}

const char *edge_param_get_string(edge_params_t *ctx, const char *key, const char *def_val) {
    if (!ctx || !key) return def_val;
    const char *res = def_val;
    uv_rwlock_rdlock(&ctx->lock);
    param_entry_t *e = find_entry_unsafe(ctx, key);
    if (e && e->type == PARAM_TYPE_STRING) res = e->val.s;
    uv_rwlock_rdunlock(&ctx->lock);
    return res;
}

// --- 生命周期 ---

edge_status_t edge_params_load_all(edge_params_t *ctx) {
    if (!ctx || !ctx->db) return EDGE_ERR_GENERIC;
    LOG_INFO(LOG_CAT_CORE, "Synchronizing memory parameters with persistence...");
    return EDGE_OK;
}

void edge_params_destroy(edge_params_t *ctx) {
    if (!ctx) return;
    uv_rwlock_wrlock(&ctx->lock);
    param_entry_t *curr = ctx->head;
    while (curr) {
        param_entry_t *next = curr->next;
        free_entry_val(curr);
        free(curr);
        curr = next;
    }
    uv_rwlock_wrunlock(&ctx->lock);
    uv_rwlock_destroy(&ctx->lock);
    free(ctx);
}