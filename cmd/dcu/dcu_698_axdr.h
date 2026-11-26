#ifndef DCU_698_AXDR_H
#define DCU_698_AXDR_H

#include <stddef.h>
#include <stdint.h>

#include "dcu_698_object_model.h"
#include "dcu_698_rules.h"
#include "dcu_scheme_store.h"

#ifdef __cplusplus
extern "C" {
#endif

int dcu_north_resolve_axdr(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
int dcu_north_accept_axdr(void *ctx, uint32_t oad, const uint8_t *axdr, size_t len);

/**
 * @brief 携带长度的原始 ACTION 入口（D1-20a 的 rebase 调用这一行）。
 *
 * 先查对象表（DAR 6/5/4 的单一口径），再严格解码 6012 的 A-XDR 参数
 * （截断/错标签/错个数/尾随字节 → DAR 7），最后把类型化操作交给处理器。
 * 形状错误在触碰 poller/SQL 之前返回。
 */
int dcu_698_accept_action_axdr(uint32_t omd, const uint8_t *axdr, size_t axdr_len);

/** @brief 6012 A2 数组 / 6013 A2 单元（元素索引 = task_id，0 = 最小现有 ID）。 */
int dcu_698_task_encode_axdr(const dcu_poller_t *poller, uint32_t oad, uint8_t *buf, size_t cap,
                             size_t *len);

/* CSD/MS 是 698 的构造类型，线上标签只有本标准一处使用。 */
#define DCU_698_TAG_CSD 0x5B
#define DCU_698_TAG_MS 0x5C

/**
 * @brief 解 6014 方法 127/128/129/130 的参数。
 *
 * 截断、错标签、错个数、尾随字节、坏 CHOICE、采集类型 > 4、MS choice > 7
 * → DAR 7。数组超过 MAX_SCHEMES / MAX_SCHEME_OADS → DAR 2（容量）。
 * 标准允许但产品不执行的取值解码成功，由 store 报 DAR 2。
 */
int dcu_698_decode_scheme_op(uint8_t method, const uint8_t *buf, size_t len,
                             dcu_scheme_op_t *out);

/**
 * @brief 6014 A2 数组 / 6015 A2 单元（元素索引 = 方案编号，0 = 最小现有 ID）。
 *
 * CSD 编成 `5B 00 + 4 字节 OAD`，MS 编成 `5C 01`。缓冲区不够返回 DAR 2，不截断。
 */
int dcu_698_scheme_encode_axdr(const dcu_poller_t *poller, uint32_t oad, uint8_t *buf,
                               size_t cap, size_t *len);
/** @brief 解 6016 方法 127/128/129/130 的参数；形状错误 DAR 7，容量超限 DAR 2。 */
int dcu_698_decode_event_scheme_op(uint8_t method, const uint8_t *buf, size_t len,
                                   dcu_event_scheme_op_t *out);
/** @brief 6016 A2 数组 / 6017 A2 单元（元素索引 = 方案编号，0 = 最小现有 ID）。 */
int dcu_698_event_scheme_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
/** @brief 解 6018 方法 127/128/129/130/131 的参数；形状错误 DAR 7，容量超限 DAR 2。 */
int dcu_698_decode_transparent_op(uint8_t method, const uint8_t *buf, size_t len,
                                  dcu_transparent_op_t *out);
/** @brief 6018 A2 数组 / 6019 A2 单元（元素索引 = 方案编号，0 = 最小现有 ID）。 */
int dcu_698_transparent_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
/** @brief 601A A2 数组 / 601B A2 单元（元素索引 = 结果存储序号，0 = 最小现有）。 */
int dcu_698_transparent_result_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
/** @brief 解 601C 方法 127/128/129 的参数；形状错误 DAR 7，记录型上报/容量超限 DAR 2。 */
int dcu_698_decode_report_op(uint8_t method, const uint8_t *buf, size_t len, dcu_report_op_t *out);
/** @brief 601C A2 数组 / 601D A2 单元（元素索引 = 方案编号，0 = 最小现有 ID）。 */
int dcu_698_report_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
/** @brief 解 601E 方法 127/128/129 的参数；ROAD/多 DI 数组 DAR 2，形状错误 DAR 7。 */
int dcu_698_decode_rule_op(uint8_t method, const uint8_t *buf, size_t len, dcu_rule_op_t *out);
/** @brief 601E A2 数组 / 601F A2 单元（元素索引 = 601E 投影位置，0 = 第一项）。 */
int dcu_698_rule_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
/** @brief 6034 A2 数组 / 6035 A2 单元（元素索引 = task_id）；未知 task_id 回 DAR 6。 */
int dcu_698_task_monitor_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);

/**
 * @brief 编码 date_time_s：UNIX 整秒 → tag 0x1C + 设备本地公历 7 字节。
 *
 * 表49 的 date_time_s 不带时区，主站看到的是设备本地民用时间，所以这里经
 * `localtime_r` 展开，进程 TZ 即设备配置的时区（部署时必须配置）。
 * 只接受 [2000, 2100) 的秒值；越界返回 -1，避免把单调纳秒或 0 编成假的公历日期。
 */
int dcu_698_put_date_time_s(uint64_t epoch_s, uint8_t *buf, size_t cap, size_t *off);

/** @brief 解码 date_time_s / date_time（0x1C / 0x19），按设备本地时区经 `mktime` 返回 UNIX 整秒。
 */
int dcu_698_date_time_s_to_epoch(const uint8_t *data, uint8_t len, uint64_t *out);

/**
 * @brief 秒值是否落在可编 date_time_s 的公历区间 [2000, 2100)。
 *
 * event_history 旧行存的是 uv_hrtime 单调纳秒，不是 epoch 秒。幅度不能可靠区分两者
 * （开机 1 到 4 秒的纳秒值正好落在 epoch 秒区间内），所以未来任何读取路径必须先用
 * 版本/单位字段识别旧行；这里的区间判断只是最后一道拒绝，不是迁移依据。
 */
int dcu_698_epoch_is_civil(uint64_t epoch_s);

/**
 * @brief 解 A.9 的 OI 8000 方法参数。
 *
 * 127/128 要求 NULL；129/130/131 要求 array<structure>，每项字段数与类型必须精确匹配。
 * 形状不符返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`；方法不在 127…131 返回 NOT_FOUND。
 */
int dcu_698_decode_action_param(uint8_t method, const uint8_t *buf, size_t len,
                                dcu_action_param_t *out);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_AXDR_H */
