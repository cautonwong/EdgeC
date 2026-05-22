#include "edge_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct edge_logger_s {
    bool console_only;
    char conf_path[256];
};

static edge_log_t *g_default_logger = NULL;

edge_log_t *edge_log_create(const char *conf_path, bool console_only) {
    edge_log_t *logger = calloc(1, sizeof(struct edge_logger_s));
    logger->console_only = console_only;
    if (conf_path) strncpy(logger->conf_path, conf_path, 255);
    
    if (!g_default_logger) g_default_logger = logger;
    return logger;
}

void edge_log_destroy(edge_log_t *logger) {
    if (logger == g_default_logger) g_default_logger = NULL;
    if (logger) free(logger);
}

void edge_log_out(edge_log_t *logger, edge_log_cat_t cat, int level, const char *fmt, ...) {
    edge_log_t *l = logger ? logger : g_default_logger;
    // For now, we print to console regardless of l. 
    // In a future sprint, we can use l->console_only and l->conf_path.
    (void)l; 

    static const char *level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    static const char *cat_names[] = {"CORE", "IO", "BUS", "SCHED", "UI", "NB"};

    va_list args;
    va_start(args, fmt);

    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);

    printf("[%s][%s][%s] ", time_buf, level_names[level % 5], cat_names[cat % LOG_CAT_MAX]);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    
    // In a future sprint, we can add file output here based on l->conf_path
}
