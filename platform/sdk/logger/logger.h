#ifndef CEDGE_LOGGER_H
#define CEDGE_LOGGER_H

#include "zlog.h"

// Global zlog category
extern zlog_category_t *g_log_cat;

/**
 * @brief Initializes the logging system.
 *
 * @param config_path Path to the zlog configuration file.
 * @return 0 on success, -1 on failure.
 */
int logger_init(const char *config_path);

/**
 * @brief Shuts down the logging system.
 */
void logger_cleanup(void);

// --- Logging Macros ---
// These macros wrap the zlog functions to provide a simpler interface
// and automatically include file, function, and line number information.

#define LOG_DEBUG(...)                                       \
  dzlog(g_log_cat, __FILE__, sizeof(__FILE__) - 1, __func__, \
        sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_DEBUG, __VA_ARGS__)

#define LOG_INFO(...)                                        \
  dzlog(g_log_cat, __FILE__, sizeof(__FILE__) - 1, __func__, \
        sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_INFO, __VA_ARGS__)

#define LOG_WARN(...)                                        \
  dzlog(g_log_cat, __FILE__, sizeof(__FILE__) - 1, __func__, \
        sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_WARN, __VA_ARGS__)

#define LOG_ERROR(...)                                       \
  dzlog(g_log_cat, __FILE__, sizeof(__FILE__) - 1, __func__, \
        sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_ERROR, __VA_ARGS__)

#define LOG_FATAL(...)                                       \
  dzlog(g_log_cat, __FILE__, sizeof(__FILE__) - 1, __func__, \
        sizeof(__func__) - 1, __LINE__, ZLOG_LEVEL_FATAL, __VA_ARGS__)

#endif  // CEDGE_LOGGER_H
