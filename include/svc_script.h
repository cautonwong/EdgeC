#ifndef SVC_SCRIPT_H
#define SVC_SCRIPT_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "shmring.h"  // For DataPoint

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCRIPT_PRIO_ALARM = 0,  // 最高优：告警检查
    SCRIPT_PRIO_ALGO = 1,   // 次优：算法计算
    SCRIPT_PRIO_PROTO = 2   // 低优：协议解析
} script_prio_t;

typedef struct script_exec_s script_executor_t;

// --- API ---

// 初始化执行器 (单线程 + Compute Loop)
script_executor_t *script_exec_create(uv_loop_t *main_loop);

// 投递任务 (主线程 -> 执行器线程)
// script_id: 对应 config/scripts/ 下的文件名 (不含.js)
// data: 输入数据 (DataPoint)
int script_exec_post(script_executor_t *exec, const char *script_id, script_prio_t prio,
                     const PipeMsg *data);

// 销毁
void script_exec_destroy(script_executor_t *exec);

#ifdef __cplusplus
}
#endif

#endif  // SVC_SCRIPT_H
