/**
 * @file iems_wal_start.c
 * @brief WAL 启动失败时明确降级，产品继续运行。
 */

#include "edge_log.h"
#include "iems_app.h"

int iems_app_wal_start(datahub_t *hub, const char *wal_path) {
    if (!hub || !wal_path) return -1;
    if (datahub_enable_wal(hub, wal_path) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "IEMS: WAL disabled, cannot open %s", wal_path);
        datahub_disable_wal(hub);
        return -1;
    }
    if (datahub_replay_wal(hub) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "IEMS: WAL disabled, replay failed for %s", wal_path);
        datahub_disable_wal(hub);
        return -1;
    }
    return 0;
}
