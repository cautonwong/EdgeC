#ifndef DB_ENGINE_H
#define DB_ENGINE_H

#include <uv.h>

#include "../include/cedgex/common_types.h"

/**
 * @brief Initializes the asynchronous database engine.
 *
 * This sets up the SQLite connection (in WAL mode), creates the necessary
 * tables, initializes the double-buffer mechanism, and starts a periodic timer
 * to flush data to disk.
 *
 * @param loop The main libuv loop.
 * @param db_path The path to the SQLite database file.
 * @return 0 on success, -1 on failure.
 */
int db_engine_init(uv_loop_t *loop, const char *db_path);

/**
 * @brief Asynchronously inserts a single data point.
 *
 * This function is non-blocking. It quickly places the data point into an
 * in-memory buffer (the active buffer) and returns immediately. The data will
 * be written to the database later in a background thread.
 *
 * @param dp A pointer to the data point to be inserted.
 * @return 0 on success, -1 if the active buffer is full.
 */
int db_engine_insert_async(cedgex_datapoint_t *dp);

/**
 * @brief Shuts down the database engine gracefully.
 *
 * This will perform a final flush of any remaining data in the active buffer.
 */
void db_engine_cleanup(void);

#endif  // DB_ENGINE_H
