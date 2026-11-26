#ifndef DCU_698_FREEZE_H
#define DCU_698_FREEZE_H

#include <stdbool.h>
#include <stdint.h>

#include "dcu_clock_port.h"
#include "dcu_inventory.h"
#include "dcu_storage.h"

struct dcu_freeze_cfgs;

#define DCU_FREEZE_MAX_SOURCES 4
#define DCU_FREEZE_OI_COUNT 8 /* 5000..5007 */
#define DCU_FREEZE_ENTRY_MAX 8
#define DCU_FREEZE_PENDING_MAX 8

typedef int (*dcu_freeze_read_fn)(void *ctx, uint32_t slot_id, uint32_t source_oad, double *out);

typedef struct {
    bool used;
    bool warned;
    uint16_t oi;
    uint64_t due_ms;
} dcu_freeze_pending_t;

typedef struct {
    dcu_storage_t *storage;
    dcu_inventory_t *inventory;
    dcu_freeze_read_fn read;
    void *read_ctx;
    const dcu_clock_port_t *clock;
    const struct dcu_freeze_cfgs *cfgs;
    uint32_t sources[DCU_FREEZE_MAX_SOURCES];
    uint8_t source_count;
    uint64_t last_fire[DCU_FREEZE_OI_COUNT][DCU_FREEZE_ENTRY_MAX]; /* [oi-5000][属性3条目] */
    dcu_freeze_pending_t pending[DCU_FREEZE_PENDING_MAX];
    uint64_t last_boundary_epoch;
    uint64_t last_align_delta_ms;
    bool primed;
} dcu_freeze_scheduler_t;

void dcu_freeze_scheduler_init(dcu_freeze_scheduler_t *s, dcu_storage_t *storage,
                               dcu_inventory_t *inventory, dcu_freeze_read_fn read, void *read_ctx,
                               const dcu_clock_port_t *clock);
void dcu_freeze_scheduler_set_sources(dcu_freeze_scheduler_t *s, const uint32_t *oads, uint8_t n);
void dcu_freeze_scheduler_set_configs(dcu_freeze_scheduler_t *s, const struct dcu_freeze_cfgs *cfgs);

/* 方法 3：按当前时钟立即冻结该 OI 的关联对象；返回写入行数，<0 = 无法触发。 */
int dcu_freeze_trigger(dcu_freeze_scheduler_t *s, uint16_t oi);

/* 方法 3 延时触发：到期由现有 uv 定时器驱动；池满回 EDGE_SLAVE_SRC_TEMPORARY。 */
int dcu_freeze_schedule(dcu_freeze_scheduler_t *s, uint16_t oi, uint32_t delay_s);
uint64_t dcu_freeze_pending_next_due_ms(const dcu_freeze_scheduler_t *s);

/* 按给定墙钟秒先处理到期的延时触发（方法 3），再触发所有已到的标准边界（5001–5007）；
 * 返回写入的冻结记录数。 */
int dcu_freeze_tick(dcu_freeze_scheduler_t *s, uint64_t now_epoch_s);

/* 下一个标准边界（秒）；0 = 无。用于把 uv_timer 对齐到边界而不是固定节拍。 */
int dcu_freeze_next_boundary(dcu_freeze_scheduler_t *s, uint64_t now_epoch_s, uint64_t *boundary_s);

#endif /* DCU_698_FREEZE_H */
