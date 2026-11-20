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
    if (conf_path) {
        strncpy(logger->conf_path, conf_path, 255);
        logger->conf_path[sizeof(logger->conf_path) - 1] = '\0';
    }
    
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
    /* 512 是 check_stack_usage 的自动数组上限；这一行日志远短于它，超长安全截断。 */
    char line[512];
    va_list args;

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
        va_start(args, fmt);
        vsnprintf(line, sizeof(line), fmt, args);
        va_end(args);
        zlog(l->zc, __FILE__, sizeof(__FILE__) - 1, __func__, sizeof(__func__) - 1, __LINE__,
             zlog_level, "%s", line);
        return;
    }

    /* 三条 printf 之间会被别的线程 fprintf(stderr)（NETBUF 日志）插进来，
     * shell 把 stdout/stderr 并到同一个管道时，启动汇总行会被劈成两半。
     * 先拼出整行，再发一次写。 */
    static const char *level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);

    int n = snprintf(line, sizeof(line), "[%s][%s][%s] ", time_buf, level_names[level % 5],
                     cat_str);
    size_t used = (n > 0) ? ((size_t)n < sizeof(line) ? (size_t)n : sizeof(line) - 1) : 0;
    va_start(args, fmt);
    int m = vsnprintf(line + used, sizeof(line) - used, fmt, args);
    va_end(args);
    if (m > 0 && used < sizeof(line) - 1) {
        size_t room = sizeof(line) - used;
        used += ((size_t)m < room) ? (size_t)m : room - 1;
    }
    if (used < sizeof(line) - 1) line[used++] = '\n';
    line[used] = '\0';
    fwrite(line, 1, used, stdout);
    fflush(stdout);
}

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
__attribute__((visibility("default"), used)) const char *__asan_default_options(void) {
    return "detect_leaks=0:halt_on_error=1";
}
#endif

#if defined(__SANITIZE_THREAD__) || __has_feature(thread_sanitizer)
__attribute__((visibility("default"), used)) const char *__tsan_default_options(void) {
    return "halt_on_error=0:suppress_equal_stacks=1:die_after_fork=0:exitcode=0";
}
#endif

