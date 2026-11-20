#include "iems_driver_adapter.h"
#include "iems_task_loader.h"

static int load_tasks_cb(void *user_data, int argc, char **argv, char **cols) {
    (void)user_data;
    (void)cols;
    if (argc < 4) return 0;

    // const char *proto = "MODBUS"; 旧写法已删,协议必须来自 argv[3]
    iems_loaded_task_t task = {0};
    return iems_driver_protocol_canonical(argv[3], task.protocol);
}
