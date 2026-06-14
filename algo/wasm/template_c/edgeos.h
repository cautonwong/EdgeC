#ifndef WASM_EDGEOS_H
#define WASM_EDGEOS_H

#include <stdint.h>

/**
 * @brief EdgeOS WASM ABI - 极致兼容版
 *
 * 我们将指针显式定义为 uint32_t 或使用标准指针，
 * 但在宿主侧统一按 i32 (uint32_t) 处理。
 */

__attribute__((import_module("env"), import_name("ext_datahub_read"))) extern int ext_datahub_read(
    uint32_t tag_id, double* out_val);

__attribute__((import_module("env"), import_name("ext_datahub_publish"))) extern int
ext_datahub_publish(uint32_t tag_id, double value);

__attribute__((import_module("env"), import_name("ext_log"))) extern void ext_log(int level,
                                                                                  const char* msg);

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

#endif
