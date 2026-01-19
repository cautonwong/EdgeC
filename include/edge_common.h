#ifndef EDGE_COMMON_H
#define EDGE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>

/**
 * @brief 工业标准 Modbus CRC16
 */
static inline uint16_t edge_crc16_modbus(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else
                crc >>= 1;
        }
    }
    return crc;
}

/**
 * @brief DLMS/HDLC 标准 CRC-CCITT (X.25)
 */
static inline uint16_t edge_crc16_ccitt(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        uint8_t data = buf[i];
        for (int j = 0; j < 8; j++) {
            if ((crc ^ data) & 0x0001)
                crc = (uint16_t)((crc >> 1) ^ 0x8408);
            else
                crc >>= 1;
            data >>= 1;
        }
    }
    return (uint16_t)(crc ^ 0xFFFF);
}

// 统一状态码
typedef enum {
    EDGE_OK = 0,
    EDGE_ERR_GENERIC = -1,
    EDGE_ERR_INVALID_PARAM = -2,
    EDGE_ERR_NOMEM = -3,
    EDGE_ERR_TIMEOUT = -4,
    EDGE_ERR_BUSY = -5,
    EDGE_ERR_NOT_FOUND = -6,
    EDGE_ERR_NOT_SUPPORTED = -7,
    EDGE_ERR_IO = -8,
    EDGE_ERR_OVERFLOW = -9,
    EDGE_ERR_INCOMPLETE = -10
} edge_status_t;

typedef uint32_t edge_tag_id_t;
typedef int64_t wall_time_t;  /**< Unix Wall Time in Milliseconds */
typedef uint64_t mono_time_t; /**< Monotonic Time in Milliseconds */

/**
 * @brief 物理层通用参数 (UART/CAN 等)
 */
typedef struct {
    uint32_t baud;
    uint8_t data_bits;
    uint8_t stop_bits;
    char parity;
} edge_phy_params_t;

// [核心补强] 系统级事件 ID
typedef enum {
    SYS_EVENT_BOOT = 1,
    SYS_EVENT_SHUTDOWN = 2,
    SYS_EVENT_CONFIG_RELOAD = 3,
    SYS_EVENT_TIME_CHANGED = 4,
    SYS_EVENT_USER_CUSTOM = 100
} edge_event_id_t;

#define EDGE_UNUSED(x) (void)(x)

int edge_get_app_root(char *out_path, size_t size);

#ifdef __cplusplus
}
#endif

#endif
