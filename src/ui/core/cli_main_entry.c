#include "datahub.h"
#include "edge_ui.h"
#include "edge_log.h"
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // In a real scenario, the UI process would need to discover and
    // attach to the existing datahub's shared memory.
    // This requires a discovery mechanism (e.g., a well-known unix socket path)
    // to get the file descriptor for the shmring.
    // For this implementation, we will assume a simplified scenario
    // where we can initialize a new datahub for demonstration.
    // The architecture proposal correctly implies linking against edge_core
    // to act as a client to the main datahub.

    printf("Starting UI Companion Process...\n");
    printf("NOTE: This is a simplified entry point.\n");
    printf("A real implementation needs to connect to the main process's datahub.\n");

    uv_loop_t *loop = uv_default_loop();

    // Placeholder: A real client would get the FD from the main process
    // and use datahub_init_from_fd().
    datahub_t hub;
    datahub_init(&hub, loop, "ui_bus", 1024, 0, NULL, 0, NULL, NULL, NULL);

    ui_service_t *ui = ui_service_create(&hub);
    if (!ui) {
        LOG_FATAL(LOG_CAT_UI, "Failed to create UI service.");
        return -1;
    }

    ui_service_run(ui);

    ui_service_destroy(ui);
    datahub_destroy(&hub);

    printf("UI Companion Process terminated.\n");

    return 0;
}
