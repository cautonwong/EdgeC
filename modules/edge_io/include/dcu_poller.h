#ifndef DCU_POLLER_H
#define DCU_POLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "dcu_inventory.h"

#define MAX_SCHEMES 32
#define MAX_SCHEME_OADS 64
#define MAX_QUEUE_ENTRIES 256

/**
 * @brief 采集方案结构体 (OI 6012 配置)
 */
typedef struct {
    uint32_t scheme_id;               // 方案/任务 ID
    uint32_t interval_s;              // 采集轮询时间周期 (秒)
    uint32_t oads[MAX_SCHEME_OADS];   // 该方案需要采集抄读的 OAD 队列
    uint16_t oad_count;               // OAD 数量
} dcu_scheme_t;

/**
 * @brief 集中器采集调度引擎主体
 */
typedef struct {
    dcu_scheme_t schemes[MAX_SCHEMES];
    uint16_t scheme_count;
    pthread_mutex_t lock;             // 线程安全锁
    
    // 模拟运行期变量 (Task 4)
    void *sim_timer_ptr;              // 避免在公共头文件里引入 uv_timer_t，防止冲突
    void *hub_ptr;
    void *inventory_ptr;
    bool sim_running;
} dcu_poller_t;

/**
 * @brief 串行物理抄读队列的单表执行节点
 */
typedef struct {
    char meter_address[MAX_ADDR_LEN]; // 目标电表 TSA 地址
    uint32_t oads[MAX_SCHEME_OADS];   // 要抄读的 OAD 数组
    uint16_t oad_count;               // OAD 数量
} dcu_poll_entry_t;

/**
 * @brief RS485 半双工通道串行执行队列容器 (Queue)
 */
typedef struct {
    dcu_poll_entry_t entries[MAX_QUEUE_ENTRIES];
    uint16_t count;                   // 队列中节点数量
} dcu_poll_queue_t;

/* --- 生产级 API 函数原型声明 --- */

/**
 * @brief 初始化轮询调度引擎
 */
int dcu_poller_init(dcu_poller_t *poller);

/**
 * @brief 注册新周期采集方案 (去重并安全边界拷贝)
 */
int dcu_poller_add_scheme(dcu_poller_t *poller, uint32_t scheme_id, uint32_t interval_s, const uint32_t *oads, uint16_t oad_count);

/**
 * @brief 根据方案 ID 检索具体的采集方案
 */
dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t *poller, uint32_t scheme_id);

/**
 * @brief 核心物理调度：根据指定方案与电表激活档案，生成半双工串行执行队列 (支持熔断表过滤 Queue Skip)
 */
int dcu_poller_generate_queue(dcu_poller_t *poller, uint32_t scheme_id, dcu_inventory_t *inventory, dcu_poll_queue_t *out_queue);

/**
 * @brief 释放资源与销毁调度引擎
 */
void dcu_poller_destroy(dcu_poller_t *poller);

/* --- 动态高频采集数据生产模拟器 API (Task 4) --- */
int dcu_poller_start_simulation(dcu_poller_t *poller, dcu_inventory_t *inventory, void *hub_ptr, void *loop_ptr);
void dcu_poller_stop_simulation(dcu_poller_t *poller);

#endif // DCU_POLLER_H
