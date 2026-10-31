#ifndef EDGE_API_H
#define EDGE_API_H

#include "bus_orchestrator.h"
#include "req_driver_legacy.h"
#include "req_raw_io.h"

/**
 * @brief Initializes the high-level IO API with the core orchestrator.
 *
 * @param orch The main bus orchestrator instance.
 * @param drv_mgr The main driver manager instance.
 */
void edge_api_init(bus_orchestrator_t* orch, driver_manager_t* drv_mgr);

/**
 * @brief Submits a high-level request to read a single tag value.
 *
 * @param priority 仲裁优先级。**周期抄表必须传 `ARBITER_PRIO_LOW`**——
 *                 抄表是后台任务，不该和交互式请求抢总线。
 */
int edge_api_read_tag(int bus_id, const char* protocol, uint32_t slave_id, uint32_t physical_tag_id,
                      int priority, legacy_on_complete_cb cb, void* user_data);

/**
 * @brief 读一个点位，显式区分「驱动认的标识」与「调用方认的标识」。
 *
 * ADR-002 之后驱动只认原生点位（645 的 DI、CAN 的 CAN ID、Modbus 的寄存器地址），
 * 而调用方（产品的点位模型）要拿回自己的 tag_id 才能把值落到正确的 key 上。
 * 这两个标识不是一回事，所以请求里各带一份，由**调用方**做翻译，驱动不参与。
 *
 * @param native_id      驱动认识的协议原生点位标识
 * @param result_tag_id  回调里 result->tag_id 用的标识（调用方自己的 tag_id）
 * @param scale          量纲：解码值 × scale 后交给回调；传 0 表示 1.0
 * @param priority       仲裁优先级。周期抄表传 `ARBITER_PRIO_LOW`；
 *                       交互式单点读传 `ARBITER_PRIO_NORMAL`
 */
int edge_api_read_tag_ex(int bus_id, const char* protocol, uint32_t slave_id, uint32_t native_id,
                         uint32_t result_tag_id, double scale, const void* point_desc,
                         size_t point_desc_len, int priority, legacy_on_complete_cb cb,
                         void* user_data);

/**
 * @brief 登记某协议的设备地址表（645 的 6 字节 BCD 表地址等）。
 *
 * 取代原先从 `driver_contract_injection_t.meta_list` 取地址的做法：
 * 组合根启动时显式登记一次，请求层按协议名查这张表交给驱动的 req_prepare。
 */
void edge_api_set_protocol_meta(const char* protocol, const void* meta, uint32_t count);

/**
 * @brief Submits a high-level request to write a single tag value.
 */
int edge_api_write_tag(int bus_id, const char* protocol, uint32_t slave_id,
                       uint32_t physical_tag_id, double value, legacy_on_complete_cb cb,
                       void* user_data);

/**
 * @brief Submits a raw I/O passthrough request.
 */
int edge_api_raw_io(int bus_id, net_buf_t* tx_buf, raw_io_on_complete_cb cb);

#endif
