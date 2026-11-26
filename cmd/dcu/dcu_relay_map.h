/**
 * @file dcu_relay_map.h
 * @brief 产品点位表：继电器 OAD → 表地址 + DO 通道（OI 8000 方法 129/130/131）
 *
 * 表 A.9 用继电器 OAD 寻址，产品只有表地址与 DO 通道。表由聚合根拥有，点位表或
 * 测试通过 set 填充；未填时查不到，调用方按 DAR 2 诚实拒绝，不按 OAD 猜继电器。
 */

#ifndef DCU_RELAY_MAP_H
#define DCU_RELAY_MAP_H

#include <stddef.h>
#include <stdint.h>

#include "dcu_inventory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 固定容量；表满时 set 显式失败，不静默截断。 */
#define DCU_RELAY_MAP_CAPACITY 32

typedef struct {
    uint32_t relay_oad;            /* 表 A.9 的继电器 OAD，映射键 */
    char meter_addr[MAX_ADDR_LEN]; /* 目标表地址（dcu_inventory 的 TSA） */
    uint8_t do_channel;            /* 目标继电器 DO 通道号 */
} dcu_relay_entry_t;

typedef struct {
    dcu_relay_entry_t entries[DCU_RELAY_MAP_CAPACITY];
    size_t count;
} dcu_relay_map_t;

/** @brief 填一行；同一 relay_oad 覆盖。地址为空/超长或表满返回 -1。 */
int dcu_relay_map_set(dcu_relay_map_t *map, uint32_t relay_oad, const char *meter_addr,
                      uint8_t do_channel);

/** @brief 查表；未命中或表为空返回 NULL。 */
const dcu_relay_entry_t *dcu_relay_map_find(const dcu_relay_map_t *map, uint32_t relay_oad);

/** @brief 聚合根装配时绑定自己拥有的表；传 NULL 解绑。未绑定按空表处理。 */
void dcu_relay_map_bind(const dcu_relay_map_t *map);

/** @brief 当前绑定的表；未绑定返回 NULL。 */
const dcu_relay_map_t *dcu_relay_map_active(void);

#ifdef __cplusplus
}
#endif

#endif /* DCU_RELAY_MAP_H */
