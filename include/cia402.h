/**
 * @file cia402.h
 * @brief CANopen CiA 402 Drive Profile Definitions
 */

#ifndef EDGE_CIA402_H
#define EDGE_CIA402_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief CiA 402 State Machine States
 */
typedef enum {
    CIA402_STATE_NOT_READY_TO_SWITCH_ON = 0,
    CIA402_STATE_SWITCH_ON_DISABLED     = 1,
    CIA402_STATE_READY_TO_SWITCH_ON     = 2,
    CIA402_STATE_SWITCHED_ON            = 3,
    CIA402_STATE_OPERATION_ENABLED      = 4,
    CIA402_STATE_QUICK_STOP_ACTIVE      = 5,
    CIA402_STATE_FAULT_REACTION_ACTIVE  = 6,
    CIA402_STATE_FAULT                  = 7,
    CIA402_STATE_UNKNOWN                = 0xFF
} cia402_state_t;

/**
 * @brief Controlword (0x6040) Bit Definitions
 */
#define CIA402_CONTROL_SWITCH_ON        0x0001
#define CIA402_CONTROL_ENABLE_VOLTAGE   0x0002
#define CIA402_CONTROL_QUICK_STOP       0x0004
#define CIA402_CONTROL_ENABLE_OPERATION 0x0008
#define CIA402_CONTROL_FAULT_RESET      0x0080
#define CIA402_CONTROL_HALT             0x0100

/**
 * @brief Statusword (0x6041) Bit Definitions
 */
#define CIA402_STATUS_READY_TO_SWITCH_ON 0x0001
#define CIA402_STATUS_SWITCHED_ON        0x0002
#define CIA402_STATUS_OPERATION_ENABLED  0x0004
#define CIA402_STATUS_FAULT              0x0008
#define CIA402_STATUS_VOLTAGE_ENABLED    0x0010
#define CIA402_STATUS_QUICK_STOP         0x0020
#define CIA402_STATUS_SWITCH_ON_DISABLED 0x0040
#define CIA402_STATUS_WARNING           0x0080
#define CIA402_STATUS_REMOTE            0x0200
#define CIA402_STATUS_TARGET_REACHED    0x0400
#define CIA402_STATUS_INTERNAL_LIMIT    0x0800

/**
 * @brief CiA 402 Standard Objects (Index)
 */
#define CIA402_OBJ_CONTROLWORD          0x6040
#define CIA402_OBJ_STATUSWORD           0x6041
#define CIA402_OBJ_MODES_OF_OPERATION   0x6060
#define CIA402_OBJ_MODES_DISPLAY        0x6061
#define CIA402_OBJ_POSITION_ACTUAL      0x6064
#define CIA402_OBJ_VELOCITY_ACTUAL      0x606C
#define CIA402_OBJ_TARGET_POSITION      0x607A
#define CIA402_OBJ_TARGET_VELOCITY      0x60FF

/**
 * @brief 从 Statusword 解析当前状态
 */
static inline cia402_state_t cia402_get_state(uint16_t statusword) {
    if ((statusword & 0x004F) == 0x0000) return CIA402_STATE_NOT_READY_TO_SWITCH_ON;
    if ((statusword & 0x004F) == 0x0040) return CIA402_STATE_SWITCH_ON_DISABLED;
    if ((statusword & 0x006F) == 0x0021) return CIA402_STATE_READY_TO_SWITCH_ON;
    if ((statusword & 0x006F) == 0x0023) return CIA402_STATE_SWITCHED_ON;
    if ((statusword & 0x006F) == 0x0027) return CIA402_STATE_OPERATION_ENABLED;
    if ((statusword & 0x006F) == 0x0007) return CIA402_STATE_QUICK_STOP_ACTIVE;
    if ((statusword & 0x004F) == 0x000F) return CIA402_STATE_FAULT_REACTION_ACTIVE;
    if ((statusword & 0x004F) == 0x0008) return CIA402_STATE_FAULT;
    return CIA402_STATE_UNKNOWN;
}

#endif
