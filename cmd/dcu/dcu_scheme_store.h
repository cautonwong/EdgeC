#ifndef DCU_SCHEME_STORE_H
#define DCU_SCHEME_STORE_H

#include <stdbool.h>
#include <stdint.h>

#include "dcu_poller.h"
#include "dcu_storage.h"

/** @brief 建表 + 首次启动播种默认方案（ID 1），只播种一次；new DB 由调用方给默认 tag。 */
int dcu_scheme_store_init(dcu_storage_t *storage, const uint32_t *default_tags,
                          uint16_t default_count);

/** @brief 把库里的方案装进 poller；坏行跳过并告警，不清库。装载前先清 RAM。 */
int dcu_scheme_store_load(dcu_storage_t *storage, dcu_poller_t *poller);

/** @brief 6014 方法 127/128/129/130 的事务化落地；失败回滚 DB 与 RAM。 */
int dcu_scheme_store_apply(dcu_storage_t *storage, dcu_poller_t *poller, const dcu_scheme_op_t *op);

#define DCU_MAX_EVENT_SCHEMES 16
#define DCU_MAX_EVENT_ROADS 8
#define DCU_ROAD_RELATED_MAX 4
#define DCU_TSA_MAX 17

typedef enum {
    DCU_EVENT_CAPTURE_PERIODIC = 0,
    DCU_EVENT_CAPTURE_NOTIFY_ALL = 1,
    DCU_EVENT_CAPTURE_NOTIFY_LIST = 2
} dcu_event_capture_t;

typedef struct {
    uint32_t oad;
    uint8_t related_count;
    uint32_t related[DCU_ROAD_RELATED_MAX];
} dcu_event_road_t;

typedef struct {
    uint8_t scheme_id;
    dcu_event_capture_t capture_type;
    bool report_flag;
    uint16_t storage_depth;
    uint8_t ms_choice;
    uint8_t road_count;
    dcu_event_road_t roads[DCU_MAX_EVENT_ROADS];
} dcu_event_scheme_cfg_t;

/** @brief 6016 方法 127/128/129/130 的解码结果；大结构，放 store 暂存区。 */
typedef struct {
    uint8_t method;
    uint8_t unit_count;
    uint8_t id_count;
    uint8_t scheme_id;
    bool report_flag;
    dcu_event_scheme_cfg_t units[DCU_MAX_EVENT_SCHEMES];
    uint8_t ids[DCU_MAX_EVENT_SCHEMES];
} dcu_event_scheme_op_t;

#define DCU_MAX_TRANSPARENT_SCHEMES 16
#define DCU_MAX_TRANSPARENT_CONTENTS 4
#define DCU_MAX_TRANSPARENT_FRAMES 4
#define DCU_MAX_TRANSPARENT_FRAME_LEN 255 /* octet-string 长度域是一字节 */
#define DCU_MAX_RESULT_ITEMS 8

typedef struct {
    uint8_t len;
    uint8_t bytes[DCU_TSA_MAX];
} dcu_tsa_t;

typedef struct {
    uint8_t seq;
    uint16_t len;
    uint8_t data[DCU_MAX_TRANSPARENT_FRAME_LEN];
} dcu_transparent_frame_t;

typedef struct {
    bool report_and_wait;
    uint16_t wait_timeout_s;
    uint8_t compare_mode;
    uint8_t feature_byte;
    uint16_t cut_start;
    uint16_t cut_len;
} dcu_transparent_ctrl_t;

typedef struct {
    uint16_t seq;
    dcu_tsa_t address;
    uint16_t pre_script;
    uint16_t post_script;
    dcu_transparent_ctrl_t ctrl;
    uint8_t frame_count;
    dcu_transparent_frame_t frames[DCU_MAX_TRANSPARENT_FRAMES];
} dcu_transparent_content_t;

typedef struct {
    uint8_t scheme_id;
    uint16_t storage_depth;
    uint8_t content_count;
    dcu_transparent_content_t contents[DCU_MAX_TRANSPARENT_CONTENTS];
} dcu_transparent_scheme_t;

/** @brief 6018 方法 127/128/129/130/131 的解码结果；大结构，放 store 暂存区。 */
typedef struct {
    uint8_t method;
    uint8_t id_count;
    uint8_t addr_count;
    uint8_t scheme_id;
    dcu_transparent_scheme_t scheme;
    dcu_transparent_content_t content;
    dcu_tsa_t addresses[DCU_MAX_TRANSPARENT_CONTENTS];
    uint8_t ids[DCU_MAX_TRANSPARENT_SCHEMES];
} dcu_transparent_op_t;

