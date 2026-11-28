/**
 * @file dcu_698_stat.h
 * @brief DL/T 698.45 Class 14/15/16（区间统计/累加平均/极值）纯软件统计引擎
 *
 * 覆盖 OI：
 * - Class 14 (0x2100..0x2104): 区间统计
 * - Class 15 (0x2110..0x2114): 累加平均
 * - Class 16 (0x2120..0x2124): 极值统计
 *
 * 零动态内存：全静态固定容量存储（每 OI 最多 DCU_STAT_MAX_ENTRIES 条）。
 */

#ifndef DCU_698_STAT_H
#define DCU_698_STAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dcu_698_object_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCU_STAT_MAX_ENTRIES 8
#define DCU_STAT_OI_COUNT 15

typedef struct {
    bool active;
    uint32_t src_oad;
    uint32_t period_s;
    uint32_t count;
    double sum;
    double avg;
    double min;
    double max;
    uint64_t min_ts_s;
    uint64_t max_ts_s;
    uint64_t last_ts_s;
    uint64_t first_ts_s;
} dcu_stat_entry_t;

typedef struct {
    uint8_t method;
    uint32_t src_oad;
    uint32_t period_s;
} dcu_stat_op_t;

/** @brief 判断 OI 是否属于 Class 14/15/16（0x2100..0x2104 / 0x2110..0x2114 / 0x2120..0x2124）。 */
bool dcu_698_stat_is_oi(uint16_t oi);

/** @brief 清空全部统计配置与运行期状态。 */
void dcu_698_stat_reset(void);

/** @brief 注入样本：更新所有匹配 src_oad 的统计条目。 */
void dcu_698_stat_ingest(uint32_t src_oad, double value, uint64_t ts_s);

/** @brief 只读查询指定 OI 的当前配置与统计条目数组。 */
int dcu_698_stat_get_entries(uint16_t oi, const dcu_stat_entry_t **out_entries, size_t *out_count);

/** @brief 编码指定 OAD（Attribute 2）的 A-XDR 载荷；未配置时返回空数组 01 00。 */
int dcu_698_stat_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);

/** @brief 解码 Class 14/15/16 Method 1/3/4 的 A-XDR 参数；形状非法返回 EDGE_SLAVE_SRC_TYPE_MISMATCH。 */
int dcu_698_stat_decode_op(uint16_t oi, uint8_t method, const uint8_t *axdr, size_t len,
                           dcu_stat_op_t *out);

/** @brief 对象模型读取回调（Attribute 2）。 */
int dcu_698_stat_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                      void *out_val);

/** @brief 对象模型方法回调（Method 1 复位 / Method 3 添加 / Method 4 删除）。 */
int dcu_698_stat_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                         void *result);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_STAT_H */
