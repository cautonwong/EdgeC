/**
 * @file modbus_rtu_driver.c
 * @brief Modbus RTU 驱动 —— 纯 codec（ADR-002 S3-b）
 *
 * 与 ModbusTCP 同构：主站只认「寄存器地址 + 个数」，从站取数走注入端口。
 * 差别只有三处：
 *   1. 帧尾有 CRC16
 *   2. **没有帧间静默代码**——3.5 字符间隔归 arbiter（`arbiter_set_bus_min_gap`），
 *      驱动里再等一次就是两处都在管，迟早打架
 *   3. 32 位量（quantity==2）按 ABCD 合包，与旧实现一致
 */

#include <stdint.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "modbus_codec.h"
#include "net_buf.h"

#define MB_MAX_REGS 32

typedef struct {
    uint16_t addr;
    uint16_t quantity;
    uint8_t slave_id;
    uint16_t raw[MB_MAX_REGS];
    uint16_t raw_n;
} modbus_rtu_req_t;

static int _decode_native(uint32_t native_id, uint16_t *addr, uint16_t *quantity) {
    *addr = MODBUS_NATIVE_ADDR(native_id);
    *quantity = MODBUS_NATIVE_QTY(native_id);
    if (*quantity == 0) *quantity = 1;
    if (*quantity > MB_MAX_REGS) return -1;
    return 0;
}

/* ==================== 主站侧 ==================== */

static int mb_rtu_req_prepare(void *priv, uint32_t slave_id, uint32_t point_id,
                              const void *meta_list, uint32_t meta_list_len) {
    (void)meta_list;
    (void)meta_list_len;
    modbus_rtu_req_t *p = (modbus_rtu_req_t *)priv;
    if (!p) return -1;
    if (_decode_native(point_id, &p->addr, &p->quantity) != 0) return -1;
    p->slave_id = (uint8_t)slave_id;
    p->raw_n = 0;
    return 0;
}

static int mb_rtu_req_encode(void *priv, net_buf_t *out) {
    modbus_rtu_req_t *p = (modbus_rtu_req_t *)priv;
    if (!p || !out) return -1;
    return modbus_rtu_encode_read(p->slave_id, MODBUS_FC_READ_HOLDING_REGS, p->addr, p->quantity,
                                  out);
}

static int mb_rtu_req_decode(void *priv, const uint8_t *buf, size_t len, size_t *consumed,
                             double *out_value) {
    modbus_rtu_req_t *p = (modbus_rtu_req_t *)priv;
    if (!p || !buf || !consumed) return -1;
    *consumed = 0;

    if (len < 5) return EDGE_ERR_INCOMPLETE;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)buf, len);
    nb.len = (uint16_t)len; /* init_with_data 只设 size，len 留给写入用 */

    uint8_t slave = 0, fc = 0;
    uint8_t *data = NULL;
    size_t data_len = 0;
    int rc = modbus_rtu_decode_frame(&nb, &slave, &fc, &data, &data_len);
    if (rc == 1) return EDGE_ERR_INCOMPLETE;
    if (rc < 0) {
        *consumed = 1; /* CRC 错：跳一格重找帧首，别把后面的好帧一起丢 */
        return EDGE_ERR_INCOMPLETE;
    }

    *consumed = (size_t)(data + data_len + 2 - buf);

    if (p->slave_id != 0 && slave != p->slave_id) return EDGE_ERR_INCOMPLETE;

    if (fc & 0x80) {
        LOG_WARN(LOG_CAT_IO, "ModbusRTU slave %u exception: 0x%02X", slave,
                 data_len ? data[0] : 0);
        return -1;
    }
    if (fc != MODBUS_FC_READ_HOLDING_REGS && fc != MODBUS_FC_READ_INPUT_REGS) return -1;
    if (data_len < 1) return -1;

    uint8_t byte_count = data[0];
    if ((size_t)(1 + byte_count) > data_len) return -1;

    uint16_t n = (uint16_t)(byte_count / 2);
    if (n > p->quantity) n = p->quantity;
    if (n > MB_MAX_REGS) n = MB_MAX_REGS;

    p->raw_n = n;
    for (uint16_t i = 0; i < n; i++) {
        p->raw[i] = (uint16_t)((data[1 + 2 * i] << 8) | data[2 + 2 * i]);
    }

    /* 只回原始值。32 位量（2 个寄存器）按 ABCD 合包，与旧实现一致；
     * 工程值由调用方按点位表换算。 */
    double v = 0.0;
    if (p->raw_n >= 2 && p->quantity == 2) {
        uint32_t v32 = ((uint32_t)p->raw[0] << 16) | p->raw[1];
        v = (double)v32;
    } else if (p->raw_n >= 1) {
        v = (double)p->raw[0];
    }
    if (out_value) *out_value = v;
    return 0;
}

/* ==================== 从站侧 ==================== */

