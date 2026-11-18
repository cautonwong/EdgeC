#include "iems_driver_adapter.h"
#include "iems_task_loader.h"

static int load_tasks_cb(void *user_data, int argc, char **argv, char **cols) {
    (void)user_data;
    (void)cols;
    if (argc < 4) return 0;

    iems_loaded_task_t task = {0};
    if (iems_driver_protocol_canonical(argv[3], task.protocol) != 0) return 0;
    return 0;
}
