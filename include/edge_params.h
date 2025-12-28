#ifndef EDGE_PARAMS_H
#define EDGE_PARAMS_H

#include <stdint.h>

#include "edge_common.h"

// 参数类型
typedef enum {
    PARAM_TYPE_INT = 1,
    PARAM_TYPE_FLOAT = 2,
    PARAM_TYPE_STRING = 3,
    PARAM_TYPE_JSON = 4
} param_type_t;

// --- API ---

// 设置参数 (持久化并更新缓存)
edge_status_t edge_param_set_float(const char *key, double val);
edge_status_t edge_param_set_int(const char *key, int64_t val);
edge_status_t edge_param_set_string(const char *key, const char *val);

// 读取参数 (优先从内存缓存读取)
double edge_param_get_float(const char *key, double default_val);
int64_t edge_param_get_int(const char *key, int64_t default_val);
const char *edge_param_get_string(const char *key, const char *default_val);

// 批量加载 (系统启动时调用)
edge_status_t edge_param_load_all(void);

#endif
