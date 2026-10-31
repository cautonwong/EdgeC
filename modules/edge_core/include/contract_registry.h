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
 * @brief 根据 TagID 获取元数据
 */
const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t id);

/**
 * @brief 获取 API 路由表
 */
const api_route_t *contract_get_api_routes(uint32_t *count);

// 💡 [ADR-0002] 从二进制 Blob 加载契约实例
contract_instance_t *contract_instance_load(const char *path);

/**
 * @brief [ADR-0002] 激活契约实例（将其设为当前生效）
 */
int contract_instance_activate(contract_instance_t *inst);

/**
 * @brief [ADR-0002] 销毁契约实例
 */
void contract_instance_destroy(contract_instance_t *inst);

/**
 * @brief [ADR-0002] 获取当前激活的契约实例
 */
contract_instance_t *contract_instance_get_active(void);

// --- [ADR-0002] Pipeline 热重载接口 ---

/** Pipeline 重载回调：contract 激活时触发 */
typedef void (*contract_on_reload_fn)(contract_instance_t *inst, void *user_data);
void contract_set_reload_callback(contract_on_reload_fn fn, void *user_data);

/**
 * @brief [ADR-0002] 准备新 Pipeline（后台构建，不切换）
 * @return 0=成功, <0=错误
 */
int contract_pipeline_prepare(contract_instance_t *inst);

/**
 * @brief [ADR-0002] 原子切换到新 Pipeline
 * @return 0=成功, <0=错误
 */
int contract_pipeline_switch(contract_instance_t *inst);

/**
 * @brief [ADR-0002] 准备+切换一步到位
 */
int contract_pipeline_reload(contract_instance_t *inst);

#endif
