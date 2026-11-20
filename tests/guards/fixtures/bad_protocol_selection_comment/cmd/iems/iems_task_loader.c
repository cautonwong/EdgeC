#include "iems_task_loader.h"

static int load_tasks_cb(void *user_data, int argc, char **argv, char **cols) {
    (void)user_data;
    (void)cols;
    if (argc < 4) return 0;

    /* 协议在 argv[3];这条注释不能替代码作证 */
    iems_loaded_task_t task = {0};
    snprintf(task.protocol, sizeof(task.protocol), "%s", argv[2] ? argv[2] : "");
    return 0;
}
