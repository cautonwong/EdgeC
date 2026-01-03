#include "edge_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "zlog.h"

#define MAX_LOG_MSG_LEN 512
#define ASYNC_RING_SIZE 1024

// 异步日志条目
typedef struct {
    edge_log_level_t level;
    uint64_t ts_ns;
    const char *cat_name;  // 使用静态宏指针，避免拷贝
    char msg[MAX_LOG_MSG_LEN];
} log_entry_t;

// --- Category 缓存 (Principles 1: Performance) ---
typedef struct {
    const char *name;
    zlog_category_t *ptr;
} cat_cache_entry_t;

static cat_cache_entry_t g_cat_cache[] = {{"core", NULL},    {"io", NULL},       {"bus", NULL},
                                          {"driver", NULL},  {"proto", NULL},    {"sched", NULL},
                                          {"storage", NULL}, {"security", NULL}, {NULL, NULL}};

static struct {
    bool inited;
    bool is_async;
    uv_thread_t worker;

    uv_mutex_t lock;
    uv_cond_t cond;

    log_entry_t *ring;
    uint32_t head;
    uint32_t tail;

    uint64_t drop_count;
    bool stop_flag;
} g_log_ctx;

// --- 内部逻辑 ---

static zlog_category_t *get_cat_ptr(const char *name) {
    for (int i = 0; g_cat_cache[i].name != NULL; i++) {
        if (g_cat_cache[i].name == name) return g_cat_cache[i].ptr;  // 指针相等判定，极速
    }
    return g_cat_cache[0].ptr;  // Fallback to core
}

static void log_to_backend(edge_log_level_t level, const char *cat_name, const char *msg) {
    zlog_category_t *c = get_cat_ptr(cat_name);

    switch (level) {
        case EDGE_LOG_DEBUG:
            zlog_debug(c, "%s", msg);
            break;
        case EDGE_LOG_INFO:
            zlog_info(c, "%s", msg);
            break;
        case EDGE_LOG_WARN:
            zlog_warn(c, "%s", msg);
            break;
        case EDGE_LOG_ERROR:
            zlog_error(c, "%s", msg);
            break;
        case EDGE_LOG_FATAL:
            zlog_fatal(c, "%s", msg);
            break;
    }
}

static void log_worker_thread(void *arg) {
    (void)arg;
    while (1) {
        log_entry_t entry;
        bool has_data = false;

        uv_mutex_lock(&g_log_ctx.lock);
        while (g_log_ctx.head == g_log_ctx.tail && !g_log_ctx.stop_flag) {
            uv_cond_wait(&g_log_ctx.cond, &g_log_ctx.lock);
        }

        if (g_log_ctx.head != g_log_ctx.tail) {
            entry = g_log_ctx.ring[g_log_ctx.tail];
            g_log_ctx.tail = (g_log_ctx.tail + 1) % ASYNC_RING_SIZE;
            has_data = true;
        }

        bool should_stop = g_log_ctx.stop_flag && (g_log_ctx.head == g_log_ctx.tail);
        uv_mutex_unlock(&g_log_ctx.lock);

        if (has_data) {
            log_to_backend(entry.level, entry.cat_name, entry.msg);
        }

        if (should_stop) break;
    }
}

// --- 公开接口 ---

int edge_log_init(const char *conf_path, bool async) {
    if (g_log_ctx.inited) return 0;

    if (zlog_init(conf_path) != 0) {
        fprintf(stderr, "[Log] zlog_init failed: %s\n", conf_path);
        return -1;
    }

    // [吸收精华] 启动时一次性 cache 所有 Category 指针
    for (int i = 0; g_cat_cache[i].name != NULL; i++) {
        g_cat_cache[i].ptr = zlog_get_category(g_cat_cache[i].name);
    }

    g_log_ctx.is_async = async;
    if (async) {
        g_log_ctx.ring = calloc(ASYNC_RING_SIZE, sizeof(log_entry_t));
        g_log_ctx.head = 0;
        g_log_ctx.tail = 0;
        g_log_ctx.drop_count = 0;
        g_log_ctx.stop_flag = false;

        uv_mutex_init(&g_log_ctx.lock);
        uv_cond_init(&g_log_ctx.cond);
        uv_thread_create(&g_log_ctx.worker, log_worker_thread, NULL);
    }

    g_log_ctx.inited = true;
    return 0;
}

uint64_t edge_log_drop_count(void) {
    return g_log_ctx.drop_count;
}

void edge_log_fini(void) {
    if (!g_log_ctx.inited) return;

    if (g_log_ctx.is_async) {
        uv_mutex_lock(&g_log_ctx.lock);
        g_log_ctx.stop_flag = true;
        uv_cond_signal(&g_log_ctx.cond);
        uv_mutex_unlock(&g_log_ctx.lock);
        uv_thread_join(&g_log_ctx.worker);
        uv_mutex_destroy(&g_log_ctx.lock);
        uv_cond_destroy(&g_log_ctx.cond);
        free(g_log_ctx.ring);
    }

    zlog_fini();
    g_log_ctx.inited = false;
}

void edge_log_write(edge_log_level_t level, const char *cat, const char *file, int line,
                    const char *fmt, ...) {
    if (!g_log_ctx.inited) return;

    char formatted_msg[MAX_LOG_MSG_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(formatted_msg, MAX_LOG_MSG_LEN, fmt, ap);
    va_end(ap);

    char final_msg[MAX_LOG_MSG_LEN + 128];
    const char *color_code = "";
    const char *reset_code = "\033[0m";

    switch (level) {
        case EDGE_LOG_DEBUG:
            color_code = "\033[1;34m";
            break;  // Blue
        case EDGE_LOG_INFO:
            color_code = "\033[1;32m";
            break;  // Green
        case EDGE_LOG_WARN:
            color_code = "\033[1;33m";
            break;  // Yellow
        case EDGE_LOG_ERROR:
            color_code = "\033[1;31m";
            break;  // Red
        case EDGE_LOG_FATAL:
            color_code = "\033[1;35m";
            break;  // Magenta
    }

    // 格式化：[Color][File:Line] Message [Reset]
    snprintf(final_msg, sizeof(final_msg), "%s[%s:%d] %s%s", color_code, file, line, formatted_msg,
             reset_code);

    if (level == EDGE_LOG_FATAL) {
        log_to_backend(level, cat, final_msg);
        fprintf(stderr, "FATAL: %s\n", final_msg);
        edge_log_fini();
        abort();
    }

    if (!g_log_ctx.is_async) {
        log_to_backend(level, cat, final_msg);
    } else {
        uv_mutex_lock(&g_log_ctx.lock);
        uint32_t next_head = (g_log_ctx.head + 1) % ASYNC_RING_SIZE;
        if (next_head != g_log_ctx.tail) {
            log_entry_t *entry = &g_log_ctx.ring[g_log_ctx.head];
            entry->level = level;
            entry->ts_ns = uv_hrtime();
            entry->cat_name = cat;  // 存入宏指针，不拷贝字符串
            // 格式化并截断日志内容
            snprintf(entry->msg, sizeof(entry->msg), "%.*s", (int)sizeof(entry->msg) - 1,
                     final_msg);
            g_log_ctx.head = next_head;
            uv_cond_signal(&g_log_ctx.cond);
        } else {
            g_log_ctx.drop_count++;
        }
        uv_mutex_unlock(&g_log_ctx.lock);
    }
}