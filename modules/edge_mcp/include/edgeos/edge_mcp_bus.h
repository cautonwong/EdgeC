/**
 * @file edge_mcp_bus.h
 * @brief 工业级共享 MCP (Model Context Protocol) 神经总线架构 (ARCH-MCP-02 / Issue #297)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 *
 * 核心机制：
 * 1. 分布式 Provider 动态挂载与命名空间隔离 (mcp/registerProvider, mcp/unregisterProvider)
 * 2. 四级特权管控 (L0~L3) 与电气五防安全联锁仲裁门禁 (DL/T 687 / GB/T 14048)
 * 3. UDS 反向 RPC 路由分发器与多客户端高并发隔离
 * 4. 快车道 (Fast Track) DataHub 共享内存 (SHM) 零拷贝穿透支持
 * 5. 纳秒级 MTT 工业审计黑匣子与操作留痕追溯
 */

#ifndef EDGE_MCP_BUS_H
#define EDGE_MCP_BUS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <cjson/cJSON.h>
#include <uv.h>

#include "edgeos/edge_mcp_dispatcher.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCP_BUS_MAX_PROVIDERS 32
#define MCP_BUS_MAX_TOOLS_PER_PROVIDER 32
#define MCP_BUS_MAX_AUDIT_LOGS 128
#define MCP_BUS_MAX_NAME_LEN 64
#define MCP_BUS_MAX_DESC_LEN 256

/**
 * @brief MCP 工业四级安全特权定义
 */
typedef enum {
    MCP_SEC_L0_SAFE_READ = 0,        /**< L0: 只读探针与算法评估 (无风险，立即放行) */
    MCP_SEC_L1_PARAM_TUNING,         /**< L1: 控制参数与阈值微调 (参数上下限与工程量边界核验) */
    MCP_SEC_L2_CRITICAL_ACTION,      /**< L2: 物理开关动作、充放电控制 (强制五防联锁预演与防误动校验) */
    MCP_SEC_L3_SYSTEM_CRITICAL       /**< L3: 固件升级/分区切换/出厂重置 (双人复核或带外高权限密钥) */
} mcp_security_level_t;

/**
 * @brief 电气“五防”联锁规则枚举 (DL/T 687)
 */
typedef enum {
    MCP_INTERLOCK_RULE_NONE = 0,
    MCP_INTERLOCK_RULE_PREVENT_LOAD_DISCONNECT, /**< 1. 防止带负荷分/合隔离开关 (刀闸) */
    MCP_INTERLOCK_RULE_PREVENT_LIVE_EARTHING,    /**< 2. 防止带电挂接地线/合接地刀闸 */
    MCP_INTERLOCK_RULE_PREVENT_EARTHED_CLOSING,  /**< 3. 防止带接地线/接地刀闸合断路器 (合闸) */
    MCP_INTERLOCK_RULE_PREVENT_MISCLOSE_BREAKER, /**< 4. 防止误分/误合断路器 */
    MCP_INTERLOCK_RULE_PREVENT_LIVE_ENTRY        /**< 5. 防止误入带电间隔 */
} mcp_interlock_rule_t;

/**
 * @brief MCP 安全调用上下文凭证
 */
typedef struct {
    const char *client_id;           /**< 调用者客户端标识 (e.g. "copilot_web", "claude_agent") */
    const char *operator_id;         /**< 操作员标识 */
    const char *supervisor_token;    /**< 监护人/复核员授权 Token (L3 必填，L2 选填) */
    mcp_security_level_t role_level; /**< 客户端授权最高特权等级 */
    uint64_t request_timestamp_ns;   /**< 请求时间戳 (MTT 纳秒) */
} mcp_security_context_t;

/**
 * @brief Provider 挂载的工具元数据
 */
typedef struct {
    char name[MCP_BUS_MAX_NAME_LEN];
    char description[MCP_BUS_MAX_DESC_LEN];
    char input_schema_json[512];
    mcp_security_level_t security_level;
    mcp_interlock_rule_t interlock_rule;
    edge_mcp_tool_handler_t local_handler; /**< 本地进程直接处理函数 (非 NULL 时优先本地调用) */
} mcp_provider_tool_t;

/**
 * @brief 分布式 Provider 注册描述结构
 */
typedef struct {
    char provider_id[MCP_BUS_MAX_NAME_LEN];  /**< Provider 唯一服务标识 (e.g. "smartbox_service") */
    int pid;                                 /**< 宿主进程 PID */
    char security_domain[MCP_BUS_MAX_NAME_LEN]; /**< 所属安全域 (e.g. "grid_distribution", "ess_storage") */
    mcp_provider_tool_t tools[MCP_BUS_MAX_TOOLS_PER_PROVIDER]; /**< 挂载工具集合 */
    size_t tool_count;                       /**< 工具数量 */
    int client_fd;                           /**< UDS 客户端套接字 FD (-1 表示进程内本地 Provider) */
    uint64_t registered_at_ns;               /**< 注册时间戳 */
} mcp_provider_desc_t;

