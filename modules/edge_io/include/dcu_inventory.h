#ifndef DCU_INVENTORY_H
#define DCU_INVENTORY_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_METERS 256
#define MAX_ADDR_LEN 32

/**
 * @brief 工业生产级电能表档案实体
 */
typedef struct {
    char address[MAX_ADDR_LEN];  // 电能表通信地址 (TSA)
    char protocol[16];           // 通信协议类型 (dlt698, dlt645 等)
    uint32_t baud_rate;          // 串口波特率
    uint8_t channel_id;          // 关联的物理通道/串口 ID
    bool is_online;              // 状态：true 表示 ACTIVE, false 表示 SUSPENDED (熔断离线)
    uint8_t fail_count;          // 连续抄表超时失败计数器，达到 3 时触发静默熔断
    uint32_t last_poll_time_s;   // 上一次被调度抄读的挂钟时间戳 (秒)
} production_meter_t;

/**
 * @brief 集中器电表档案容器 (线程安全锁保护)
 */
typedef struct {
    production_meter_t meters[MAX_METERS];
    uint16_t count;
    pthread_mutex_t lock;        // 互斥安全锁
} dcu_inventory_t;

/* --- 生产级 API 函数原型声明 --- */

/**
 * @brief 初始化电表档案容器
 */
int dcu_inventory_init(dcu_inventory_t *inv);

/**
 * @brief 增加一台电表注册档案 (C11 边界安全拷贝)
 */
int dcu_inventory_add(dcu_inventory_t *inv, const char *addr, const char *proto, uint32_t baud, uint8_t channel);

/**
 * @brief 删除指定 TSA 地址的表计
 */
int dcu_inventory_remove(dcu_inventory_t *inv, const char *addr);

/**
 * @brief 根据 TSA 地址查找对应的表计档案条目
 */
production_meter_t* dcu_inventory_get(dcu_inventory_t *inv, const char *addr);

/**
 * @brief 释放资源与销毁容器
 */
void dcu_inventory_destroy(dcu_inventory_t *inv);

#endif // DCU_INVENTORY_H
