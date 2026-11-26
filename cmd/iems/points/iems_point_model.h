/**
 * @file iems_point_model.h
 * @brief 点位/设备模型骨架：点键、槽位校验与已知名表 (P1-02)。
 *
 * 一个点由 (device_id, dense_idx) 定位，DataHub 影子槽位是
 * device_id * 16 + dense_idx，合法槽位 [0, 1024)。dense_idx 上限 15
 * 与 iems_tag_map_fn 的每设备步长一致；本模型把 device_id 限在 63 以内，
 * 越界直接拒绝，不回绕，避免落进别的设备或应用槽区
 * （见 /home/edge/migration/datahub-capacity.md 的 1,024 槽实测）。
 *
 * 名称表静态只读，无动态分配。条目分两类：
 *   - tag_id != IEMS_POINT_TAG_NONE：EdgeC 14 条元数据短名，名字与 tag
 *     双向可查，dense_idx 与 iems_tag_map_fn 一致；
 *   - tag_id == IEMS_POINT_TAG_NONE：iems_v3 旧名样例，已确认存在但缺
 *     模板/点表输入，按未知名返回 -1；B1-B4 输入到位后再补 tag_id。
 * iems_v3 的 424 个 dev.node 全名与 14 条短名当前 0 匹配
 * （/home/edge/migration/tag-config-map.md §2.3），所以这里不伪造
 * dev.node 映射。
 *
 * 重名策略：表是唯一实例，tests/test_iems_point_model.c 逐对断言名字与
 * 已映射 tag 唯一，因此重复名不可能进入查找结果。lookup 返回唯一匹配，
 * 不返回 -2；新增条目破坏唯一性时测试先失败。
 */

#ifndef IEMS_POINT_MODEL_H
#define IEMS_POINT_MODEL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IEMS_POINT_DENSE_PER_DEVICE 16u
#define IEMS_POINT_MAX_DEVICES 64u
#define IEMS_POINT_MAX_SLOTS (IEMS_POINT_MAX_DEVICES * IEMS_POINT_DENSE_PER_DEVICE)

/* tag 0 未被任何协议占用（现有 id 从 1001 起），留作未映射哨兵。 */
#define IEMS_POINT_TAG_NONE 0u
#define IEMS_POINT_DENSE_NONE 0xFFFFu

/** @brief 点位键：设备内稠密索引，不是全局 tag_id。 */
typedef struct {
    uint16_t device_id; /* 0..63 */
    uint16_t dense_idx; /* 0..15 */
} iems_point_key_t;

/** @brief 名称表条目；dense_idx 只在 tag_id 已映射时有意义。 */
typedef struct {
    const char *name;
    uint32_t tag_id;
    uint16_t dense_idx;
} iems_point_name_entry_t;

extern const iems_point_name_entry_t g_iems_point_names[];
extern const size_t g_iems_point_names_len;

/**
 * @brief 计算点键的 DataHub 槽位。
 * @return 0 成功；-1 空指针或 device_id/dense_idx 越界。失败不写 slot，
 *         不回绕。
 */
int iems_point_key_slot(const iems_point_key_t *key, uint32_t *slot);

/**
 * @brief 按旧名精确查 tag_id（不做大小写、下划线或点号归一化）。
 * @return 0 成功；-1 未知名、未映射旧名、空指针或 tag_id 为 NULL。
 *         失败不写输出。
 */
int iems_point_lookup_name(const char *name, uint32_t *tag_id);

/**
 * @brief 按 tag_id 查名字。
 * @return 0 成功；-1 未知 tag、未映射哨兵或 name 为 NULL。失败不写输出。
 */
int iems_point_lookup_tag(uint32_t tag_id, const char **name);

/**
 * @brief 已知 tag 的槽位。tag 本身不带设备维，这里返回设备 0 的槽位，
 *        即 dense_idx；带设备的槽位用 iems_point_key_slot。
 * @return 0 成功；-1 未知 tag、未映射、空指针或 tag_id 为 NULL。
 *         失败不写输出。
 */
int iems_point_slot(uint32_t tag_id, uint32_t *slot);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_POINT_MODEL_H