/**
 * @brief 工业审计黑匣子日志条目
 */
typedef struct {
    uint64_t timestamp_ns;                   /**< MTT 纳秒时间戳 */
    char client_id[MCP_BUS_MAX_NAME_LEN];    /**< 客户端标识 */
    char operator_id[MCP_BUS_MAX_NAME_LEN];  /**< 操作人 */
    char tool_name[MCP_BUS_MAX_NAME_LEN];    /**< 执行工具 */
    mcp_security_level_t sec_level;          /**< 特权等级 */
    bool is_authorized;                      /**< 是否通过安全仲裁放行 */
    char block_reason[128];                  /**< 阻断原因 (五防冲突或特权不足) */
    int exec_return_code;                    /**< 工具执行返回状态码 */
    uint32_t duration_us;                    /**< 路由与执行耗时 (微秒) */
} mcp_audit_log_entry_t;

typedef struct mcp_bus_hub_s mcp_bus_hub_t;

/**
 * @brief 创建并初始化共享 MCP 工业神经总线 Hub
 * @param env 依赖环境上下文
 * @return 总线 Hub 实例句柄
 */
mcp_bus_hub_t *mcp_bus_hub_create(const edge_mcp_env_t *env);

/**
 * @brief 销毁 MCP 工业神经总线 Hub
 * @param hub 总线 Hub 句柄
 */
void mcp_bus_hub_destroy(mcp_bus_hub_t *hub);

/**
 * @brief 获取底层 MCP Dispatcher 句柄
 * @param hub 总线 Hub 句柄
 * @return Dispatcher 句柄
 */
edge_mcp_dispatcher_t *mcp_bus_get_dispatcher(mcp_bus_hub_t *hub);

/**
 * @brief 动态注册/挂载分布式 Provider
 * @param hub 总线 Hub 句柄
 * @param desc Provider 描述结构
 * @return 0 成功, 负数 错误码
 */
int mcp_bus_register_provider(mcp_bus_hub_t *hub, const mcp_provider_desc_t *desc);

/**
 * @brief 动态下架/注销分布式 Provider
 * @param hub 总线 Hub 句柄
 * @param provider_id Provider 标识
 * @return 0 成功, 负数 错误码
 */
int mcp_bus_unregister_provider(mcp_bus_hub_t *hub, const char *provider_id);

/**
 * @brief 根据 Socket FD 注销 Provider (用于连接断开时自动清理)
 * @param hub 总线 Hub 句柄
 * @param client_fd 客户端套接字 FD
 * @return 注销的 Provider 数量
 */
int mcp_bus_unregister_by_fd(mcp_bus_hub_t *hub, int client_fd);

/**
 * @brief 执行工业安全门禁与五防联锁预演仲裁
 * @param hub 总线 Hub 句柄
 * @param tool_name 目标调用工具名称
 * @param params 输入参数 JSON 对象
 * @param sec_ctx 调用者凭证上下文
 * @param out_block_reason 输出阻断原因缓存 (至少 128 字节)
 * @param reason_len 缓存长度
 * @return 0 允许放行, 负数 阻断拦截错误码 (-32001 安全或五防违规, -32002 权限不足)
 */
int mcp_bus_security_evaluate(mcp_bus_hub_t *hub,
                              const char *tool_name,
                              const cJSON *params,
                              const mcp_security_context_t *sec_ctx,
                              char *out_block_reason,
                              size_t reason_len);

/**
 * @brief 携带安全凭证上下文的 JSON-RPC 2.0 请求总线分发
 * @param hub 总线 Hub 句柄
 * @param json_req JSON-RPC 请求字符串
 * @param sec_ctx 安全凭证上下文 (传 NULL 时默认以 L0 访客身份核验)
 * @return 分发响应 JSON 字符串 (调用方需 free 释放)
 */
char *mcp_bus_dispatch_with_security(mcp_bus_hub_t *hub,
                                     const char *json_req,
                                     const mcp_security_context_t *sec_ctx);

/**
 * @brief 查询最近的工业审计黑匣子流水
 * @param hub 总线 Hub 句柄
 * @param limit 最大返回条目数
 * @param out_logs 输出 cJSON 数组
 * @return 审计条目总数
 */
int mcp_bus_get_audit_logs(mcp_bus_hub_t *hub, size_t limit, cJSON **out_logs);

/**
 * @brief 获取当前全局已挂载 Provider 列表
 * @param hub 总线 Hub 句柄
 * @param out_providers 输出 cJSON 数组
 * @return Provider 总数
 */
int mcp_bus_get_providers_json(mcp_bus_hub_t *hub, cJSON **out_providers);

/**
 * @brief 安全等级转字符串
 */
const char *mcp_security_level_to_str(mcp_security_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_MCP_BUS_H */
