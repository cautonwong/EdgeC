#ifndef DB_ENGINE_H
#define DB_ENGINE_H

#include <uv.h>
#include <stdbool.h>
#include <stdint.h>

// A generic datapoint structure for the database
typedef struct {
    uint64_t ts;
    uint32_t tag_id;
    double value;
    uint8_t quality;
} db_datapoint_t;

/**
 * @brief Initializes the asynchronous database service.
 * @param loop The libuv event loop.
 * @param db_path Path to the SQLite database file.
 * @param batch_size The number of datapoints to buffer before forcing a flush.
 * @param flush_interval_ms The interval in milliseconds to periodically flush the buffer.
 * @return 0 on success, -1 on error.
 */
int db_engine_init(uv_loop_t *loop, const char *db_path, size_t batch_size, uint64_t flush_interval_ms);

/**
 * @brief Asynchronously inserts a single datapoint.
 *
 * This function is non-blocking and returns immediately. The datapoint is
 * placed in a memory buffer and will be written to the database later.
 * @param dp Pointer to the datapoint to insert.
 * @return true if the datapoint was successfully buffered, false if the buffer
 *         was full and the data was dropped.
 */
bool db_engine_insert(const db_datapoint_t *dp);

/**
 * @brief Destroys the database service, flushing any remaining data.
 */
void db_engine_destroy(void);

#endif /* DB_ENGINE_H */