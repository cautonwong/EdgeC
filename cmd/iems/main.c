#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "contracts.h"
#include "edge_log.h"
#include "edge_service.h"
#include "js_engine.h"
#include "script_loader.h"
#include "stream_engine.h"

// 模拟器：产生实时数据并更新影子缓存
static void on_dummy_produce(uv_timer_t *t) {
    service_t *s = (service_t *)t->data;
    datahub_t *hub = service_get_datahub(s);
    if (!hub) return;

    // 1. 产生电网功率 (Tag 1003)
    PipeMsg m1 = {.tag_id = TAG_GRID_POWER,
                  .type = PIPEMSG_SCALAR_F64,
                  .payload = {.val = {.f = 2000.0 + (rand() % 500)}},
                  .ts = (uint64_t)time(NULL) * 1000};
    datahub_publish(hub, &m1);

    // 2. 产生电池 SOC (Tag 4001)
    PipeMsg m2 = {.tag_id = TAG_BATTERY_SOC,
                  .type = PIPEMSG_SCALAR_F64,
                  .payload = {.val = {.f = 85.5}},
                  .ts = (uint64_t)time(NULL) * 1000};
    datahub_publish(hub, &m2);
}

int main(int argc, char **argv) {
    service_options_t opts = {.web_port = 8080, .rest_port = 8081};
    int opt;
    while ((opt = getopt(argc, argv, "w:r:h")) != -1) {
        switch (opt) {
            case 'w':
                opts.web_port = atoi(optarg);
                break;
            case 'r':
                opts.rest_port = atoi(optarg);
                break;
            case 'h':
                printf("EdgeOS IEMS\n");
                return 0;
        }
    }

    service_t *svc = service_new(&opts);
    edge_log_init("config/zlog.conf", false);
    service_set_contract_path(svc, "./build/products/iems/generated/contracts.yaml");

    uv_loop_t *loop = uv_default_loop();
    service_set_loop(svc, loop);

    // --- 数据总线 ---
    static datahub_t hub;
    datahub_init(&hub, loop, "/iems_full_bus", 512, NULL, NULL);
    service_set_datahub(svc, &hub);

    // --- 流引擎 ---
    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    service_set_stream_engine(svc, engine);

    // 加载真实的 Pipeline 逻辑
    pipeline_loader_ctx_t l_ctx = {
        .db = NULL,
        .js_engine = NULL,
        .params = service_get_params(svc)  // 注入参数服务
    };
    stream_engine_load_config(engine, "config/iems_pipelines.json", &l_ctx);
    // stream_engine_start 已被移除，create 时自动启动线程

    // --- 北向与模拟器 ---
    persistence_mgr_t storage = {0};
    northbound_mgr_t cloud;
    northbound_mgr_init(&cloud, loop, &hub.ring);
    cloud.web_port = opts.web_port;
    cloud.rest_port = opts.rest_port;
    strncpy(cloud.web_root, "build/products/iems/www/index.html", sizeof(cloud.web_root) - 1);
    cloud.user_data = svc;
    northbound_mgr_start(&cloud);

    uv_timer_t dummy_producer;
    uv_timer_init(loop, &dummy_producer);
    dummy_producer.data = svc;
    uv_timer_start(&dummy_producer, on_dummy_produce, 100, 1000);

    printf("[IEMS] System LIVE. UI: http://localhost:%d\n", opts.web_port);
    return service_run(svc, loop, NULL, NULL, &hub, &storage, &cloud);
}