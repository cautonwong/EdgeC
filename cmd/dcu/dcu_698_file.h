/**
 * @file dcu_698_file.h
 * @brief Class 18 (0xF000/0xF001/0xF002) 文件传输软件领域引擎
 */

#ifndef DCU_698_FILE_H
#define DCU_698_FILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DCU_FILE_MAX_BLOCKS 64u
#define DCU_FILE_BLOCK_SIZE 256u
#define DCU_FILE_MAX_SIZE (DCU_FILE_MAX_BLOCKS * DCU_FILE_BLOCK_SIZE) /* 16384 B */
#define DCU_FILE_NAME_MAX 64u
#define DCU_FILE_BITMAP_BYTES (DCU_FILE_MAX_BLOCKS / 8u) /* 8 B */

/**
 * @brief Class 18 命令结果枚举（DL/T 698.45 表 176 属性 3）。
 */
typedef enum {
    DCU_FILE_RES_OK = 100,
    DCU_FILE_RES_NOT_FOUND = 104,
    DCU_FILE_RES_DENIED = 105,
    DCU_FILE_RES_ABORTED = 106,
    DCU_FILE_RES_VERIFY_FAIL = 107,
    DCU_FILE_RES_NO_SPACE = 112,
    DCU_FILE_RES_RESET = 255,
} dcu_file_cmd_result_t;

/**
 * @brief Class 18 静态有界文件传输会话上下文（零动态内存，必须放在 static 存储）。
 */
typedef struct {
    bool active;
    bool verified;
    char filename[DCU_FILE_NAME_MAX];
    uint32_t total_size;
    uint16_t block_size;
    uint16_t total_blocks;
    uint32_t expected_crc32;
    uint8_t cmd_result;
    uint8_t bitmap[DCU_FILE_BITMAP_BYTES];
    uint8_t buffer[DCU_FILE_MAX_SIZE];
} dcu_file_session_t;

/**
 * @brief 可选硬件 DFU 激活/回滚端口。未注入时诚实返回 EDGE_SLAVE_SRC_CLASS_MISMATCH。
 */
typedef struct {
    int (*activate)(void *ctx);
    int (*rollback)(void *ctx);
} dcu_file_dfu_ops_t;

/** @brief 重置静态文件传输会话上下文为初始态。 */
void dcu_698_file_reset(void);

/** @brief 获取当前静态文件传输会话只读视图。 */
const dcu_file_session_t *dcu_698_file_get_session(void);

/** @brief 计算标准 IEEE 802.3 CRC32（多项式 0xEDB88320）。 */
uint32_t dcu_698_file_crc32(const uint8_t *data, size_t len);

/**
 * @brief 启动分块文件传输（对应 F001 M7）。
 *
 * @param filename       文件名（非空，长度 < DCU_FILE_NAME_MAX）
 * @param total_size     文件总字节数（1..16384）
 * @param block_size     块大小（1..256，且总块数 <= 64）
 * @param expected_crc32 预期 IEEE 802.3 CRC32
 * @return 0 成功；负值 EDGE_SLAVE_SRC_* 表示参数越界或非法
 */
int dcu_698_file_start(const char *filename, uint32_t total_size, uint16_t block_size,
                       uint32_t expected_crc32);

/**
 * @brief 写文件分块（对应 F001 M8）；重复写同一块幂等覆盖。
 */
int dcu_698_file_write_block(uint16_t block_idx, const uint8_t *data, size_t len);

/**
 * @brief 读文件分块（对应 F001 M9）；未写入块返回未就绪错误。
 */
int dcu_698_file_read_block(uint16_t block_idx, uint8_t *out_buf, size_t cap, size_t *out_len);

/**
 * @brief 校验文件完整性与 CRC32（对应 F001 M10/M4）。
 */
int dcu_698_file_verify(void);

/**
 * @brief 编码 Class 18 属性为 A-XDR 字节流（F001 A2 文件信息/状态、A3 传输位图）。
 */
int dcu_698_file_encode_attr(uint16_t oi, uint8_t attr_id, uint8_t *buf, size_t cap,
                             size_t *out_len);

/**
 * @brief 触发 DFU 激活（未注入端口时返回 EDGE_SLAVE_SRC_CLASS_MISMATCH）。
 */
int dcu_698_file_activate(const dcu_file_dfu_ops_t *ops, void *ctx);

/**
 * @brief 触发 DFU 回滚（未注入端口时返回 EDGE_SLAVE_SRC_CLASS_MISMATCH）。
 */
int dcu_698_file_rollback(const dcu_file_dfu_ops_t *ops, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_FILE_H */
