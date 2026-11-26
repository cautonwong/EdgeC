#include "iems_execution_adapter.h"

#include <math.h>
#include <stdatomic.h>
#include <strings.h>

#include "dlt645_codec.h"
#include "edge_api.h"
#include "edge_log.h"
#include "iems_driver_adapter.h"
#include "modbus_codec.h"

/* 点位描述必须活到请求层的异步 setup（req_prepare 在总线调度时才读它），
 * 因此按宽度用两份静态只读描述，避免栈上指针悬空。 */
static const dlt645_write_desc_t k_dlt645_desc_len2 = {.data_len = 2};
static const dlt645_write_desc_t k_dlt645_desc_len4 = {.data_len = 4};

/* 完成回调上下文池：总线结算时回调只带一个 user_data，这里把领域回调与其用户指针
 * 寄存在定长槽里，legacy trampoline 用槽指针找回。16 路在途控制写远大于实际并发；
 * 槽满时不提交（返回 -2），不阻塞、不丢失已完成请求的回调。 */
#define IEMS_EXEC_DONE_SLOTS 16

typedef struct {
    atomic_bool in_use;
    iems_write_done_fn fn;
    void *user;
} iems_exec_done_slot_t;

static iems_exec_done_slot_t g_exec_done_slots[IEMS_EXEC_DONE_SLOTS];

static iems_exec_done_slot_t *_claim_done_slot(void) {
    for (size_t i = 0; i < IEMS_EXEC_DONE_SLOTS; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&g_exec_done_slots[i].in_use, &expected, true)) {
            return &g_exec_done_slots[i];
        }
    }
    return NULL;
}

static void _release_done_slot(iems_exec_done_slot_t *slot) {
    if (slot) atomic_store(&slot->in_use, false);
}

iems_write_done_status_t iems_exec_map_write_status(int cmd_status) {
    if (cmd_status == CMD_STATUS_SUCCESS) return IEMS_WRITE_DONE_OK;
    if (cmd_status == CMD_STATUS_TIMEOUT) return IEMS_WRITE_DONE_TIMEOUT;
    return IEMS_WRITE_DONE_FAILED;
}

/* 请求层回调（总线线程）：映射状态、先释放槽再交给领域回调，避免回调里再提交时占槽。 */
static void _iems_exec_on_complete(int status, const pipe_msg_t *result, void *user_data) {
    (void)result;
    iems_exec_done_slot_t *slot = user_data;
    iems_write_done_fn fn = slot->fn;
    void *user = slot->user;
    _release_done_slot(slot);
    if (fn) fn(user, iems_exec_map_write_status(status));
}

int iems_exec_translate_write(const char *proto, uint32_t tag_id, double value,
                              uint32_t *native_id, double *raw, const void **point_desc,
                              size_t *point_desc_len) {
    if (!native_id || !raw || !point_desc || !point_desc_len) return -1;
    /* 驱动是纯 codec，不认识点位模型。原生标识复用读路径的翻译：Modbus 带
     * 寄存器个数、CAN 换 CAN ID、DLT645 换 DI；标定沿用原有口径。 */
    *native_id = iems_driver_adapter_native_id(proto, tag_id);
    *raw = value;
    *point_desc = NULL;
    *point_desc_len = 0;

    if (proto && strcasecmp(proto, "DLT645") == 0) {
        /* 645 写的数据段宽度（2/4 字节）是点位模型的知识。宽度不对会把 BCD
         * 写到错误的字节位置，点表缺失时也不该让驱动去猜。 */
        const dlt645_map_t *m = iems_lookup_dlt645_by_tag(tag_id);
        if (!m || (m->length != 2 && m->length != 4)) {
            LOG_WARN(LOG_CAT_CORE, "IEMS control: DLT645 tag %u 点位宽度未知（%u），拒绝写入",
                     tag_id, m ? (unsigned)m->length : 0u);
            return -1;
        }
        double sc = (m->scaler > 0.0f) ? (double)m->scaler : 1.0;
        *raw = (double)lround(value / sc);
        *point_desc = (m->length == 2) ? (const void *)&k_dlt645_desc_len2
                                       : (const void *)&k_dlt645_desc_len4;
        *point_desc_len = sizeof(dlt645_write_desc_t);
        return 0;
    }

    /* 只有 Modbus 家族的写值要按点位表标定；其它协议的原生值口径不同，
     * 混用 point 表会把 CAN 的写值算错。 */
    bool modbus_family = proto && (strcasecmp(proto, "ModbusRTU") == 0 ||
                                   strcasecmp(proto, "ModbusRTUHiCRC") == 0 ||
                                   strcasecmp(proto, "ModbusTCP") == 0 ||
                                   strcasecmp(proto, "Modbus") == 0);
    const modbus_map_t *map = modbus_family ? edge_get_map_modbus_by_tag(tag_id) : NULL;
    if (map) {
        double sc = (map->scaler > 0.0) ? (double)map->scaler : 1.0;
        *raw = (double)lround(value / sc);
    } else if (*native_id == tag_id) {
        LOG_WARN(LOG_CAT_CORE,
                 "IEMS control: tag %u 不在点位表里，按原生 id 下发（地址可能与预期不符）",
                 tag_id);
    }
    return 0;
}

static int iems_exec_write(void *ctx, int bus_id, uint32_t slave_id, uint32_t tag_id, double value,
                           const char *proto, iems_write_done_fn on_done, void *on_done_user) {
    (void)ctx;
    uint32_t native_id = tag_id;
    double raw = value;
    const void *point_desc = NULL;
    size_t point_desc_len = 0;
    if (iems_exec_translate_write(proto, tag_id, value, &native_id, &raw, &point_desc,
                                  &point_desc_len) != 0) {
        return -1;
    }

    iems_exec_done_slot_t *slot = NULL;
    if (on_done) {
        slot = _claim_done_slot();
        if (!slot) {
            /* 槽满不提交：调用方看到的失败与总线拒绝一致，不会静默丢结果。 */
            LOG_ERROR(LOG_CAT_CORE, "IEMS control: 完成回调槽位已满(%d)，拒绝提交 tag %u",
                      IEMS_EXEC_DONE_SLOTS, tag_id);
            return -2;
        }
        slot->fn = on_done;
        slot->user = on_done_user;
    }

    int rc = edge_api_write_tag_ex(bus_id, proto, slave_id, native_id, raw, point_desc,
                                   point_desc_len, slot ? _iems_exec_on_complete : NULL, slot);
    /* 非零 = 未入队：请求层不会再回调，槽必须在这里还回去。 */
    if (rc != 0) _release_done_slot(slot);
    return rc;
}

iems_execution_port_t iems_execution_port_create(void *ctx) {
    return (iems_execution_port_t){.ctx = ctx, .write_tag = iems_exec_write};
}
