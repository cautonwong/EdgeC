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
