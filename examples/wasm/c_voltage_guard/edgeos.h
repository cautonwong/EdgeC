#ifndef EDGEOS_H
#define EDGEOS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((import_module("env"), import_name("host_get_tag"))) extern int ext_datahub_read(
    uint32_t tag_id, double* out_val);

__attribute__((import_module("env"), import_name("host_set_tag"))) extern int ext_datahub_publish(
    uint32_t tag_id, double value);

__attribute__((import_module("env"), import_name("ext_log"))) extern void ext_log(int level,
                                                                                  const char* msg);

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3

#ifdef __cplusplus
}
#endif

#endif  // EDGEOS_H
