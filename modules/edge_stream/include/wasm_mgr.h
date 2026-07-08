#ifndef EDGE_WASM_MGR_H
#define EDGE_WASM_MGR_H

#include <stdint.h>
#include "datahub.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wasm_app_s wasm_app_t;

/**
 * @brief 初始化全局 WASM 运行时
 */
int wasm_mgr_init(datahub_t *hub);

/**
 * @brief 清理全局 WASM 运行时
 */
void wasm_mgr_cleanup(void);

/**
 * @brief 创建受限的 WASM 应用容器
 * @param wasm_path .wasm 文件路径
 * @param heap_size 堆大小 (字节)
 * @param stack_size 栈大小 (字节)
 * @param timeout_ms 执行超时时间 (0 表示不限)
 */
wasm_app_t *wasm_app_create(const char *wasm_path, uint32_t heap_size, uint32_t stack_size, uint32_t timeout_ms);

/**
 * @brief 创建 WASM 应用容器并强制校验签名
 * @param wasm_path .wasm 文件路径
 * @param heap_size 堆大小 (字节)
 * @param stack_size 栈大小 (字节)
 * @param timeout_ms 执行超时时间 (0 表示不限)
 * @param pubkey_path RSA 公钥 PEM 文件路径
 * @return wasm_app_t* 成功时返回实例, 签名无效或文件缺失时返回 NULL
 */
wasm_app_t *wasm_app_create_verify(const char *wasm_path, uint32_t heap_size, uint32_t stack_size, uint32_t timeout_ms, const char *pubkey_path);

/**
 * @brief 在容器中执行函数
 */
int wasm_app_exec(wasm_app_t *app, const char *func_name, pipe_msg_t *msg);

void wasm_app_destroy(wasm_app_t *app);

// 兼容旧接口
int wasm_mgr_exec(const char *path, const char *func_name, pipe_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif
