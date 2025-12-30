#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_service.h"
#include "edge_version.h"
#include "js_engine.h"
#include "script_loader.h"
#include "contracts.h"

// IEMS 产品特定的业务上下文
typedef struct {
    service_t *svc;
    double battery_soc;
} iems_ctx_t;

static void print_usage(const char *prog) {
    printf("EdgeOS IEMS Product Console\n");
    printf("Usage: %s [options]\n", prog);
}

int main(int argc, char **argv) {
    service_options_t opts = {.web_port = 8080, .rest_port = 8081};
    strncpy(opts.config_path, "config/device_profile.json", 255);
    strncpy(opts.mode, "iems", 31);

    int opt;
    while ((opt = getopt(argc, argv, "c:m:w:r:h")) != -1) {
        switch (opt) {
            case 'c':
                strncpy(opts.config_path, optarg, 255);
                break;
            case 'm':
                strncpy(opts.mode, optarg, 31);
                break;
            case 'w':
                opts.web_port = atoi(optarg);
                break;
            case 'r':
                opts.rest_port = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                return 1;
        }
    }

    // --- IEMS 产品组件装配 ---
    iems_ctx_t *iems = calloc(1, sizeof(iems_ctx_t));
    service_t *svc = service_new(&opts);

    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    service_set_loop(svc, loop);

    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 512, 512, NULL, NULL);

    arbiter_t *arbiter;
    arbiter_init(&arbiter, loop);

    datahub_t hub;
    datahub_init(&hub, loop, "/iems_hub", 512, NULL, NULL);

    persistence_mgr_t storage;
    sqlite_config_t db_cfg = {.strategy = STRATEGY_SERIES,
                              .db_path = "iems_data.db",
                              .batch_size = 100,
                              .flush_interval_ms = 1000};
    storage.writer = sqlite_svc_create(loop, &db_cfg);
    persistence_mgr_init(&storage, loop, &hub.ring, 1000);

    northbound_mgr_t cloud;
    northbound_mgr_init(&cloud, loop, &hub.ring);
    cloud.web_port = opts.web_port;
    cloud.rest_port = opts.rest_port;
    cloud.user_data = svc;
    northbound_mgr_start(&cloud);

    // JS 引擎也是产品级的需求，在 App 层开启
    js_engine_t *js_engine = js_engine_create(svc, 2 * 1024 * 1024);
    script_loader_t *loader = script_loader_create(loop, js_engine, "algo/js", 1000);
    script_loader_start(loader);

    // --- S1 Scenario Simulator: Produce dummy data ---
    uv_timer_t dummy_producer;
    uv_timer_init(loop, &dummy_producer);
    void on_dummy_produce(uv_timer_t * t) {
        service_t *s = (service_t *)t->data;
        shm_ring_t *ring = service_get_ring(s);
        if (!ring) return;

        pipe_msg_t m = {.type = PIPEMSG_SCALAR_F64,

                        .tag_id = TAG_GRID_POWER,

                        .payload = {.val = {.f = 220.0 + (rand() % 100) / 10.0}},

                        .ts = uv_now(service_get_loop(s))

        };

        shm_ring_write(ring, &m);
    }
    dummy_producer.data = svc;
    // 延迟 100ms 启动，确保容器已 Ready
    uv_timer_start(&dummy_producer, on_dummy_produce, 100, 1000);

    // 运行服务
    int ret = service_run(svc, loop, &pool, arbiter, &hub, &storage, &cloud);

    // 清理
    script_loader_destroy(loader);
    js_engine_destroy(js_engine);
    northbound_mgr_stop(&cloud);
    persistence_mgr_close(&storage);
    arbiter_shutdown(arbiter);
    net_buf_pool_destroy(&pool);
    service_delete(svc);
    free(iems);
    free(loop);

    return ret;
}
