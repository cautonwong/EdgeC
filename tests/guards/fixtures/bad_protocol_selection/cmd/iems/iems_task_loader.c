#include "iems_task_loader.h"

static int load_tasks_cb(void *user_data, int argc, char **argv, char **cols) {
    (void)user_data;
    (void)argv;
    (void)cols;
    if (argc < 4) return 0;

    const char *proto = "MODBUS";
    iems_loaded_task_t task = {0};
    strncpy(task.protocol, proto, sizeof(task.protocol) - 1);
    return 0;
}
