#ifndef EDGE_LOG_H
#define NORTH_LOG_H

#include <stdarg.h>
#include <stdbool.h>

typedef enum {
    LOG_CAT_CORE = 0,
    LOG_CAT_IO,
    LOG_CAT_BUS,
    LOG_CAT_SCHED,
    LOG_CAT_UI,
    LOG_CAT_MAX
} edge_log_cat_t;

typedef struct edge_logger_s edge_log_t;

/**
 * @brief 创建日志对象实例 (解耦全局状态)
 */
edge_log_t *edge_log_create(const char *conf_path, bool console_only);

/**
 * @brief 销毁日志对象
 */
void edge_log_destroy(edge_log_t *logger);

/**
 * @brief 实例化的日志输出接口
 */
void edge_log_out(edge_log_t *logger, edge_log_cat_t cat, int level, const char *fmt, ...);

// 为了兼容现有代码的宏定义 (会使用主实例)
#define LOG_INFO(cat, fmt, ...) printf("[INFO][" #cat "] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(cat, fmt, ...) printf("[WARN][" #cat "] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(cat, fmt, ...) fprintf(stderr, "[ERR][" #cat "] " fmt "\n", ##__VA_ARGS__)
#define LOG_FATAL(cat, fmt, ...) fprintf(stderr, "[FATAL][" #cat "] " fmt "\n", ##__VA_ARGS__)

#endif