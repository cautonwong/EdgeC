#ifndef EDGE_LOG_H
#define EDGE_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 日志分级
typedef enum {
    EDGE_LOG_DEBUG = 20,
    EDGE_LOG_INFO = 40,
    EDGE_LOG_WARN = 60,
    EDGE_LOG_ERROR = 80,
    EDGE_LOG_FATAL = 100
} edge_log_level_t;

// --- 标准 Category 定义 ---
#define LOG_CAT_CORE "core"          // 核心系统、配置、生命周期
#define LOG_CAT_IO "io"              // 网络、uv、socket
#define LOG_CAT_BUS "bus"            // shmring、总线交换
#define LOG_CAT_DRIVER "driver"      // 设备驱动、modbus/can
#define LOG_CAT_PROTO "proto"        // 协议编解码
#define LOG_CAT_SCHED "scheduler"    // 任务调度、状态机
#define LOG_CAT_STORAGE "storage"    // sqlite、持久化
#define LOG_CAT_SECURITY "security"  // 认证、审计、安全

/**
 * @brief 初始化日志系统
 * @param conf_path zlog 配置文件路径
 * @param async 是否开启异步旁路 (libuv thread 模式)
 * @return 0 成功
 */
int edge_log_init(const char *conf_path, bool async);

/**
 * @brief 获取因缓冲区满而丢弃的日志总数 (工业级观测指标)
 */
uint64_t edge_log_drop_count(void);

/**
 * @brief 彻底关闭日志系统
 */
void edge_log_fini(void);

/**
 * @brief 工业级日志接口 (支持格式化)
 * @param level 分级
 * @param cat 模块分类 (如 "core", "driver", "stream")
 * @param file 文件名
 * @param line 行号
 * @param fmt 格式化字符串
 */
void edge_log_write(edge_log_level_t level, const char *cat, const char *file, int line,
                    const char *fmt, ...);

// --- 面向业务的便捷宏 ---

#define LOG_DEBUG(cat, fmt, ...) \
    edge_log_write(EDGE_LOG_DEBUG, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_INFO(cat, fmt, ...) \
    edge_log_write(EDGE_LOG_INFO, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WARN(cat, fmt, ...) \
    edge_log_write(EDGE_LOG_WARN, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(cat, fmt, ...) \
    edge_log_write(EDGE_LOG_ERROR, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(cat, fmt, ...) \
    edge_log_write(EDGE_LOG_FATAL, cat, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif  // EDGE_LOG_H
