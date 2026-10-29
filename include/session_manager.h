#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_slave_plane.h"

typedef struct session_manager_s session_manager_t;

/**
 * @brief 创建会话管理器
 * @param mgr 驱动管理器 (用于获取驱动模板)
 * @param max_sessions 最大持久化会话数
 */
session_manager_t* session_manager_create(driver_manager_t* mgr, int max_sessions);

/**
 * @brief 销毁管理器并释放所有会话
 */
void session_manager_destroy(session_manager_t* sm);

/**
 * @brief 获取或创建持久化会话
 * @param sm 管理器实例
 * @param protocol 协议名称 (如 "DLMS")
 * @param device_id 物理设备 ID
 * @return 驱动上下文指针，失败返回 NULL
 */
edge_driver_ctx_t* session_manager_get(session_manager_t* sm, const char* protocol,
                                       uint32_t device_id);

/**
 * @brief 会话心跳维护 (应由 MasterEngine 定期调用)
 * @param sm 管理器实例
 * @param bus_id 关联的总线 ID (用于日志)
 * @param out_txs 传出缓冲区数组 (由驱动生成的自发报文，如心跳)
 * @param max_txs 数组大小
 * @return 实际生成的报文数
 */
int session_manager_tick(session_manager_t* sm, int bus_id, net_buf_t** out_txs, int max_txs);

/**
 * @brief 强制关闭并回收特定会话
 */
void session_manager_close(session_manager_t* sm, const char* protocol, uint32_t device_id);

int session_manager_add_session(session_manager_t* sm, const char* protocol, uint32_t device_id,
                                const driver_contract_injection_t* inject, void* user_data);

/* [ADR-002 S3-0] 注入从站面端口（取当前值 / 收写入值）。
 * 由组合根调用；request-native 驱动的 req_slave_exchange 用它，
 * 驱动自己不认识点位模型。端口不进 edge_driver_ctx_t。 */
void session_manager_set_slave_source(session_manager_t* sm,
                                      const edge_slave_source_ops_t* ops, void* ctx);

int session_manager_handle_rx(session_manager_t* sm, net_buf_t* rx, net_buf_t* resp_out,
                              pipe_msg_t* async_msgs, int* async_count, size_t* consumed);

#endif
