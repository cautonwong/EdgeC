#include "edge_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "zlog.h"

struct edge_logger_s {
    bool console_only;
    char conf_path[256];
    zlog_category_t *zc;
    bool zlog_active;
};

static edge_log_t *g_default_logger = NULL;

edge_log_t *edge_log_create(const char *conf_path, bool console_only) {
    edge_log_t *logger = calloc(1, sizeof(struct edge_logger_s));
    if (!logger) return NULL;
    
    logger->console_only = console_only;
    if (conf_path) strncpy(logger->conf_path, conf_path, 255);
    
    if (!console_only && conf_path && conf_path[0] != '\0') {
        int rc = zlog_init(conf_path);
        if (rc == 0) {
            logger->zc = zlog_get_category("edgec");
            if (logger->zc) {
                logger->zlog_active = true;
            }
        }
    }
    
    if (!g_default_logger) g_default_logger = logger;
    return logger;
}

void edge_log_destroy(edge_log_t *logger) {
    if (!logger) return;
    if (logger->zlog_active) {
        zlog_fini();
        logger->zlog_active = false;
    }
    if (logger == g_default_logger) g_default_logger = NULL;
    free(logger);
}

void edge_log_out(edge_log_t *logger, edge_log_cat_t cat, int level, const char *fmt, ...) {
    edge_log_t *l = logger ? logger : g_default_logger;
    static const char *cat_names[] = {"CORE", "IO", "BUS", "SCHED", "UI", "NB"};
    const char *cat_str = cat_names[cat % LOG_CAT_MAX];

    va_list args;
    va_start(args, fmt);

    if (l && l->zlog_active && l->zc) {
        int zlog_level = ZLOG_LEVEL_INFO;
        switch (level) {
            case 0: zlog_level = ZLOG_LEVEL_DEBUG; break;
            case 1: zlog_level = ZLOG_LEVEL_INFO; break;
            case 2: zlog_level = ZLOG_LEVEL_NOTICE; break;
            case 3: zlog_level = ZLOG_LEVEL_WARN; break;
            case 4: zlog_level = ZLOG_LEVEL_ERROR; break;
            default: zlog_level = ZLOG_LEVEL_FATAL; break;
        }
        vzlog(l->zc, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__, zlog_level, fmt, args);
    } else {
        static const char *level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        time_t now = time(NULL);
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);

        printf("[%s][%s][%s] ", time_buf, level_names[level % 5], cat_str);
        vprintf(fmt, args);
        printf("\n");
    }
    va_end(args);
}
