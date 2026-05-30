#ifndef DCU_POLLER_H
#define DCU_POLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "dcu_inventory.h"

#define MAX_TASKS 16
#define MAX_SCHEMES 16
#define MAX_SCHEME_OADS 32
#define MAX_QUEUE_ENTRIES 128
#define DCU_MAX_CHANNELS  32

#include "queue.h"

/**
 * @brief DL/T 698.45 采集任务监控单元 (OI 6035)
 */
typedef struct {
    uint32_t task_id;                 // 关联任务编号
    uint8_t status;                   // 状态 (0:未执行, 1:执行中, 2:已完成)
    uint32_t total_meters;            // 应抄表总数
    uint32_t success_meters;          // 抄表成功数
    uint32_t total_packets;           // 物理信道收发总包数
} dcu_task_monitor_t;

/**
 * @brief DL/T 698.45 普通采集方案 (OI 6015)
 */
typedef struct {
    uint32_t scheme_id;               // 方案编号
    uint8_t capture_mode;             // 采集方式 (0:当前数据, 1:按冻结时标, 2:按时标间隔, 3:补抄)
    
    // [PRD Alignment] 6015 附加属性: 电能表集合 MS
    uint8_t ms_type;                  // MS 类型 (0:全体, 1:指定组, 2:指定多块表)
    char ms_addrs[8][MAX_ADDR_LEN];   // 如果是指定多块表，此数组生效
    uint8_t ms_count;                 // 指定表的数量

    uint32_t csd_list[MAX_SCHEME_OADS]; // 记录列选择 array CSD (要抄读的数据，如 0x20000200)
    uint16_t csd_count;               // CSD 数量
} dcu_scheme_t;

/**
 * @brief DL/T 698.45 任务配置单元 (OI 6013) - 升级为 Handle 风格
 */
typedef struct {
    uint32_t task_id;                 
    uint32_t interval_s;              
    uint8_t scheme_type;              
    uint32_t scheme_id;               
    uint8_t run_time_window[4];       
    uint8_t priority;                 

    uint64_t last_run_time;           
    bool is_active;                   
    
    // [Nexus-Pro] 运行期状态追踪
    uint16_t cursor_meter_idx;        // 扫描游标
    uint16_t active_cmds;             // 该任务当前在飞的包数量
    dcu_task_monitor_t monitor;       // 实时 6035 监控
    struct uv__queue node;            // 挂载到执行队列
} dcu_task_unit_t;

/**
 * @brief 补抄追踪条目 (用于 RETRY_QUEUE)
 */
typedef struct {
    char meter_address[MAX_ADDR_LEN];
    uint32_t oad;
    uint8_t retry_count;
    uint8_t channel_id;
    uint32_t task_id;
    struct uv__queue node;
} dcu_retry_item_t;

/**
 * @brief 集中器定时抄表调度大脑 (异步泵送引擎)
 */
typedef struct {
    dcu_task_unit_t tasks[MAX_TASKS];
    uint16_t task_count;
    
    dcu_scheme_t schemes[MAX_SCHEMES];
    uint16_t scheme_count;
    
    // [Nexus-Pro] 调度链表
    struct uv__queue active_tasks;    // 正在执行扫描的任务
    struct uv__queue retry_queues[DCU_MAX_CHANNELS]; // 每个通道专属的重试优先队列
    uint8_t in_flight_per_chan[DCU_MAX_CHANNELS];    // 通道背压计数器
    
    uv_mutex_t lock;             
    
    void *sim_timer_ptr;              
    void *hub_ptr;
    void *inventory_ptr;
    void *drv_mgr_ptr;                
    bool sim_running;
} dcu_poller_t;

/**
 * @brief 抄表执行节点实体
 */
typedef struct {
    char meter_address[MAX_ADDR_LEN]; // TSA 物理通信地址
    uint32_t oads[MAX_SCHEME_OADS];   // 需要物理抄取的 CSD 列表
    uint16_t oad_count;
} dcu_poll_entry_t;

/**
 * @brief 调度引擎生成的物理执行包队列
 */
typedef struct {
    dcu_poll_entry_t entries[MAX_QUEUE_ENTRIES];
    uint16_t count;
} dcu_poll_queue_t;

/* --- 698.45 规约核心调度原型 API --- */

/**
 * @brief 初始化调度引擎
 */
int dcu_poller_init(dcu_poller_t *poller);

/**
 * @brief 动态添加任务单元 (OI 6012 Method Add Task)
 */
int dcu_poller_add_task(dcu_poller_t *poller, uint32_t task_id, uint32_t interval_s, uint32_t scheme_id);

/**
 * @brief 动态注册采集方案 (OI 6014 Method Add Scheme)
 */
int dcu_poller_add_scheme(dcu_poller_t *poller, uint32_t scheme_id, uint8_t mode, const uint32_t *csds, uint16_t csd_count);

/**
 * @brief 检索方案条目
 */
dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t *poller, uint32_t scheme_id);

/**
 * @brief 核心物理调度：周期性时间对齐检查，生成物理总线待抄队列
 */
int dcu_poller_generate_queue(dcu_poller_t *poller, dcu_inventory_t *inventory, dcu_poll_queue_t *out_queue, uint64_t now_s);

/**
 * @brief 销毁调度引擎
 */
void dcu_poller_destroy(dcu_poller_t *poller);

#include "driver_manager.h"

/* --- 定期时间对齐调度引擎 (Matrix IO 驱动) --- */
int dcu_poller_start(dcu_poller_t *poller, dcu_inventory_t *inventory, void *orch_ptr, driver_manager_t *drv_mgr, void *loop_ptr);
void dcu_poller_stop(dcu_poller_t *poller);

#endif // DCU_POLLER_H
