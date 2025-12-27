#include "profile_loader.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

// 使用 libuv 的同步文件读取封装
static char *read_file_sync(const char *filename) {
    uv_fs_t req;
    uv_file fd;

    // 1. Open
    int r = uv_fs_open(NULL, &req, filename, O_RDONLY, 0, NULL);
    if (r < 0) return NULL;
    fd = (uv_file)r;
    uv_fs_req_cleanup(&req);

    // 2. Stat to get size
    r = uv_fs_fstat(NULL, &req, fd, NULL);
    if (r < 0) {
        uv_fs_close(NULL, &req, fd, NULL);
        return NULL;
    }
    uint64_t size = req.statbuf.st_size;
    uv_fs_req_cleanup(&req);

    // 3. Read
    char *data = malloc(size + 1);
    if (!data) {
        uv_fs_close(NULL, &req, fd, NULL);
        return NULL;
    }

    uv_buf_t buf = uv_buf_init(data, (unsigned int)size);
    r = uv_fs_read(NULL, &req, fd, &buf, 1, 0, NULL);
    if (r < 0) {
        free(data);
        uv_fs_close(NULL, &req, fd, NULL);
        return NULL;
    }
    data[size] = '\0';
    uv_fs_req_cleanup(&req);

    // 4. Close
    uv_fs_close(NULL, &req, fd, NULL);
    uv_fs_req_cleanup(&req);

    return data;
}

device_config_t *profile_load(const char *filename) {
    char *data = read_file_sync(filename);
    if (!data) return NULL;

    cJSON *json = cJSON_Parse(data);
    if (!json) {
        free(data);
        return NULL;
    }

    device_config_t *cfg = calloc(1, sizeof(device_config_t));
    cJSON *name = cJSON_GetObjectItem(json, "device_name");
    if (cJSON_IsString(name)) strncpy(cfg->device_name, name->valuestring, 63);

    cJSON *sid = cJSON_GetObjectItem(json, "slave_id");
    if (sid) cfg->slave_id = (uint32_t)sid->valueint;

    cJSON *bid = cJSON_GetObjectItem(json, "bus_id");
    if (bid) cfg->bus_id = bid->valueint;

    cJSON *points = cJSON_GetObjectItem(json, "points");
    cfg->point_count = (size_t)cJSON_GetArraySize(points);
    cfg->points = calloc(cfg->point_count, sizeof(edge_point_config_t));

    for (size_t i = 0; i < cfg->point_count; i++) {
        cJSON *item = cJSON_GetArrayItem(points, (int)i);
        cfg->points[i].tag_id = (uint32_t)cJSON_GetObjectItem(item, "tag_id")->valueint;
        cfg->points[i].reg_addr = (uint16_t)cJSON_GetObjectItem(item, "reg_addr")->valueint;
        cfg->points[i].reg_count = (uint16_t)cJSON_GetObjectItem(item, "reg_count")->valueint;
        cfg->points[i].scale = cJSON_GetObjectItem(item, "scale")->valuedouble;
        cfg->points[i].offset = cJSON_GetObjectItem(item, "offset")->valuedouble;
    }

    cJSON_Delete(json);
    free(data);
    return cfg;
}

void profile_free(device_config_t *cfg) {
    if (!cfg) return;
    if (cfg->points) free(cfg->points);
    free(cfg);
}