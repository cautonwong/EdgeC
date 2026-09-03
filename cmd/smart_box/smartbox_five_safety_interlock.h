/**
 * @file smartbox_five_safety_interlock.h
 * @brief SmartBox Switchgear Electrical Five-Safety Intelligent Interlocking Engine (AEP-442)
 */

#ifndef SMARTBOX_FIVE_SAFETY_INTERLOCK_H
#define SMARTBOX_FIVE_SAFETY_INTERLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    INTERLOCK_OP_CLOSE_BREAKER        = 1, // 合断路器
    INTERLOCK_OP_OPEN_BREAKER         = 2, // 分断路器
    INTERLOCK_OP_CLOSE_BUS_ISOLATOR   = 3, // 合母线刀闸
    INTERLOCK_OP_OPEN_BUS_ISOLATOR    = 4, // 分母线刀闸
    INTERLOCK_OP_CLOSE_LINE_ISOLATOR  = 5, // 合出线刀闸
    INTERLOCK_OP_OPEN_LINE_ISOLATOR   = 6, // 分出线刀闸
    INTERLOCK_OP_CLOSE_EARTH_SWITCH   = 7, // 合接地刀闸
    INTERLOCK_OP_OPEN_EARTH_SWITCH    = 8, // 分接地刀闸
    INTERLOCK_OP_OPEN_CUBICLE_DOOR    = 9  // 开网门/柜门
} interlock_operation_type_t;

typedef enum {
    INTERLOCK_PERMITTED                = 0,  // 允许操作
    INTERLOCK_BLOCK_LOADED_ISOLATOR    = -1, // 闭锁: 禁止带负荷分合隔离开关 (断路器处于合闸状态)
    INTERLOCK_BLOCK_CHARGED_EARTH      = -2, // 闭锁: 禁止带电合接地刀闸 (主回路带电或隔离开关合闸)
    INTERLOCK_BLOCK_EARTHED_CLOSING    = -3, // 闭锁: 禁止带接地刀闸合隔离开关/断路器 (接地刀闸处于合位)
    INTERLOCK_BLOCK_CHARGED_DOOR_ENTRY = -4, // 闭锁: 禁止误入带电间隔 (回路带电或接地刀闸未合)
    INTERLOCK_BLOCK_INVALID_OP         = -5  // 无效操作
} interlock_result_t;

typedef struct {
    uint32_t bay_id;                  // 间隔编号
    bool is_breaker_closed;           // 断路器合闸状态 (true:合, false:分)
    bool is_bus_isolator_closed;      // 母线隔离开关合闸状态
    bool is_line_isolator_closed;     // 出线隔离开关合闸状态
    bool is_earth_switch_closed;      // 接地刀闸合闸状态
    bool is_line_charged;             // 线路高压带电指示 (true:有电)
    bool is_busbar_charged;           // 母线高压带电指示 (true:有电)
    bool is_cubicle_door_open;        // 间隔柜门/网门状态 (true:开, false:关)
    bool is_lock_override_key_active; // 机械强制解锁钥匙使能 (仅紧急运维)
} smartbox_switchgear_bay_t;

/**
 * @brief 初始化开关柜间隔状态
 */
void smartbox_interlock_bay_init(smartbox_switchgear_bay_t *bay, uint32_t bay_id);

/**
 * @brief 执行电气五防智能闭锁合规判定
 * @param bay 间隔状态
 * @param op 期望执行的操作指令
 * @param out_reason 闭锁拦截原因描述 (可选输出)
 * @return 0 (INTERLOCK_PERMITTED) 准予操作; 负值表示对应五防逻辑闭锁拦截
 */
interlock_result_t smartbox_interlock_evaluate(const smartbox_switchgear_bay_t *bay,
                                              interlock_operation_type_t op,
                                              const char **out_reason);

#ifdef __cplusplus
}
#endif

#endif // SMARTBOX_FIVE_SAFETY_INTERLOCK_H
