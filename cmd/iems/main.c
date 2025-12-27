#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "edge_service.h"
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
    svc.arbiter = arbiter_create(svc.loop);
    
    datahub_t hub;
    datahub_init(&hub, svc.loop, "/iems_hub", 512, NULL, &svc);
    svc.data_hub = &hub;
    
    persistence_mgr_t storage;
    persistence_mgr_init(&storage, svc.loop, &hub.ring, 1000);
    svc.storage = &storage;
    
    northbound_mgr_t cloud;
    northbound_mgr_init(&cloud, svc.loop, &hub.ring);
    svc.cloud = &cloud;
    northbound_mgr_start(svc.cloud);

    int ret = service_start(&svc);

    northbound_mgr_stop(svc.cloud);
    persistence_mgr_close(&storage);
    arbiter_destroy(svc.arbiter);
    net_buf_pool_destroy(&pool);
    return ret;
}