/**
 * @file dcu_dlt698_mgmt_engine.h
 * @brief Production DL/T 698.45 Management Engine (0xF001 DFU OTA, 0x4000 Parameters & 0xF002 Security Session)
 */

#ifndef DCU_DLT698_MGMT_ENGINE_H
#define DCU_DLT698_MGMT_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DCU_DFU_BLOCK_SIZE 512
#define DCU_MAX_TOU_SLOTS 8

typedef enum {
    DFU_STATE_IDLE       = 0,
    DFU_STATE_INITIATED  = 1,
    DFU_STATE_RECEIVING  = 2,
    DFU_STATE_VERIFYING  = 3,
    DFU_STATE_READY      = 4,
    DFU_STATE_ACTIVATED  = 5,
    DFU_STATE_FAILED     = 6
} dcu_dfu_state_t;

typedef struct {
    uint8_t slot_id;
    uint8_t start_hour;
    uint8_t start_minute;
    uint8_t tariff_rate; // 1=Peak(尖), 2=Shoulder(峰), 3=Flat(平), 4=Valley(谷)
} dcu_tou_slot_t;

typedef struct {
    dcu_tou_slot_t slots[DCU_MAX_TOU_SLOTS];
    uint8_t slot_count;
    uint32_t version;
} dcu_tou_tariff_table_t;

typedef struct {
    dcu_dfu_state_t state;
    uint32_t total_image_size;
    uint32_t received_bytes;
    uint32_t expected_blocks;
    uint32_t received_blocks;
    uint32_t target_crc32;
    uint32_t computed_crc32;
    char target_version[32];
    uint8_t active_bank; // 0=Bank A, 1=Bank B
} dcu_dfu_session_t;

typedef struct {
    bool is_authenticated;
    uint32_t session_id;
    uint64_t session_expire_ms;
    uint8_t security_level; // 0=None, 1=Password, 2=Symmetric(SM4), 3=Asymmetric(SM2)
} dcu_security_session_t;

typedef struct {
    dcu_dfu_session_t dfu;
    dcu_tou_tariff_table_t tariff_table;
    dcu_security_session_t sec_session;
} dcu_dlt698_mgmt_t;

/**
 * @brief 初始化 DL/T 698.45 管理与控制引擎
 */
void dcu_mgmt_init(dcu_dlt698_mgmt_t* mgmt);

/**
 * @brief OAD 0xF001 固件空中升级 (DFU): 启动固件传输任务
 */
int dcu_mgmt_dfu_start(dcu_dlt698_mgmt_t* mgmt, const char* version, uint32_t total_size, uint32_t crc32);

/**
 * @brief OAD 0xF001 固件空中升级 (DFU): 写入一个分包数据
 */
int dcu_mgmt_dfu_write_block(dcu_dlt698_mgmt_t* mgmt, uint32_t block_index, const uint8_t* block_data, uint32_t block_len);

/**
 * @brief OAD 0xF001 固件空中升级 (DFU): 校验并激活目标 Bank 分区
 */
int dcu_mgmt_dfu_verify_and_activate(dcu_dlt698_mgmt_t* mgmt);

/**
 * @brief OAD 0x4000 参数配置: 动态更新阶梯电价时段表 (TOU)
 */
int dcu_mgmt_set_tariff_table(dcu_dlt698_mgmt_t* mgmt, const dcu_tou_slot_t* slots, uint8_t count);

/**
 * @brief OAD 0xF002/0xF003 安全会话与身份鉴权
 */
int dcu_mgmt_authenticate_session(dcu_dlt698_mgmt_t* mgmt, uint8_t level, uint32_t duration_s, uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif // DCU_DLT698_MGMT_ENGINE_H
