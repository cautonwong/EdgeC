/**
 * @file modbus_tcp_driver.c
 * @brief Modbus TCP 驱动 —— 纯 codec（ADR-002 S3-b）
 *
 * 主站与从站两条路径都不再接触点位表、影子表、消息总线：
 *
 *   主站：req_encode 只认「寄存器地址 + 个数」，req_decode 只回原始寄存器值
 *   从站：req_slave_exchange 解请求 → 向注入的端口取数/收数 → 组应答
 *
 * ## 点位标识怎么表达
 *
 * Modbus 的一个点位是「起始地址 + 寄存器个数」（32 位量占 2 个），
 * 而 native_id 只有一个 32 位数，所以编码为：
 *
 *     native_id = (quantity << 16) | address
 *
 * 产品侧用 MODBUS_NATIVE() 编，驱动侧解。地址与长度都是协议原生概念，
 * 不涉及任何 TagID —— 这正是 ADR-002 要的形态。
 */

#include <stdint.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "modbus_codec.h"
#include "net_buf.h"

#define MB_MAX_REGS 32

/** 一次读请求的私有数据：驱动认识的全部信息 */
typedef struct {
    uint16_t addr;     /**< 起始寄存器地址（协议原生点位） */
    uint16_t quantity; /**< 寄存器个数 */
    uint16_t tid;      /**< 事务号 */
    uint8_t slave_id;
    uint16_t raw[MB_MAX_REGS]; /**< 解码出的原始寄存器 */
    uint16_t raw_n;
} modbus_tcp_req_t;

/* 事务号：协议层序号，不依赖任何全局服务。应答匹配实际只用 slave_id，
 * 这里保持单调递增只是为了抓包时好对照。 */
static uint16_t s_tid;

static int _decode_native(uint32_t native_id, uint16_t *addr, uint16_t *quantity) {
    *addr = (uint16_t)(native_id & 0xFFFFu);
    *quantity = (uint16_t)((native_id >> 16) & 0xFFFFu);
    if (*quantity == 0) *quantity = 1;
    if (*quantity > MB_MAX_REGS) return -1;
    return 0;
}

/* ==================== 主站侧 ==================== */

static int mb_tcp_req_prepare(void *priv, uint32_t slave_id, uint32_t point_id,
                              const void *meta_list, uint32_t meta_list_len) {
    (void)meta_list;
    (void)meta_list_len;
    modbus_tcp_req_t *p = (modbus_tcp_req_t *)priv;
    if (!p) return -1;
    if (_decode_native(point_id, &p->addr, &p->quantity) != 0) return -1;
    p->slave_id = (uint8_t)slave_id;
    p->tid = ++s_tid;
    p->raw_n = 0;
    return 0;
}

static int mb_tcp_req_encode(void *priv, net_buf_t *out) {
    modbus_tcp_req_t *p = (modbus_tcp_req_t *)priv;
    if (!p || !out) return -1;
    return modbus_tcp_encode_read(p->tid, p->slave_id, MODBUS_FC_READ_HOLDING_REGS, p->addr,
                                  p->quantity, out);
}

static int mb_tcp_req_decode(void *priv, const uint8_t *buf, size_t len, size_t *consumed,
                             double *out_value) {
    modbus_tcp_req_t *p = (modbus_tcp_req_t *)priv;
    if (!p || !buf || !consumed) return -1;
    *consumed = 0;

    if (len < MODBUS_TCP_HEADER_LEN + 2) return EDGE_ERR_INCOMPLETE;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)buf, len);
    nb.len = (uint16_t)len; /* init_with_data 只设 size，len 留给写入用 */

    uint16_t tid = 0;
    uint8_t slave = 0;
    size_t pdu_len = 0;
    int rc = modbus_tcp_decode_mbap(&nb, &tid, &slave, &pdu_len);
    if (rc != 0) return (rc == 1) ? EDGE_ERR_INCOMPLETE : -1;

    *consumed = MODBUS_TCP_HEADER_LEN + pdu_len + 1;

    /* 不是本会话的从站：不认，让调用方继续找 */
    if (p->slave_id != 0 && slave != p->slave_id) return EDGE_ERR_INCOMPLETE;

    net_buf_simple_pull(&nb, MODBUS_TCP_HEADER_LEN);
    if (pdu_len < 2) return -1;

    uint8_t fc = nb.data[0];
    if (fc & 0x80) {
        LOG_WARN(LOG_CAT_IO, "ModbusTCP: exception fc=0x%02X code=%d", fc, nb.data[1]);
        return -1;
    }
    if (fc != MODBUS_FC_READ_HOLDING_REGS && fc != MODBUS_FC_READ_INPUT_REGS) return -1;
    if (pdu_len < 3) return -1;

    uint8_t byte_count = nb.data[1];
    if ((size_t)(2 + byte_count) > pdu_len) return -1;

    uint16_t n = (uint16_t)(byte_count / 2);
    if (n > p->quantity) n = p->quantity;
    if (n > MB_MAX_REGS) n = MB_MAX_REGS;

    p->raw_n = n;
    for (uint16_t i = 0; i < n; i++) {
        p->raw[i] = (uint16_t)((nb.data[2 + 2 * i] << 8) | nb.data[3 + 2 * i]);
    }
    /* 只回原始寄存器值；工程值 = raw / scaler，由调用方按点位表换算 */
    if (out_value) *out_value = p->raw_n ? (double)p->raw[0] : 0.0;
    return 0;
}

/* ==================== 从站侧 ==================== */

