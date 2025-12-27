#ifndef DATAHUB_H
#define DATAHUB_H

#include <uv.h>

#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct datahub_s datahub_t;

// Callback for when new data is available
typedef void (*datahub_on_data_cb)(datahub_t *hub, void *user_data);

// The DataHub structure
struct datahub_s {
    shm_ring_t ring;             // The shared memory ring buffer
    uv_loop_t *loop;             // The loop to notify
    uv_async_t async_handle;     // Handle to wake up the loop
    datahub_on_data_cb on_data;  // User callback
    void *user_data;             // User data for the callback
};

/**
 * @brief Initializes a DataHub instance.
 *
 * @param hub The DataHub instance to initialize.
 * @param loop The uv_loop_t to associate with the DataHub for notifications.
 * @param name A unique name for the underlying shared memory segment.
 * @param slots The number of slots to create in the ring buffer.
 * @param on_data The callback to invoke when new data is published.
 * @param user_data Custom data to be passed to the on_data callback.
 * @return 0 on success, or a negative error code.
 */
int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 datahub_on_data_cb on_data, void *user_data);

/**
 * @brief Shuts down the DataHub and releases its resources.
 *
 * @param hub The DataHub instance to destroy.
 */
void datahub_destroy(datahub_t *hub);

/**
 * @brief Publishes a message to the DataHub.
 *
 * This function writes a message to the shared memory ring buffer and
 * notifies the consumer's loop that new data is available.
 * This function is thread-safe.
 *
 * @param hub The DataHub instance.
 * @param msg The message to publish.
 * @return 0 on success, or a negative error code.
 */
int datahub_publish(datahub_t *hub, const pipe_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif  // DATAHUB_H
