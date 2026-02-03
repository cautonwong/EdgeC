#ifndef WASM_MGR_H
#define WASM_MGR_H

#include <stdint.h>

#include "datahub.h"

/**
 * @brief 初始化 WASM 运行时环境
 */
int wasm_mgr_init(datahub_t *hub);

/**
 * @brief 销毁运行时环境
 */
void wasm_mgr_cleanup(void);

/**
 * @brief 加载并执行一个 WASM 模块中的特定函数
 * @param path .wasm 文件路径
 * @param func_name 函数名
 * @param msg 触发计算的原始消息
 */
int wasm_mgr_exec(const char *path, const char *func_name, pipe_msg_t *msg);

#endif
