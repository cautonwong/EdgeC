/**
 * @file dcu_698_group.h
 * @brief Class 23 总加组（2301..2308）纯软件领域引擎与滑差/A-XDR 编码
 */

#ifndef DCU_698_GROUP_H
#define DCU_698_GROUP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DCU_GROUP_COUNT 8
#define DCU_GROUP_OI_BASE 0x2301u
#define DCU_GROUP_MAX_MEMBERS 16
#define DCU_GROUP_SLIP_WINDOW 15
#define DCU_GROUP_TSA_MAX 17

typedef struct {
    uint32_t slot_id;
    uint8_t tsa[DCU_GROUP_TSA_MAX];
    uint8_t tsa_len;      /* 2..17；为 0 时按 slot_id 生成 TSA */
    uint8_t add_sub_flag; /* 0 = 加 (+1)，1 = 减 (-1) */
    uint8_t calc_flag;    /* 0 = 不参与，1 = 参与 */
} dcu_group_member_t;

typedef struct {
    int64_t active_power;        /* 当前有功功率（kW × 10000，即 0.1 W） */
    int64_t reactive_power;      /* 当前无功功率（kvar × 10000） */
    int64_t pos_active_energy;   /* 正向有功电能（kWh × 10000） */
    int64_t neg_active_energy;   /* 反向有功电能（kWh × 10000） */
    int64_t pos_reactive_energy; /* 正向无功电能（kvarh × 10000） */
    int64_t neg_reactive_energy; /* 反向无功电能（kvarh × 10000） */
} dcu_group_member_reading_t;

typedef struct {
    dcu_group_member_t members[DCU_GROUP_MAX_MEMBERS];
    uint8_t member_count;

    int64_t active_power;        /* A3：当前总加有功功率 */
    int64_t reactive_power;      /* A4：当前总加无功功率 */
    int64_t slip_active_power;   /* A5：有功滑差平均功率 */
    int64_t slip_reactive_power; /* A6：无功滑差平均功率 */
    int64_t pos_active_energy;   /* A7：总加正向有功电能 */
    int64_t neg_active_energy;   /* A8：总加反向有功电能 */
    int64_t pos_reactive_energy; /* A9：总加正向无功电能 */
    int64_t neg_reactive_energy; /* A10：总加反向无功电能 */

    int64_t slip_active_hist[DCU_GROUP_SLIP_WINDOW];
    int64_t slip_reactive_hist[DCU_GROUP_SLIP_WINDOW];
    uint8_t slip_head;
    uint8_t slip_count;

    uint64_t last_sample_s;
    bool has_sample;
} dcu_group_state_t;

typedef int (*dcu_group_read_fn)(void *ctx, const dcu_group_member_t *member,
                                 dcu_group_member_reading_t *out);

/** @brief 初始化/重置全部 8 个总加组（2301..2308）。 */
void dcu_698_group_init(void);

/** @brief 添加或更新一个总加配置单元（M3/M4 语义）；同地址/slot 覆盖，满 16 返回 -1。 */
int dcu_698_group_add_member(uint8_t group_idx, const dcu_group_member_t *member);

/** @brief 删除一个总加配置单元（M5 语义）；tsa 非空时按 TSA 匹配，否则按 slot_id 匹配。 */
int dcu_698_group_delete_member(uint8_t group_idx, uint32_t slot_id, const uint8_t *tsa,
                                uint8_t tsa_len);

/** @brief 清空指定总加组的成员与滑差历史（M1 语义）。 */
int dcu_698_group_clear(uint8_t group_idx);

/** @brief 采样计算与滑差窗口推进；无有效参与成员时返回 -1，不除零、不伪造。 */
int dcu_698_group_sample(uint8_t group_idx, dcu_group_read_fn read_fn, void *ctx, uint64_t now_s);

/** @brief 读取指定总加组的只读状态；group_idx >= 8 返回 NULL。 */
const dcu_group_state_t *dcu_698_group_get(uint8_t group_idx);

/** @brief 编码 Class 23 属性 A2 与 A3..A10 的 A-XDR 数据；cap 不足或无有效样本返回 -1。 */
int dcu_698_group_encode_attr(uint8_t group_idx, uint8_t attr_id, uint8_t *buf, size_t cap,
                              size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_GROUP_H */
