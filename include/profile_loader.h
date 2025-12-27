#ifndef PROFILE_LOADER_H
#define PROFILE_LOADER_H

#include "edge_driver.h"
#include <stddef.h>

typedef struct {
    char device_name[64];
    uint32_t slave_id;
    int bus_id;
    edge_point_config_t *points;
    size_t point_count;
} device_config_t;

// 加载指定路径的 JSON 配置文件
device_config_t* profile_load(const char *filename);

// 释放资源
void profile_free(device_config_t *cfg);

#endif
