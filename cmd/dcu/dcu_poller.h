#ifndef DCU_POLLER_H
#define DCU_POLLER_H

#include <stdint.h>
#include <stdbool.h>
#include <uv.h>
#include "dcu_inventory.h"
#include "queue.h"

// 前向声明驱动管理器，避免头文件循环依赖
struct driver_manager_s;
typedef struct driver_manager_s driver_manager_t;

#define MAX_TASKS 16
#define MAX_SCHEMES 16
#define MAX_SCHEME_OADS 32
#define MAX_QUEUE_ENTRIES 128
#define DCU_MAX_CHANNELS  32

/**
 * @brief DL/T 698.45 采集任务监控单元 (OI 6035)
 */
typedef struct {
    uint32_t task_id;                 
    uint8_t status;                   
    uint32_t total_meters;            
    uint32_t success_meters;          
    uint32_t total_packets;           
} dcu_task_monitor_t;

/**
 * @brief DL/T 698.45 方案类型枚举
 */
typedef enum {
    SCHEME_TYPE_NORMAL      = 1, 
    SCHEME_TYPE_EVENT       = 2, 
    SCHEME_TYPE_TRANSPARENT = 3, 
    SCHEME_TYPE_REPORT      = 4, 
    SCHEME_TYPE_SCRIPT      = 5  
} dcu_scheme_type_t;

typedef struct {
    uint32_t scheme_id;
    dcu_scheme_type_t type;
    union {
        struct {
            uint8_t capture_mode;
            uint8_t ms_type;
            char ms_addrs[8][MAX_ADDR_LEN];
            uint8_t ms_count;
            uint32_t csd_list[MAX_SCHEME_OADS];
            uint16_t csd_count;
        } normal;
    } data;
} dcu_scheme_t;

typedef struct {
    uint32_t task_id;                 
    uint32_t interval_s;              
    uint8_t scheme_type;              
    uint32_t scheme_id;               
    uint16_t cursor_meter_idx;        
    dcu_task_monitor_t monitor;       
} dcu_task_unit_t;

typedef struct {
    dcu_task_unit_t tasks[MAX_TASKS];
    uint16_t task_count;
    dcu_scheme_t schemes[MAX_SCHEMES];
    uint16_t scheme_count;
    
    uv_timer_t timer; // 【补全】
    uv_mutex_t lock;
    
    void *hub;        // 【补全】
    void *inventory;  // 【补全】
    void *orch;       // 【补全】
    void *drv_mgr;    // 【补全】
} dcu_poller_t;

int dcu_poller_init(dcu_poller_t *poller);
int dcu_poller_add_task(dcu_poller_t *poller, uint32_t task_id, uint32_t interval_s, uint32_t scheme_id);
int dcu_poller_add_scheme(dcu_poller_t *poller, uint32_t scheme_id, uint8_t mode, const uint32_t *csds, uint16_t csd_count);
dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t *poller, uint32_t scheme_id);
void dcu_poller_pump(dcu_poller_t *poller);
int dcu_poller_start(dcu_poller_t *poller, dcu_inventory_t *inventory, void *orch_ptr, void *hub_ptr, driver_manager_t *drv_mgr, void *loop_ptr);
void dcu_poller_stop(dcu_poller_t *poller);

#endif
