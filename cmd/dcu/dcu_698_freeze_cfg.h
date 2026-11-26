#ifndef DCU_698_FREEZE_CFG_H
#define DCU_698_FREEZE_CFG_H

#include <stdbool.h>
#include <stdint.h>

#include "dcu_698_freeze.h"
#include "dcu_698_object_internal.h"
#include "dcu_storage.h"

#define DCU_FREEZE_OIS 8 /* 5000..5007 */
#define DCU_FREEZE_CFG_MAX DCU_FREEZE_ENTRY_MAX

typedef struct {
    uint32_t oad;
    uint32_t period; /* 0 = 非自动，只由方法 3/外部事件触发 */
    uint32_t depth;
} dcu_freeze_attr_t;

typedef struct {
    uint16_t oi;
    bool configured; /* 主站动过属性 3：不再套默认集 */
    bool truncated;  /* 库里有超过上限的历史行，加载时被忽略 */
    uint8_t count;
    dcu_freeze_attr_t items[DCU_FREEZE_CFG_MAX];
} dcu_freeze_cfg_t;

typedef struct dcu_freeze_cfgs {
    dcu_freeze_cfg_t ois[DCU_FREEZE_OIS];
} dcu_freeze_cfgs_t;

typedef struct {
    uint8_t method;
    uint8_t count;
    dcu_freeze_attr_t items[DCU_FREEZE_CFG_MAX];
    uint32_t oad;     /* 方法 5 */
    uint32_t delay_s; /* 方法 3 */
} dcu_freeze_op_t;

/* 未配置时的缺省：周期 1、三源、深度 1。 */
void dcu_freeze_cfgs_init(dcu_freeze_cfgs_t *cfgs);
int dcu_freeze_cfgs_load(dcu_freeze_cfgs_t *cfgs, dcu_storage_t *storage);
const dcu_freeze_cfg_t *dcu_freeze_cfg_find(const dcu_freeze_cfgs_t *cfgs, uint16_t oi);

/* 组合根绑定后的属性 3 快照；未绑定时返回 NULL（不伪造默认）。 */
const dcu_freeze_cfg_t *dcu_698_freeze_cfg_get(uint16_t oi);
dcu_freeze_cfg_t *dcu_freeze_cfg_mut(dcu_freeze_cfgs_t *cfgs, uint16_t oi);

/* 属性 3 的 A-XDR：array<structure{ 周期 lu, OAD, 深度 lu }>。 */
int dcu_freeze_cfg_encode(const dcu_freeze_cfg_t *cfg, uint8_t *buf, size_t cap, size_t *len);

/* 方法 3/4/5/7/8 的参数解码与应用；应用后缓存与 SQLite 一致。 */
int dcu_698_freeze_decode_op(uint8_t method, const uint8_t *axdr, size_t len, dcu_freeze_op_t *op);
int dcu_698_freeze_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                           void *result);

/* 组合根注入；方法 3 需要调度器，属性 3 需要存储。 */
void dcu_698_freeze_bind(dcu_storage_t *storage, dcu_freeze_cfgs_t *cfgs,
                         dcu_freeze_scheduler_t *sched);

#endif /* DCU_698_FREEZE_CFG_H */
