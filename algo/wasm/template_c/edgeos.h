#ifndef WASM_EDGEOS_H
#define WASM_EDGEOS_H

#include <stdint.h>

/**
 * @brief EdgeOS WASM ABI - 导入函数声明
 */

// 从 DataHub 读取影子值
extern int ext_datahub_read(uint32_t tag_id, double *out_val);

// 发布值到 DataHub
extern int ext_datahub_publish(uint32_t tag_id, double value);

// 日志打印
extern void ext_log(int level, const char *msg);

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

#endif
