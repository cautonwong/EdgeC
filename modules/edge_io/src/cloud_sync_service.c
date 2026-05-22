/**
 * @file cloud_sync_service.c
 * @brief Edge-Ops 云端数据同步服务 (DataHub 快照上传)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <uv.h>

#include "datahub.h"
#include "edge_log.h"

typedef struct {
    datahub_t *hub;
    uv_timer_t timer;
    char cloud_url[256];
    uint32_t sync_interval_ms;
} cloud_sync_ctx_t;

static void _on_sync_timer(uv_timer_t *handle) {
    cloud_sync_ctx_t *ctx = (cloud_sync_ctx_t *)handle->data;
    
    // 1. 生成快照 (存入内存缓冲区)
    // 此处简化处理：直接调用 datahub_save_snapshot 存入临时文件
    const char *tmp_file = "/tmp/edge_snapshot.bin";
    if (datahub_save_snapshot(ctx->hub, tmp_file) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "CloudSync: Failed to save DataHub snapshot.");
        return;
    }

    // 2. 使用 libcurl 上传至云端
    CURL *curl = curl_easy_init();
    if (curl) {
        struct curl_httppost *formpost = NULL;
        struct curl_httppost *lastptr = NULL;

        curl_formadd(&formpost, &lastptr,
                     CURLFORM_COPYNAME, "snapshot",
                     CURLFORM_FILE, tmp_file,
                     CURLFORM_END);

        curl_easy_setopt(curl, CURLOPT_URL, ctx->cloud_url);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            LOG_ERROR(LOG_CAT_CORE, "CloudSync: Upload failed: %s", curl_easy_strerror(res));
        } else {
            LOG_INFO(LOG_CAT_CORE, "CloudSync: Successfully uploaded snapshot to %s", ctx->cloud_url);
        }

        curl_easy_cleanup(curl);
        curl_formfree(formpost);
    }
}

int cloud_sync_service_init(datahub_t *hub, uv_loop_t *loop, const char *url, uint32_t interval_ms) {
    cloud_sync_ctx_t *ctx = calloc(1, sizeof(cloud_sync_ctx_t));
    ctx->hub = hub;
    strncpy(ctx->cloud_url, url, sizeof(ctx->cloud_url) - 1);
    ctx->sync_interval_ms = interval_ms;

    uv_timer_init(loop, &ctx->timer);
    ctx->timer.data = ctx;
    
    LOG_INFO(LOG_CAT_CORE, "Edge-Ops: Cloud Sync Service initialized (Target: %s, Interval: %ums)", url, interval_ms);
    
    return uv_timer_start(&ctx->timer, _on_sync_timer, interval_ms, interval_ms);
}
