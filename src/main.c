#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "edge_service.h"
#include "edge_version.h"

// 产品入口声明 (通常应在各自头文件中)
extern int run_iems_product(service_options_t *opts);

static void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -c, --config <path>    Config file (default: config/device_profile.json)\n");
    printf("  -m, --mode <type>      Mode: hems | iems | sg | evse | dcu\n");
    printf("  -w, --web-port <port>  Web HMI port (default: 8080)\n");
    printf("  -r, --rest-port <port> REST API port (default: 8081)\n");
    printf("  -v, --version          Show version\n");
    printf("  -h, --help             Show help\n");
}

int main(int argc, char **argv) {
    service_options_t opts;
    memset(&opts, 0, sizeof(opts));

    // 默认值
    strncpy(opts.config_path, "config/device_profile.json", 255);
    strncpy(opts.mode, "iems", 31);
    opts.web_port = 8080;
    opts.rest_port = 8081;

    static struct option long_options[] = {
        {"config",    required_argument, 0, 'c'},
        {"mode",      required_argument, 0, 'm'},
        {"web-port",  required_argument, 0, 'w'},
        {"rest-port", required_argument, 0, 'r'},
        {"version",   no_argument,       0, 'v'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:m:w:r:vh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c': strncpy(opts.config_path, optarg, 255); break;
            case 'm': strncpy(opts.mode, optarg, 31); break;
            case 'w': opts.web_port = atoi(optarg); break;
            case 'r': opts.rest_port = atoi(optarg); break;
            case 'v': printf("EdgeOS version %s\n", edge_get_version()); return 0;
            case 'h': print_usage(argv[0]); return 0;
            default: return 1;
        }
    }

    printf("[Main] Starting EdgeOS in %s mode...\n", opts.mode);

    // 根据模式分发到不同产品逻辑
    if (strcmp(opts.mode, "iems") == 0) {
        return run_iems_product(&opts);
    } else {
        fprintf(stderr, "Mode '%s' not implemented yet.\n", opts.mode);
        return 1;
    }
}