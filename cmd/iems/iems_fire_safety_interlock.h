/**
 * @file iems_fire_safety_interlock.h
 * @brief Production Commercial Energy Storage Fire Suppression, Gas Detection & Safety Interlock Engine
 */

#ifndef IEMS_FIRE_SAFETY_INTERLOCK_H
#define IEMS_FIRE_SAFETY_INTERLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    FIRE_STAGE_NORMAL    = 0,
    FIRE_STAGE_1_WARNING = 1, // 早期气体释放 (CO > 30ppm / H2 > 100ppm) -> 声光报警 + 开启排风
    FIRE_STAGE_2_ALARM   = 2, // 烟雾确认或高浓度气体 -> 紧急停机 PCS + 关闭防火阀
    FIRE_STAGE_3_RELEASE = 3  // 热失控明火确认 -> 分断高压主断路器 + 倒计时释放全氟己酮灭火剂
} iems_fire_stage_t;

typedef struct {
    // 物理环境传感器采样
    double co_ppm;               // 一氧化碳浓度 (ppm)
    double h2_ppm;               // 氢气浓度 (ppm)
    double cabin_temp_c;         // 储能舱环境温度 (℃)
    bool is_smoke_detected;      // 烟感探测器触发
    bool is_water_immersion;     // 水浸传感器触发

    // 控制输出动作
    iems_fire_stage_t current_stage;
    bool is_sound_light_alarm;   // 声光报警器使能
    bool is_ventilation_fan_on;  // 防爆排风机开启
    bool is_fire_damper_closed;  // 防火阀关闭 (阻止氧气进入)
    bool is_pcs_emergency_stop;  // PCS 紧急封锁停机
    bool is_hv_breaker_tripped;  // 高压箱主断路器硬联锁分断
    bool is_gas_released;        // 灭火气体 (全氟己酮) 释放已触发
    uint32_t release_countdown_s;// 灭火延时倒计时 (秒, 留出人员疏散时间, 如 30s)
} iems_fire_safety_interlock_t;

/**
 * @brief 初始化储能舱消防与防爆联锁控制器
 */
void iems_fire_interlock_init(iems_fire_safety_interlock_t* fire);

/**
 * @brief 周期性评估气体浓度、烟感与热失控消防联锁逻辑 (100ms 周期执行)
 * @param co 一氧化碳浓度 (ppm)
 * @param h2 氢气浓度 (ppm)
 * @param temp 环境温度 (℃)
 * @param smoke 烟感状态
 * @param water 水浸状态
 * @param dt_s 步长 (秒)
 */
int iems_fire_interlock_evaluate(iems_fire_safety_interlock_t* fire,
                                 double co, double h2, double temp,
                                 bool smoke, bool water, double dt_s);

#ifdef __cplusplus
}
#endif

#endif // IEMS_FIRE_SAFETY_INTERLOCK_H
