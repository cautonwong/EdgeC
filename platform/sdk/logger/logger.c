#include "logger.h"
#include <stdio.h>

zlog_category_t *g_log_cat = NULL;

int logger_init(const char *config_path) {
    if (zlog_init(config_path) != 0) {
        printf("ERROR: Failed to initialize zlog with config: %s\n", config_path);
        return -1;
    }

    g_log_cat = zlog_get_category("default_cat");
    if (!g_log_cat) {
        printf("ERROR: zlog_get_category('default_cat') failed\n");
        zlog_fini();
        return -1;
    }

    return 0;
}

void logger_cleanup(void) {
    zlog_fini();
}