typedef struct {
    uint8_t seq;
    uint64_t resp_time;
    uint16_t len;
    uint8_t data[DCU_MAX_TRANSPARENT_FRAME_LEN];
} dcu_transparent_result_item_t;

typedef struct {
    uint32_t result_id; /* 存储序号：601B 的元素索引，1 起，0 = 最小现有 */
    uint8_t scheme_id;
    uint64_t exec_time;
    dcu_tsa_t address;
    uint8_t item_count;
    dcu_transparent_result_item_t items[DCU_MAX_RESULT_ITEMS];
} dcu_transparent_result_t;

/** @brief 解码大结构时使用的暂存区（单线程 uv 循环，和 6014 的 poller->scheme_op 同归属）。 */
dcu_event_scheme_op_t *dcu_scheme_store_event_op(void);
dcu_transparent_op_t *dcu_scheme_store_transparent_op(void);
dcu_transparent_result_t *dcu_scheme_store_result_buf(void);

/** @brief 6016 方法 127/128/129/130 的事务化落地；返回 0 或 EDGE_SLAVE_SRC_*。 */
int dcu_scheme_store_event_apply(const dcu_event_scheme_op_t *op);

/** @brief 6018 方法 127/128/129/130/131 的事务化落地；返回 0 或 EDGE_SLAVE_SRC_*。 */
int dcu_scheme_store_transparent_apply(const dcu_transparent_op_t *op);

/** @brief 追加一条透明结果；返回 0 或 EDGE_SLAVE_SRC_*。 */
int dcu_scheme_store_result_append(const dcu_transparent_result_t *r);

int dcu_scheme_store_event_count(void);
/** @brief 按编号升序取第 pos 个；pos 越界回 NOT_FOUND。 */
int dcu_scheme_store_event_at(uint16_t pos, dcu_event_scheme_cfg_t *out);
int dcu_scheme_store_event_get(uint8_t scheme_id, dcu_event_scheme_cfg_t *out);

int dcu_scheme_store_transparent_count(void);
int dcu_scheme_store_transparent_at(uint16_t pos, dcu_transparent_scheme_t *out);
int dcu_scheme_store_transparent_get(uint8_t scheme_id, dcu_transparent_scheme_t *out);

int dcu_scheme_store_result_count(void);
int dcu_scheme_store_result_at(uint16_t pos, dcu_transparent_result_t *out);
/** @brief 元素索引 → 结果：ordinal 1 起，0 = 最小现有存储序号。 */
int dcu_scheme_store_result_get(uint16_t ordinal, dcu_transparent_result_t *out);

#define DCU_MAX_REPORT_SCHEMES 16
#define DCU_MAX_REPORT_CHANNELS 4

typedef enum {
    DCU_REPORT_CONTENT_OAD = 0,
    DCU_REPORT_CONTENT_RECORD = 1   /* 记录型上报：产品无存储/执行路径 */
} dcu_report_content_t;

typedef struct {
    uint8_t scheme_id;
    uint8_t channel_count;
    uint32_t channels[DCU_MAX_REPORT_CHANNELS];
    uint8_t timeout_unit;
    uint16_t timeout_value;
    uint8_t max_times;
    dcu_report_content_t content_type;
    uint32_t content_oad;
} dcu_report_scheme_cfg_t;

typedef struct {
    uint8_t method;
    uint8_t unit_count;
    uint8_t id_count;
    dcu_report_scheme_cfg_t units[DCU_MAX_REPORT_SCHEMES];
    uint8_t ids[DCU_MAX_REPORT_SCHEMES];
} dcu_report_op_t;

dcu_report_op_t *dcu_scheme_store_report_op(void);
/** @brief 601C 方法 127/128/129 的事务化落地；返回 0 或 EDGE_SLAVE_SRC_*。 */
int dcu_scheme_store_report_apply(const dcu_report_op_t *op);
int dcu_scheme_store_report_count(void);
int dcu_scheme_store_report_at(uint16_t pos, dcu_report_scheme_cfg_t *out);
int dcu_scheme_store_report_get(uint8_t scheme_id, dcu_report_scheme_cfg_t *out);

#endif /* DCU_SCHEME_STORE_H */
