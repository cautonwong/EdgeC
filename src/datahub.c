#include "datahub.h"
#include <stdlib.h>
#include <string.h>

// This is the internal callback triggered by uv_async_send
static void on_async_notification(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (hub && hub->on_data) {
        hub->on_data(hub, hub->user_data);
    }
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots, datahub_on_data_cb on_data, void *user_data) {
    if (!hub || !loop || !name || !on_data) {
        return -1;
    }

    // First, ensure any previous shared memory with the same name is gone.
    // This is important for robust startup, especially during development.
    shmring_destroy(&(shm_ring_t){ .name = {0} }); // A bit of a hack to destroy by name if needed, but shmring API requires a ring object. Let's create and destroy.
    shm_ring_t old_ring;
    strncpy(old_ring.name, name, sizeof(old_ring.name) - 1);
    shmring_destroy(&old_ring);


    // Initialize the shared memory ring buffer
    int r = shmring_create(&hub->ring, name, slots);
    if (r != 0) {
        return r;
    }

    hub->loop = loop;
    hub->on_data = on_data;
    hub->user_data = user_data;
    hub->async_handle.data = hub;

    // Initialize the async handle to wake up the loop
    r = uv_async_init(loop, &hub->async_handle, on_async_notification);
    if (r != 0) {
        shmring_destroy(&hub->ring);
        return r;
    }

    return 0;
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;

    // Close the async handle. It's important to do this to allow the event
    // loop to exit cleanly. The handle is freed automatically after the
    // close callback returns.
    if (!uv_is_closing((uv_handle_t*)&hub->async_handle)) {
        uv_close((uv_handle_t*)&hub->async_handle, NULL);
    }

    // Destroy the shared memory ring buffer
    shmring_destroy(&hub->ring);
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) {
        return -1;
    }

    // Write the message to the ring buffer
    shm_ring_write(&hub->ring, msg);

    // Send the notification. This is thread-safe.
    // uv_async_send will coalesce calls, so multiple publishes before
    // the loop runs will only result in one callback invocation.
    int r = uv_async_send(&hub->async_handle);
    return r;
}
