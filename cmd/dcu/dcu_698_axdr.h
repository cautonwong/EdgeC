#ifndef DCU_698_AXDR_H
#define DCU_698_AXDR_H

#include <stddef.h>
#include <stdint.h>

#include "dcu_698_object_model.h"

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