static void _seal_rtu(net_buf_t *out, uint8_t slave, uint8_t fc, const uint8_t *body,
                      size_t body_len) {
    uint8_t *p = net_buf_add(out, 2 + body_len + 2);
    p[0] = slave;
    p[1] = fc;
    if (body_len) memcpy(&p[2], body, body_len);
    uint16_t crc = modbus_calculate_crc(p, 2 + body_len);
    p[2 + body_len] = (uint8_t)(crc & 0xFF); /* CRC 低字节先 */
    p[3 + body_len] = (uint8_t)(crc >> 8);
}

static int mb_rtu_req_slave_exchange(void *priv, const uint8_t *in, size_t in_len,
                                     const edge_slave_source_ops_t *src, void *src_ctx,
                                     net_buf_t *out, size_t *consumed) {
    modbus_rtu_req_t *p = (modbus_rtu_req_t *)priv;
    if (!p || !in || !out || !consumed) return -1;
    *consumed = 0;

    if (in_len < 5) return EDGE_ERR_INCOMPLETE;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)in, in_len);
    nb.len = (uint16_t)in_len;

    uint8_t slave = 0, fc = 0;
    uint8_t *data = NULL;
    size_t data_len = 0;
    int rc = modbus_rtu_decode_req_frame(&nb, &slave, &fc, &data, &data_len);
    if (rc == 1) return EDGE_ERR_INCOMPLETE;
    if (rc < 0) {
        *consumed = 1;
        return EDGE_ERR_INCOMPLETE;
    }

    *consumed = (size_t)(data + data_len + 2 - in);

    if (p->slave_id != 0 && slave != p->slave_id) return 1;

    if (fc == MODBUS_FC_READ_HOLDING_REGS || fc == MODBUS_FC_READ_INPUT_REGS) {
        if (data_len < 4) {
            modbus_rtu_encode_exception(slave, fc, MODBUS_EX_ILLEGAL_DATA_VALUE, out);
            return 0;
        }
        uint16_t addr = (uint16_t)((data[0] << 8) | data[1]);
        uint16_t count = (uint16_t)((data[2] << 8) | data[3]);
        if (count == 0 || count > MB_MAX_REGS) {
            modbus_rtu_encode_exception(slave, fc, MODBUS_EX_ILLEGAL_DATA_VALUE, out);
            return 0;
        }

        uint8_t body[1 + 2 * MB_MAX_REGS];
        body[0] = (uint8_t)(count * 2);
        size_t n = 1;
        for (uint16_t i = 0; i < count; i++) {
            uint32_t pid = MODBUS_NATIVE(addr + i, 1);
            edge_point_value_t v;
            memset(&v, 0, sizeof(v));
            uint16_t raw = 0;
            if (src && src->resolve && src->resolve(src_ctx, pid, &v) == 0) raw = (uint16_t)v.raw;
            body[n++] = (uint8_t)(raw >> 8);
            body[n++] = (uint8_t)(raw & 0xFF);
        }
        _seal_rtu(out, slave, fc, body, n);
        return 0;
    }

    if (fc == MODBUS_FC_WRITE_SINGLE_REG) {
        if (data_len < 4) return -1;
        uint16_t addr = (uint16_t)((data[0] << 8) | data[1]);
        uint16_t raw = (uint16_t)((data[2] << 8) | data[3]);
        if (src && src->accept) (void)src->accept(src_ctx, MODBUS_NATIVE(addr, 1), (double)raw);
        _seal_rtu(out, slave, fc, data, 4); /* 应答是回显 */
        return 0;
    }

    if (fc == MODBUS_FC_WRITE_MULTIPLE_REGS) {
        if (data_len < 5) return -1;
        uint16_t addr = (uint16_t)((data[0] << 8) | data[1]);
        uint16_t count = (uint16_t)((data[2] << 8) | data[3]);
        if ((size_t)(5 + 2 * count) > data_len) return -1;
        for (uint16_t i = 0; i < count; i++) {
            uint16_t raw = (uint16_t)((data[5 + 2 * i] << 8) | data[6 + 2 * i]);
            if (src && src->accept)
                (void)src->accept(src_ctx, MODBUS_NATIVE(addr + i, 1), (double)raw);
        }
        _seal_rtu(out, slave, fc, data, 4); /* 应答：地址 + 数量 */
        return 0;
    }

    modbus_rtu_encode_exception(slave, fc, MODBUS_EX_ILLEGAL_FUNCTION, out);
    return 0;
}

const edge_driver_t modbus_rtu_driver = {
    .name = "ModbusRTU",
    .ctx_size = 0,
    .req_priv_size = sizeof(modbus_rtu_req_t),
    .req_prepare = mb_rtu_req_prepare,
    .req_encode = mb_rtu_req_encode,
    .req_decode = mb_rtu_req_decode,
    .req_slave_exchange = mb_rtu_req_slave_exchange,
};

int modbus_rtu_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &modbus_rtu_driver);
}

/* 兼容别名：老调用方（tests/test_iems_full_stack.c 等）用的是这个名字。 */
int modbus_driver_register(driver_manager_t *mgr) {
    return modbus_rtu_driver_register(mgr);
}
