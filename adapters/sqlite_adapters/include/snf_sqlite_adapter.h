#ifndef SNF_SQLITE_ADAPTER_H
#define SNF_SQLITE_ADAPTER_H

#include "snf_manager.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build a SnF store port backed by an existing sqlite service.
 *
 * 墙钟语义 (P4-03b1):
 * - `telemetry.wall_ts_us` 是记录墙钟，单位 epoch 微秒 (CLOCK_REALTIME)，由
 *   `snf_manager_buffer_data()` 在入库边界采样一次并在同一微秒内按 +1 钳制。
 * - 主键与补发顺序是 `(wall_ts_us, tag_id)`；`mark_synced` 用同一身份。`ts`
 *   列仍保存单调时间纳秒，只供排障与旧查询使用，不能当公历时间。
 * - 旧库处理：schema v18 迁移会 DROP 并重建 telemetry。本表没有 shipped 数据，
 *   重跑即建新表（旧行丢弃）；没有 wall_ts_us 列的库会被 `ensure_schema` 拒绝。
 */
snf_store_port_t snf_sqlite_store_port(sqlite_svc_t *db);

#ifdef __cplusplus
}
#endif

#endif /* SNF_SQLITE_ADAPTER_H */
