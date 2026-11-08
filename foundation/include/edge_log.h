#ifndef EDGE_LOG_H
#define EDGE_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    LOG_CAT_CORE = 0,
    LOG_CAT_IO,
    LOG_CAT_BUS,
    LOG_CAT_SCHED,
    LOG_CAT_UI,
    LOG_CAT_NB,    /**< Northbound / API categories */
    LOG_CAT_MAX
} edge_log_cat_t;

// [Sprint 1] 兼容旧代码的简写定义
#define CORE  LOG_CAT_CORE
#define IO    LOG_CAT_IO
#define BUS   LOG_CAT_BUS
#define SCHED LOG_CAT_SCHED
#define UI    LOG_CAT_UI
#define NB    LOG_CAT_NB

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

// 为了兼容现有代码的宏定义 (使用主实例)
#define LOG_DEBUG(cat, fmt, ...) edge_log_out(NULL, cat, 0, fmt, ##__VA_ARGS__)
#define LOG_INFO(cat, fmt, ...)  edge_log_out(NULL, cat, 1, fmt, ##__VA_ARGS__)
#define LOG_WARN(cat, fmt, ...)  edge_log_out(NULL, cat, 2, fmt, ##__VA_ARGS__)
#define LOG_ERROR(cat, fmt, ...) edge_log_out(NULL, cat, 3, fmt, ##__VA_ARGS__)
#define LOG_FATAL(cat, fmt, ...) edge_log_out(NULL, cat, 4, fmt, ##__VA_ARGS__)

#endif