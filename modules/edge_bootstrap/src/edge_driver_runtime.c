#include "edge_driver_runtime.h"

#include <string.h>

int edge_driver_runtime_init(edge_driver_runtime_t *runtime) {
    if (!runtime) return -1;
    memset(runtime, 0, sizeof(*runtime));
    runtime->manager = driver_manager_create_in(&runtime->manager_storage);
    if (!runtime->manager) return -1;

    const edge_driver_registry_port_t *registry =
        driver_manager_registry_port_ptr(runtime->manager);
    const edge_driver_session_port_t *session = driver_manager_session_port_ptr(runtime->manager);
    if (!registry || !session) {
        driver_manager_destroy(runtime->manager);
        memset(runtime, 0, sizeof(*runtime));
        return -1;
    }
    runtime->registry = *registry;
    runtime->session = *session;
    return 0;
}

void edge_driver_runtime_destroy(edge_driver_runtime_t *runtime) {
    if (!runtime) return;
    if (runtime->manager) driver_manager_destroy(runtime->manager);
    memset(runtime, 0, sizeof(*runtime));
}
