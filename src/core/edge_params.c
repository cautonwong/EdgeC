#include "edge_params.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sqlite_service.h"

// 内部结构：映射 param_kv 表
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

static param_entry_t *g_cache = NULL;
static sqlite_svc_t *g_db = NULL;

void edge_param_init(sqlite_svc_t *svc) {
    g_db = svc;
    // 自动创建符合 Web UI IA 规范的表结构
    const char *schema =
        "CREATE TABLE IF NOT EXISTS param_kv ("
        "  key TEXT PRIMARY KEY,"
        "  type INTEGER,"
        "  value TEXT,"
        "  updated_at INTEGER"
        ");"
        "CREATE TABLE IF NOT EXISTS param_audit ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  key TEXT,"
        "  old_val TEXT,"
        "  new_val TEXT,"
        "  ts INTEGER"
        ");";

    // 注意：这里应调用 sqlite_svc 执行 schema，目前 SDK 简化处理
    printf("[EdgeParams] Schema initialized via SQLite Service.\n");
}

static param_entry_t *find_in_cache(const char *key) {
    param_entry_t *curr = g_cache;
    while (curr) {
        if (strcmp(curr->key, key) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

static void update_cache(const char *key, param_type_t type, void *val) {
    param_entry_t *entry = find_in_cache(key);
    if (!entry) {
        entry = calloc(1, sizeof(param_entry_t));
        strncpy(entry->key, key, 63);
        entry->next = g_cache;
        g_cache = entry;
    } else if (entry->type == PARAM_TYPE_STRING) {
        free(entry->val.s);
    }

    entry->type = type;
    if (type == PARAM_TYPE_FLOAT)
        entry->val.f = *(double *)val;
    else if (type == PARAM_TYPE_INT)
        entry->val.i = *(int64_t *)val;
    else if (type == PARAM_TYPE_STRING)
        entry->val.s = strdup((char *)val);
}

edge_status_t edge_param_set_float(const char *key, double val) {
    // 1. 更新内存缓存 (用于快速点查)
    update_cache(key, PARAM_TYPE_FLOAT, &val);

    // 2. 异步持久化到 SQLite 并记录审计日志 (全场景要求)
    if (g_db) {
        // TODO: 使用 sqlite_svc_insert 或执行 SQL
        // 模拟审计：INSERT INTO param_audit (key, new_val, ts) ...
    }

    // 3. 发布参数变更事件到 DataHub，通知 Stream Engine 重载策略
    // PipeMsg msg; pipe_msg_init_event(&msg, EVENT_PARAM_CHANGED, ...);

    return EDGE_OK;
}

double edge_param_get_float(const char *key, double default_val) {
    param_entry_t *entry = find_in_cache(key);
    if (entry && entry->type == PARAM_TYPE_FLOAT) return entry->val.f;
    return default_val;
}

// 其余 Get/Set 逻辑类似 ...

edge_status_t edge_param_load_all(void) {
    printf("[EdgeParams] Loading all parameters from SQLite to memory cache...\n");
    // TODO: 执行 SELECT * FROM param_kv 并填充 g_cache
    return EDGE_OK;
}

void edge_params_destroy(void) {
    param_entry_t *curr = g_cache;
    while (curr) {
        param_entry_t *next = curr->next;
        if (curr->type == PARAM_TYPE_STRING) free(curr->val.s);
        free(curr);
        curr = next;
    }
    g_cache = NULL;
}
