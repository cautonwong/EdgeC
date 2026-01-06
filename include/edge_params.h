#ifndef EDGE_PARAMS_H
#define EDGE_PARAMS_H

#include "contract_data_types.h"
#include "edge_common.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct edge_params_s edge_params_t;

/**
 * @brief 创建参数服务实例
 * 它会自动执行“元数据同步”流程：代码定义 <-> 数据库对齐
 */
edge_params_t *edge_params_create(sqlite_svc_t *db);

/**
 * @brief 注册应用层的参数元数据 (解耦核心)
 * 通常在 main.c 启动时调用：edge_params_register_metadata(..., g_param_metadata,
 * g_param_metadata_len)
 */
void edge_params_register_metadata(edge_params_t *ctx, const edge_param_info_t *table, int count);

void edge_params_destroy(edge_params_t *ctx);

/**
 * @brief 高性能 ID 访问 (O(1) SHM 缓存)
 * 适用于控制回路、高频计算算子
 */
float edge_params_get_float(edge_param_id_t id);
int64_t edge_params_get_int(edge_param_id_t id);

// --- 运维管理接口 (String-based) ---
double edge_param_get_float(edge_params_t *ctx, const char *key, double def_val);
edge_status_t edge_param_set_float(edge_params_t *ctx, const char *key, double val);

/**
 * @brief 将所有参数恢复为 YAML 定义的默认值
 */
int edge_params_restore_factory(edge_params_t *ctx);

/**
 * @brief 将单个参数恢复为默认值
 */
int edge_params_restore_single(edge_params_t *ctx, edge_param_id_t id);

#ifdef __cplusplus
}
#endif

#endif
