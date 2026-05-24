#ifndef DCU_DISCOVERY_H
#define DCU_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_DISCOVERED_METERS 16
#define DISCOVERY_ADDR_LEN 32

typedef enum {
    DISCOVERY_STATUS_IDLE = 0,       // 空闲状态
    DISCOVERY_STATUS_SCANNING = 1,   // 正在扫描下行物理总线中
    DISCOVERY_STATUS_COMPLETED = 2   // 扫描完成
} discovery_status_t;

/**
 * @brief 自动搜表发现业务管理器 (OI 6002/6003 核心模型)
 */
typedef struct {
    uint8_t status;                                          // 当前搜表状态 (discovery_status_t)
    char discovered_tsas[MAX_DISCOVERED_METERS][DISCOVERY_ADDR_LEN]; // 发现的未登记电表 TSA 物理地址数组
    uint16_t count;                                          // 搜出的新表数量
    pthread_mutex_t lock;                                    // 线程安全互斥锁
} dcu_discovery_t;

/**
 * @brief 初始化自动搜表模块
 */
void dcu_discovery_init(dcu_discovery_t *disc);

/**
 * @brief 启动自动/物理搜表方法 127 (Start Discovery)
 * @param duration_sec 搜表持续时间 (秒)
 * @return 0 成功，负数失败
 */
int dcu_discovery_start(dcu_discovery_t *disc, uint32_t duration_sec);

/**
 * @brief 模拟搜表后台物理扫描帧捕获 (在任务循环或管线中触发)
 * 扫描并搜出新的表计 TSA 物理地址，写入搜表结果
 */
void dcu_discovery_tick_simulate(dcu_discovery_t *disc);

/**
 * @brief 销毁搜表管理器
 */
void dcu_discovery_destroy(dcu_discovery_t *disc);

#endif // DCU_DISCOVERY_H
