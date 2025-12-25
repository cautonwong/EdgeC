#pragma once
#include <uv.h>

// Callback function type for SQL execution results
typedef void (*sql_cb_t)(int rc, void *user);

/**
 * @brief Initializes the SQLite service.
 * @param loop The uv_loop to use for asynchronous operations.
 * @param db_path The path to the SQLite database file.
 * @return 0 on success, -1 on failure.
 */
int sqlite_service_init(uv_loop_t *loop, const char *db_path);

/**
 * @brief Closes the SQLite service, releasing resources.
 */
void sqlite_service_close(void);

/**
 * @brief Executes an SQL statement asynchronously.
 * The SQL execution happens in a libuv worker thread,
 * and the callback is invoked in the main uv_loop thread.
 *
 * @param sql The SQL statement to execute. Must be a NULL-terminated string.
 *            The service will take ownership of this string (strdup it).
 * @param cb The callback function to be invoked after SQL execution.
 * @param user User data to be passed to the callback.
 * @return 0 on success, -1 if the service is not initialized or arguments are invalid.
 */
int sqlite_exec_async(
    const char *sql,
    sql_cb_t cb,
    void *user
);
