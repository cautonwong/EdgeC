#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "edge_service.h"
#include "services/js_engine.h"
#include "services/script_loader.h"
#include "edge_version.h"

static void print_usage(const char *prog) {
    printf("EdgeOS IEMS Product Console\n");
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -c, --config <path>    Config file\n");
    printf("  -w, --web-port <port>  Web HMI port\n");
    printf("  -r, --rest-port <port> REST API port\n");
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

    service_t svc;
    memset(&svc, 0, sizeof(svc));
    svc.opts = opts;
    svc.loop = uv_default_loop();

    printf("[IEMS] Initializing components...\n");
    
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 512, 512, NULL, NULL);
    svc.pool = &pool;
    
    svc.config = profile_load(opts.config_path);
    arbiter_init(&svc.arbiter, svc.loop);
    
    datahub_t hub;
    datahub_init(&hub, svc.loop, "/iems_hub", 512, NULL, &svc);
    svc.data_hub = &hub;
    
    persistence_mgr_t storage;
    persistence_mgr_init(&storage, svc.loop, &hub.ring, 1000);
    svc.storage = &storage;
    
    // Northbound setup
    northbound_mgr_t cloud;
    northbound_mgr_init(&cloud, svc.loop, &hub.ring);
    svc.cloud = &cloud;
    svc.cloud->web_port = opts.web_port;
    svc.cloud->rest_port = opts.rest_port;
    svc.cloud->user_data = &svc;
    northbound_mgr_start(svc.cloud);

    // --- JS Scripting Service ---
    js_engine_t *js_engine = js_engine_create(&svc, 2 * 1024 * 1024); // 2MB limit
    
    // --- Dynamic Script Loader ---
    // Watch 'config/scripts' directory, run every 1000ms
    system("mkdir -p config/scripts");
    script_loader_t *loader = script_loader_create(js_engine, "config/scripts", 1000);
    script_loader_start(loader);

    int ret = service_start(&svc);

    // Cleanup
    script_loader_destroy(loader);
    js_engine_destroy(js_engine);
    northbound_mgr_stop(svc.cloud);
    persistence_mgr_close(&storage);
    arbiter_shutdown(svc.arbiter);
    net_buf_pool_destroy(&pool);
    return ret;
}