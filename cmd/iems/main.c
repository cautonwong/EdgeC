#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "edge_service.h"
#include "services/js_engine.h"
#include "services/script_loader.h"
#include "edge_version.h"

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
    service_options_t opts = { .web_port = 8080, .rest_port = 8081 };
    strncpy(opts.config_path, "config/device_profile.json", 255);
    strncpy(opts.mode, "iems", 31);

    int opt;
    while ((opt = getopt(argc, argv, "c:w:r:h")) != -1) {
        switch (opt) {
            case 'c': strncpy(opts.config_path, optarg, 255); break;
            case 'w': opts.web_port = atoi(optarg); break;
            case 'r': opts.rest_port = atoi(optarg); break;
            case 'h': print_usage(argv[0]); return 0;
            default: return 1;
        }
    }

    // --- IEMS 产品组件装配 ---
    iems_ctx_t *iems = calloc(1, sizeof(iems_ctx_t));
    service_t *svc = service_new(&opts);
    
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 512, 512, NULL, NULL);

    arbiter_t *arbiter;
    arbiter_init(&arbiter, loop);

    datahub_t hub;
    datahub_init(&hub, loop, "/iems_hub", 512, NULL, NULL);

    persistence_mgr_t storage;
    persistence_mgr_init(&storage, loop, &hub.ring, 1000);

    northbound_mgr_t cloud;
    northbound_mgr_init(&cloud, loop, &hub.ring);
    cloud.web_port = opts.web_port;
    cloud.rest_port = opts.rest_port;
    cloud.user_data = svc; 
    northbound_mgr_start(&cloud);

    // JS 引擎也是产品级的需求，在 App 层开启
    js_engine_t *js_engine = js_engine_create(svc, 2 * 1024 * 1024);
    script_loader_t *loader = script_loader_create(js_engine, "config/scripts", 1000);
    script_loader_start(loader);

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