static int mb_tcp_req_slave_exchange(void *priv, const uint8_t *in, size_t in_len,
                                     const edge_slave_source_ops_t *src, void *src_ctx,
                                     net_buf_t *out, size_t *consumed) {
    modbus_tcp_req_t *p = (modbus_tcp_req_t *)priv;
    if (!p || !in || !out || !consumed) return -1;
    *consumed = 0;

    if (in_len < MODBUS_TCP_HEADER_LEN + 2) return EDGE_ERR_INCOMPLETE;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)in, in_len);
    nb.len = (uint16_t)in_len;

    uint16_t tid = 0;
    uint8_t slave = 0;
    size_t pdu_len = 0;
    int rc = modbus_tcp_decode_mbap(&nb, &tid, &slave, &pdu_len);
    if (rc != 0) return (rc == 1) ? EDGE_ERR_INCOMPLETE : -1;

    *consumed = MODBUS_TCP_HEADER_LEN + pdu_len + 1;

    /* 只应答本机地址；地址不匹配就当不是给我的，跳过去 */
    if (p->slave_id != 0 && slave != p->slave_id) return 1;

    net_buf_simple_pull(&nb, MODBUS_TCP_HEADER_LEN);
    if (pdu_len < 2) return -1;

    uint8_t fc = nb.data[0];

    switch (fc) {
        case MODBUS_FC_READ_HOLDING_REGS:
        case MODBUS_FC_READ_INPUT_REGS: {
            if (pdu_len < 5) {
                modbus_tcp_encode_exception(tid, slave, fc, MODBUS_EX_ILLEGAL_DATA_VALUE, out);
                return 0;
            }
            uint16_t start = (uint16_t)((nb.data[1] << 8) | nb.data[2]);
            uint16_t quantity = (uint16_t)((nb.data[3] << 8) | nb.data[4]);
            if (quantity == 0 || quantity > MB_MAX_REGS) {
                modbus_tcp_encode_exception(tid, slave, fc, MODBUS_EX_ILLEGAL_DATA_VALUE, out);
                return 0;
            }

            uint8_t resp[2 * MB_MAX_REGS];
            size_t n = 0;
            for (uint16_t i = 0; i < quantity; i++) {
                /* 每个寄存器是一个独立点位：quantity 固定 1 */
                uint32_t pid = ((1u << 16) | (uint32_t)(start + i));
                edge_point_value_t v;
                memset(&v, 0, sizeof(v));
                uint16_t raw = 0;
                /* Modbus 没有"部分失败"语义，取不到只能按 0 应答；
                 * 点位缺失由产品侧日志暴露，不在协议层造假。 */
                if (src && src->resolve && src->resolve(src_ctx, pid, &v) == 0) {
                    raw = (uint16_t)v.raw;
                }
                resp[n++] = (uint8_t)(raw >> 8);
                resp[n++] = (uint8_t)(raw & 0xFF);
            }
            modbus_tcp_encode_read_resp(tid, slave, resp, n, out);
            return 0;
        }

        case MODBUS_FC_WRITE_SINGLE_REG: {
            if (pdu_len < 5) return -1;
            uint16_t addr = (uint16_t)((nb.data[1] << 8) | nb.data[2]);
            uint16_t raw = (uint16_t)((nb.data[3] << 8) | nb.data[4]);
            uint32_t pid = ((1u << 16) | (uint32_t)addr);
            /* 端口收到的是**原始寄存器值**：产品按自己的点位表换算工程值 */
            if (src && src->accept) (void)src->accept(src_ctx, pid, (double)raw);
            modbus_tcp_encode_write_resp(tid, slave, addr, raw, out);
            return 0;
        }

        case MODBUS_FC_WRITE_MULTIPLE_REGS: {
            if (pdu_len < 8) return -1;
            uint16_t addr = (uint16_t)((nb.data[1] << 8) | nb.data[2]);
            uint16_t quantity = (uint16_t)((nb.data[3] << 8) | nb.data[4]);
            if ((size_t)(6 + 2 * quantity) > pdu_len) return -1;

            for (uint16_t i = 0; i < quantity; i++) {
                uint16_t raw = (uint16_t)((nb.data[6 + 2 * i] << 8) | nb.data[7 + 2 * i]);
                uint32_t pid = ((1u << 16) | (uint32_t)(addr + i));
                if (src && src->accept) (void)src->accept(src_ctx, pid, (double)raw);
            }

            uint8_t body[5];
            body[0] = MODBUS_FC_WRITE_MULTIPLE_REGS;
            body[1] = (uint8_t)(addr >> 8);
            body[2] = (uint8_t)(addr & 0xFF);
            body[3] = (uint8_t)(quantity >> 8);
            body[4] = (uint8_t)(quantity & 0xFF);
            modbus_tcp_encode_mbap(tid, slave, 5, out);
            net_buf_simple_add_mem(&out->b, body, sizeof(body));
            return 0;
        }

        default:
            modbus_tcp_encode_exception(tid, slave, fc, MODBUS_EX_ILLEGAL_FUNCTION, out);
            return 0;
    }
}

const edge_driver_t modbus_tcp_driver = {
    .name = "ModbusTCP",
    .ctx_size = 0, /* request-native：无 per-session 上下文 */
    .req_priv_size = sizeof(modbus_tcp_req_t),
    .req_prepare = mb_tcp_req_prepare,
    .req_encode = mb_tcp_req_encode,
    .req_decode = mb_tcp_req_decode,
    .req_slave_exchange = mb_tcp_req_slave_exchange,
};

int modbus_tcp_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &modbus_tcp_driver);
}
