#include "profile_loader.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

device_config_t *profile_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc((size_t)len + 1);
    if (!data) {
        fclose(f);
        return NULL;
    }
    fread(data, 1, (size_t)len, f);
    fclose(f);
    data[len] = '\0';

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) return NULL;

    device_config_t *cfg = calloc(1, sizeof(device_config_t));
    if (!cfg) {
        cJSON_Delete(root);
        return NULL;
    }

    // 填充元数据
    cJSON *name = cJSON_GetObjectItem(root, "device_name");
    if (name) strncpy(cfg->device_name, name->valuestring, 63);

    cfg->slave_id = (uint32_t)cJSON_GetObjectItem(root, "slave_id")->valueint;
    cfg->bus_id = cJSON_GetObjectItem(root, "bus_id")->valueint;

    cJSON *points = cJSON_GetObjectItem(root, "points");
    cfg->point_count = (size_t)cJSON_GetArraySize(points);
    cfg->points = calloc(cfg->point_count, sizeof(edge_point_config_t));

    for (size_t i = 0; i < cfg->point_count; i++) {
        cJSON *item = cJSON_GetArrayItem(points, (int)i);
        // 关键修复：填入新的高性能测点配置
        cfg->points[i].tag_id = (uint32_t)cJSON_GetObjectItem(item, "tag_id")->valueint;
        cfg->points[i].reg_addr = (uint32_t)cJSON_GetObjectItem(item, "addr")->valueint;
        cfg->points[i].reg_count = (uint16_t)cJSON_GetObjectItem(item, "len")->valueint;
        cfg->points[i].func_code = 0x03;
        cfg->points[i].scaler = cJSON_GetObjectItem(item, "scale")->valuedouble;
        cfg->points[i].offset = cJSON_GetObjectItem(item, "offset")->valuedouble;
    }

    cJSON_Delete(root);
    LOG_INFO(LOG_CAT_CORE, "Device Profile loaded: %s, Points: %zu", path, cfg->point_count);
    return cfg;
}

void profile_free(device_config_t *cfg) {
    if (cfg) {
        if (cfg->points) free(cfg->points);
        free(cfg);
    }
}
