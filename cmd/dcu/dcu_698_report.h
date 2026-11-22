/**
 * @file dcu_698_report.h
 * @brief DCU 698 从站主动上报队列（ADR-0017）
 *
 * 事件入库时把"哪个对象属性变了"放进固定大小的队列；698 从站引擎的
 * 周期发帧钩子从队列取一条，按 REPORT-Notification 对象列表上报。
 */

#ifndef DCU_698_REPORT_H
#define DCU_698_REPORT_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief 入队一条待上报的对象属性（标量）。
 *
 * 线程安全：事件入库在 stream engine 线程，出队在 IO 事件循环线程。
 * @param oad      对象属性描述符
 * @param type_tag A-XDR 标量类型标签（见 DCU_698_TAG_*）
 * @param raw      原始整数值（按类型宽度编码）
 */
void dcu_698_report_enqueue(uint32_t oad, uint8_t type_tag, int64_t raw);

/**
 * @brief 主动上报端口（`edge_slave_report_ops_t.next`）。
 *
 * 写一帧上报的 APDU 体：CHOICE | PIID | 个数 | 结果项；无待报内容时 `*len=0`。
 */
int dcu_698_report_next(void *ctx, uint8_t *body, size_t cap, size_t *len);

/** @brief 清空队列与 PIID 计数（测试用）。 */
void dcu_698_report_reset(void);

#endif /* DCU_698_REPORT_H */
