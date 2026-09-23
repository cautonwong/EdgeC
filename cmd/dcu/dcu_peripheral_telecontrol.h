/**
 * @file dcu_peripheral_telecontrol.h
 * @brief Production SGCC/CSG Peripheral Control & "Four-Remotes" (四遥: YC/YX/YK/YT) Interface
 * Engine
 */

#ifndef DCU_PERIPHERAL_TELECONTROL_H
#define DCU_PERIPHERAL_TELECONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define DCU_MAX_DI_CHANNELS 16
#define DCU_MAX_DO_CHANNELS 8
#define DCU_MAX_CAPACITOR_BANKS 4

typedef enum {
    YK_ACTION_TRIP = 0,  // 遥控分闸 (Open/Trip)
    YK_ACTION_CLOSE = 1  // 遥控合闸 (Close)
} dcu_yk_action_t;

typedef enum {
    BREAKER_STATE_OPEN = 0,
    BREAKER_STATE_CLOSED = 1,
    BREAKER_STATE_TRIPPED = 2,
    BREAKER_STATE_FAULT = 3
} dcu_breaker_state_t;

/**
 * @brief Consumer-defined Hardware I/O Interface (Golang-style decoupling)
 */
typedef struct dcu_io_ops {
    int (*read_di)(uint8_t di_channel, bool* out_state, void* user_data);
    int (*set_do)(uint8_t do_channel, bool state, void* user_data);
    int (*set_oltc_tap)(int8_t tap_step, void* user_data);  // 有载调压分接头步进
    void* user_data;
} dcu_io_ops_t;

typedef struct {
    // 遥信 (YX / DI) 状态表
    bool di_states[DCU_MAX_DI_CHANNELS];
    dcu_breaker_state_t breaker_state;
    bool surge_protector_tripped;  // 防雷器脱扣
    bool cabinet_door_open;        // 柜门门禁开闭
    bool smoke_alarm_active;       // 烟雾传感器

    // 遥控 (YK / DO) 状态表
    bool do_states[DCU_MAX_DO_CHANNELS];
    bool capacitor_bank_states[DCU_MAX_CAPACITOR_BANKS];  // 4组无功电容器投退状态

    // 遥调 (YT) 状态
    int8_t current_oltc_tap;         // 当前变压器分接头档位 (-8 ~ +8)
    double pv_curtailment_limit_kw;  // 光伏出力限制 (kW)

    dcu_io_ops_t ops;
} dcu_telecontrol_manager_t;

/**
 * @brief 初始化四遥与外设控制引擎
 */
void dcu_telecontrol_init(dcu_telecontrol_manager_t* mgr, const dcu_io_ops_t* ops);

/**
 * @brief 遥信扫描 (YX Scan): 扫描 16 路 DI 与外设传感器状态
 */
int dcu_telecontrol_scan_yx(dcu_telecontrol_manager_t* mgr);

/**
 * @brief 遥控执行 (YK Execution): 下发主断路器分/合闸或继电器动作 (DL/T 698 OMD 0x80000100)
 */
int dcu_telecontrol_exec_yk(dcu_telecontrol_manager_t* mgr, uint8_t do_channel,
                            dcu_yk_action_t action);

/**
 * @brief 无功电容器组自动步进投切 (Capacitor Bank Switching, OMD 0x80010100)
 * @param bank_idx 电容器组序号 (0~3)
 * @param enable 投 (true) / 切 (false)
 */
int dcu_telecontrol_switch_capacitor(dcu_telecontrol_manager_t* mgr, uint8_t bank_idx, bool enable);

/**
 * @brief 变压器有载分接头遥调 (OLTC Tap Regulation, OMD 0x80030100)
 */
int dcu_telecontrol_regulate_oltc(dcu_telecontrol_manager_t* mgr, int8_t target_tap);

#ifdef __cplusplus
}
#endif

#endif  // DCU_PERIPHERAL_TELECONTROL_H
