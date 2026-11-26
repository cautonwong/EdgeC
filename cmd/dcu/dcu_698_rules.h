#ifndef DCU_698_RULES_H
#define DCU_698_RULES_H

#include <stdint.h>

#include "dcu_storage.h"
#include "edge_slave_plane.h"

#define DCU_RULE_CACHE_MAX 128
#define DCU_RULE_BATCH_MAX 16
#define DCU_RULE_FRAME_MAX 255 /* octet-string 长度域是一字节 */

typedef enum {
    DCU_RULE_DLT645_2007 = 1,
    DCU_RULE_DLT645_1997 = 2,
    DCU_RULE_DLT698 = 3,
    DCU_RULE_MODBUS = 4,
    DCU_RULE_TRANS = 5,
} dcu_rule_protocol_t;

typedef enum {
    DCU_RULE_MAIN = 0,
    DCU_RULE_SPARE = 1,
} dcu_rule_role_t;

typedef struct {
    uint32_t oad;
    uint32_t protocol_id;
    float scale;
    uint8_t protocol;
    uint8_t role;
    uint8_t length;
} dcu_rule_row_t;

typedef struct {
    uint16_t count;
    dcu_rule_row_t rows[DCU_RULE_CACHE_MAX];
} dcu_rule_cache_t;

/* 601F 规则单元：标准线缆只有 645-2007/1997 的主/备 DI 与透明帧；产品内部行另含 698/Modbus。 */
typedef struct {
    uint32_t oad; /* 内部消费侧 tag（规则表主键） */
    uint32_t csd; /* 主站可见的 698 CSD OAD；相别行与 oad 不同 */
    uint32_t di_2007_main;
    uint32_t di_2007_spare;
    uint16_t di_1997_main;
    uint16_t di_1997_spare;
    uint8_t has_2007_main;
    uint8_t has_2007_spare;
    uint8_t has_1997_main;
    uint8_t has_1997_spare;
    uint8_t has_trans;
    uint8_t frame_len;
    uint8_t frame[DCU_RULE_FRAME_MAX];
} dcu_rule_unit_t;

typedef struct {
    uint8_t method; /* 127 Add / 128 Delete / 129 Clear */
    uint8_t unit_count;
    uint8_t csd_count;
    dcu_rule_unit_t units[DCU_RULE_BATCH_MAX];
    uint32_t csds[DCU_RULE_BATCH_MAX];
} dcu_rule_op_t;

/* 单线程所有权：cache 由 dcu_node 内嵌分配，组合根 init 一次；调用点都在 node 的 uv loop。
 * detach 由持有者在释放 cache 内存前调用，之后查找回落到内置默认点表。 */
int dcu_698_rules_init(dcu_storage_t *storage, dcu_rule_cache_t *cache);
void dcu_698_rules_detach(void);
int dcu_698_rules_find_by_oad(uint8_t protocol, uint32_t oad, dcu_rule_row_t *out);
int dcu_698_rules_find_by_proto(uint8_t protocol, uint32_t protocol_id, dcu_rule_row_t *out);

/* 601E/601F 管理面：投影只含协议 1/2/5，按 OAD 升序；601E 数组与 601F 元素共用此投影。 */
dcu_rule_op_t *dcu_698_rules_op(void);
int dcu_698_rules_apply(const dcu_rule_op_t *op);
int dcu_698_rules_projected_count(uint16_t *out);
int dcu_698_rules_projected_oad(uint16_t pos, uint32_t *out);
int dcu_698_rules_unit_load(uint32_t oad, dcu_rule_unit_t *out);

/* 645 主用 DI 失败后是否用备用 DI 重试一次；只读规则快照，不碰 SQLite。 */
int dcu_698_rules_retry_with_spare(const char *protocol, uint32_t oad, int already_retried,
                                   uint32_t *spare_protocol_id);

#endif /* DCU_698_RULES_H */
