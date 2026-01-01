#ifndef EDGE_PARAMS_H
#define EDGE_PARAMS_H

#include "edge_common.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

// 参数类型枚举
typedef enum { PARAM_TYPE_FLOAT = 1, PARAM_TYPE_INT = 2, PARAM_TYPE_STRING = 3 } param_type_t;

// 参数服务句柄 (Opaque Handle)
typedef struct edge_params_s edge_params_t;

/**
 * @brief 创建参数服务实例
 * @param db SQLite 服务句柄 (用于持久化)
 * @return 实例指针，失败返回 NULL
 */
edge_params_t *edge_params_create(sqlite_svc_t *db);

/**
 * @brief 销毁实例
 */
void edge_params_destroy(edge_params_t *ctx);

/**
 * @brief 从数据库加载所有参数 (预热缓存)
 */
edge_status_t edge_params_load_all(edge_params_t *ctx);

// --- Getters ---
double edge_param_get_float(edge_params_t *ctx, const char *key, double def_val);
int64_t edge_param_get_int(edge_params_t *ctx, const char *key, int64_t def_val);
const char *edge_param_get_string(edge_params_t *ctx, const char *key, const char *def_val);

// --- Setters ---
edge_status_t edge_param_set_float(edge_params_t *ctx, const char *key, double val);
edge_status_t edge_param_set_int(edge_params_t *ctx, const char *key, int64_t val);
edge_status_t edge_param_set_string(edge_params_t *ctx, const char *key, const char *val);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_PARAMS_H