#include "edge_params.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite_service.h"

// 简单的内存缓存项
typedef struct param_item {
    char key[64];
    param_type_t type;
    union {
        double f;
        int64_t i;
        char *s;
    } val;
    struct param_item *next;
} param_item_t;

static param_item_t *g_params = NULL;
static sqlite_svc_t *g_db_svc = NULL;

void edge_param_init(sqlite_svc_t *svc) {
    g_db_svc = svc;
}

static param_item_t *_find_param(const char *key) {
    param_item_t *curr = g_params;
    while (curr) {
        if (strcmp(curr->key, key) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

static void _update_cache(const char *key, param_type_t type, void *val) {
    param_item_t *item = _find_param(key);
    if (!item) {
        item = calloc(1, sizeof(param_item_t));
        strncpy(item->key, key, 63);
        item->next = g_params;
        g_params = item;
    } else if (item->type == PARAM_TYPE_STRING) {
        free(item->val.s);
    }

    item->type = type;
    if (type == PARAM_TYPE_FLOAT)
        item->val.f = *(double *)val;
    else if (type == PARAM_TYPE_INT)
        item->val.i = *(int64_t *)val;
    else if (type == PARAM_TYPE_STRING)
        item->val.s = strdup((char *)val);
}

edge_status_t edge_param_set_float(const char *key, double val) {
    _update_cache(key, PARAM_TYPE_FLOAT, &val);
    // TODO: 异步写入 SQLite sys_config 表
    return EDGE_OK;
}

edge_status_t edge_param_set_int(const char *key, int64_t val) {
    _update_cache(key, PARAM_TYPE_INT, &val);
    return EDGE_OK;
}

edge_status_t edge_param_set_string(const char *key, const char *val) {
    _update_cache(key, PARAM_TYPE_STRING, (void *)val);
    return EDGE_OK;
}

double edge_param_get_float(const char *key, double default_val) {
    param_item_t *item = _find_param(key);
    return (item && item->type == PARAM_TYPE_FLOAT) ? item->val.f : default_val;
}

int64_t edge_param_get_int(const char *key, int64_t default_val) {
    param_item_t *item = _find_param(key);
    return (item && item->type == PARAM_TYPE_INT) ? item->val.i : default_val;
}

const char *edge_param_get_string(const char *key, const char *default_val) {
    param_item_t *item = _find_param(key);
    return (item && item->type == PARAM_TYPE_STRING) ? item->val.s : default_val;
}

edge_status_t edge_param_load_all(void) {
    // TODO: 从 SQLite sys_config 表加载所有参数到内存
    return EDGE_OK;
}
