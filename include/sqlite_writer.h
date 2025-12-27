#pragma once
#include <uv.h>
#include <sqlite3.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sqlite_writer_s sqlite_writer_t;

/**
 * @brief Initialize the SQLite Writer (WAL mode, persistent connection).
 * 
 * @param w Pointer to the writer handle pointer.
 * @param path Path to the database file.
 * @return 0 on success, < 0 on failure.
 */
int sqlite_writer_init(sqlite_writer_t **w, const char *path);

/**
 * @brief Submit a SQL statement for asynchronous execution.
 * 
 * @param w The writer handle.
 * @param sql The SQL statement string (will be copied).
 */
void sqlite_writer_submit(sqlite_writer_t *w, const char *sql);

/**
 * @brief Execute a SQL statement synchronously (blocking).
 *        Useful for CREATE TABLE or initial setup.
 * 
 * @param w The writer handle.
 * @param sql The SQL statement.
 * @return SQLite error code (SQLITE_OK on success).
 */
int sqlite_writer_exec(sqlite_writer_t *w, const char *sql);

/**
 * @brief Destroy the writer and close the database connection.
 *        Waits for pending works (best effort).
 * 
 * @param w The writer handle.
 */
void sqlite_writer_destroy(sqlite_writer_t *w);

#ifdef __cplusplus
}
#endif
