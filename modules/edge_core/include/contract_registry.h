#ifndef CONTRACT_REGISTRY_H
#define CONTRACT_REGISTRY_H

#include "contract_data_types.h"

/**
 * @brief 注册一个全局契约
 */
int contract_register(const edge_contract_t *contract);

/**
 * @brief 获取当前生效的契约句柄
 */
const edge_contract_t *contract_get_active(void);

/**
 * @brief 根据名称查找模型注入包 (替代原有的 edge_model_find 硬编码)
 */
const driver_contract_injection_t *contract_model_find(const char *name);

/**
 * @brief 根据 TagID 获取元数据
 */
const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t id);

/**
 * @brief 获取 API 路由表
 */
const api_route_t *contract_get_api_routes(uint32_t *count);

#endif
