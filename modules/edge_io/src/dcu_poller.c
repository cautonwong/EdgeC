#include "dcu_poller.h"
#include <string.h>
#include <stdlib.h>
#include "sched_time.h"

/**
 * @brief 初始化轮询调度引擎 (以接地气的中文添加注释)
 */
int dcu_poller_init(dcu_poller_t *poller) {
    if (!poller) return -1;
    
    poller->scheme_count = 0;
    memset(poller->schemes, 0, sizeof(poller->schemes));
    
    if (pthread_mutex_init(&poller->lock, NULL) != 0) {
        return -2;
    }
    
    return 0;
}

/**
 * @brief 注册新采集周期方案 (具有去重与边界安全保障)
 */
int dcu_poller_add_scheme(dcu_poller_t *poller, uint32_t scheme_id, uint32_t interval_s, const uint32_t *oads, uint16_t oad_count) {
    if (!poller || !oads || oad_count == 0) return -1;
    
    pthread_mutex_lock(&poller->lock);
    
    // 1. 去重校验，确保不能有重复的方案 ID
    for (uint16_t i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id == scheme_id) {
            pthread_mutex_unlock(&poller->lock);
            return -2; // 方案已存在
        }
    }
    
    // 2. 容量检查
    if (poller->scheme_count >= MAX_SCHEMES) {
        pthread_mutex_unlock(&poller->lock);
        return -3; // 方案数量超限
    }
    
    if (oad_count > MAX_SCHEME_OADS) {
        pthread_mutex_unlock(&poller->lock);
        return -4; // OAD 数量超限
    }
    
    // 3. 安全深拷贝
    dcu_scheme_t *s = &poller->schemes[poller->scheme_count];
    s->scheme_id = scheme_id;
    s->interval_s = interval_s;
    s->oad_count = oad_count;
    
    memcpy(s->oads, oads, sizeof(uint32_t) * oad_count);
    
    poller->scheme_count++;
    
    pthread_mutex_unlock(&poller->lock);
    return 0;
}

/**
 * @brief 检索方案条目
 */
dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t *poller, uint32_t scheme_id) {
    if (!poller) return NULL;
    
    pthread_mutex_lock(&poller->lock);
    
    dcu_scheme_t *found = NULL;
    for (uint16_t i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id == scheme_id) {
            found = &poller->schemes[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&poller->lock);
    return found;
}

/**
 * @brief 生成串行抄读队列，核心实现基于 half-duplex 总线跳过 (Queue Skip) 熔断表计
 */
int dcu_poller_generate_queue(dcu_poller_t *poller, uint32_t scheme_id, dcu_inventory_t *inventory, dcu_poll_queue_t *out_queue) {
    if (!poller || !inventory || !out_queue) return -1;
    
    // 1. 获取目标采集方案
    dcu_scheme_t *s = dcu_poller_get_scheme(poller, scheme_id);
    if (!s) return -2; // 方案不存在
    
    memset(out_queue, 0, sizeof(dcu_poll_queue_t));
    time_t now = time(NULL);
    
    // 2. 线程安全地遍历已注册电能表档案，并依据各自的“采集周期”组装队列
    pthread_mutex_lock(&inventory->lock);
    
    for (uint16_t i = 0; i < inventory->count; i++) {
        production_meter_t *m = &inventory->meters[i];
        
        // 【核心生产级逻辑】：根据当前表计是在线状态还是 SUSPENDED 熔断降级状态进行周期对齐判断
        if (m->is_online) {
            // 在线状态下：检查是否达到了方案规定的高频常规采集周期 (例如 15 秒)
            // 如果 last_poll_time_s 为 0 代表首次上电抄读，则无条件通过
            if (m->last_poll_time_s > 0 && (now - (time_t)m->last_poll_time_s < (time_t)s->interval_s)) {
                continue; // 抄读周期未到，安全跳过
            }
        } else {
            // 熔断挂起状态下 (SUSPENDED)：将采集周期降级为 10 分钟一次 (600 秒) 的低频心跳探测
            // 彻底杜绝坏表占用通道资源导致全台区遥测大面积超时延时抖动
            if (m->last_poll_time_s == 0) {
                m->last_poll_time_s = (uint32_t)now;
                continue; // 首次进入熔断，初始化计时点并跳过抄读
            }
            if (now - (time_t)m->last_poll_time_s < 600) {
                continue; // 降级周期未到 (不满10分钟)，静默跳过，彻底杜绝抖动
            }
        }
        
        // 容量检验，防止越界
        if (out_queue->count >= MAX_QUEUE_ENTRIES) {
            break; 
        }
        
        // 组装队列节点
        dcu_poll_entry_t *entry = &out_queue->entries[out_queue->count];
        
        strncpy(entry->meter_address, m->address, MAX_ADDR_LEN - 1);
        entry->meter_address[MAX_ADDR_LEN - 1] = '\0';
        
        // 深度拷贝要抄取的 OAD 数据
        entry->oad_count = s->oad_count;
        memcpy(entry->oads, s->oads, sizeof(uint32_t) * s->oad_count);
        
        out_queue->count++;
    }
    
    pthread_mutex_unlock(&inventory->lock);
    return 0;
}

/**
 * @brief 释放引擎锁资源
 */
void dcu_poller_destroy(dcu_poller_t *poller) {
    if (!poller) return;
    pthread_mutex_destroy(&poller->lock);
}

/* ==============================================================================
 * 动态高频采集数据生产模拟器实现 (Task 4)
 * ============================================================================== */

#include <uv.h>
#include "datahub.h"

// 统计模拟抄表触发次数的全局计数器 (供 CTest 断言)
uint32_t g_simulated_count = 0;

static void _sim_periodic_cb(uv_periodic_t *handle) {
    dcu_poller_t *poller = (dcu_poller_t *)handle->data;
    if (!poller || !poller->sim_running) return;

    dcu_inventory_t *inv = (dcu_inventory_t *)poller->inventory_ptr;
    datahub_t *hub = (datahub_t *)poller->hub_ptr;
    if (!inv || !hub) return;

    // 1. 生成物理串行轮询队列 (自动跳过 SUSPENDED 熔断电表)
    dcu_poll_queue_t queue;
    // 这里我们使用方案1 (Task 4 中注册 of 10ms 定时采样方案)
    if (dcu_poller_generate_queue(poller, 1, inv, &queue) != 0) {
        return;
    }

    // 2. 串行模拟读取电参量并发布 DataHub (以接地气的中文添加注释)
    time_t poll_time = time(NULL);
    for (uint16_t i = 0; i < queue.count; i++) {
        dcu_poll_entry_t *entry = &queue.entries[i];
        
        // 调度更新：记录该电表上一次被调度抄读的绝对挂钟时间，确保下次周期间隔计算正确
        production_meter_t *m = dcu_inventory_get(inv, entry->meter_address);
        if (m) {
            m->last_poll_time_s = (uint32_t)poll_time;
        }
        
        for (uint16_t j = 0; j < entry->oad_count; j++) {
            double sim_value = 0.0;
            uint32_t oad = entry->oads[j];
            
            // 依据不同 OAD 类型模拟合理的物理数值
            if (oad == 0x20000200 || oad == 0x20000300) {
                // 模拟 220V 左右的单相电压
                sim_value = 220.0 + ((double)(rand() % 10) - 5.0) * 0.1;
            } else if (oad == 0x20010200 || oad == 0x20010300) {
                // 模拟 5A 左右的电流
                sim_value = 5.0 + ((double)(rand() % 10) - 5.0) * 0.05;
            } else if (oad == 0x20040200 || oad == 0x20040400) {
                // 有功功率 P = U * I
                sim_value = 1100.0 + ((double)(rand() % 100) - 50.0) * 0.5;
            } else if (oad == 0x00100200 || oad == 0x00100100) {
                // 累计正向有功电能量 (随时间单调递增，模拟累加)
                static double s_accumulated_energy = 12543.21;
                s_accumulated_energy += 0.001; 
                sim_value = s_accumulated_energy;
            } else {
                sim_value = 1.0;
            }
            
            // 3. 构建 DataHub 总线消息并发布
            pipe_msg_t msg;
            memset(&msg, 0, sizeof(pipe_msg_t));
            msg.tag_id = oad; // 将 OAD 映射为 tag_id 发布
            msg.type = PIPEMSG_TYPE_SCALAR;
            msg.payload.val.f = (float)sim_value;
            
            datahub_publish(hub, &msg);
            g_simulated_count++; // 递增全局生产计数器
        }
    }
}

/**
 * @brief 启动高频动态数采物理模拟器 (使用 high-precision uv_periodic_t 对齐循环)
 */
int dcu_poller_start_simulation(dcu_poller_t *poller, dcu_inventory_t *inventory, void *hub_ptr, void *loop_ptr) {
    if (!poller || !inventory || !hub_ptr || !loop_ptr) return -1;
    
    pthread_mutex_lock(&poller->lock);
    
    poller->inventory_ptr = inventory;
    poller->hub_ptr = hub_ptr;
    poller->sim_running = true;
    
    // 【高精度对齐 MTT】：分配并初始化 uv_periodic_t 周期对齐调度结构
    uv_periodic_t *periodic = malloc(sizeof(uv_periodic_t));
    uv_periodic_init((uv_loop_t *)loop_ptr, periodic);
    periodic->data = poller;
    
    // 以 10.0 ms 为绝对时间对齐步长设置周期调度策略 (offset = 0.0 ms, interval = 10.0 ms)
    uv_periodic_set(periodic, 0.0, 10.0, NULL);
    
    // 启动周期对齐调度引擎
    uv_periodic_start(periodic, _sim_periodic_cb);
    
    poller->sim_timer_ptr = periodic;
    g_simulated_count = 0; // 计数器复位
    
    pthread_mutex_unlock(&poller->lock);
    return 0;
}

/**
 * @brief 关闭模拟器并清理资源 (配合 uv_periodic_stop 释放底层句柄)
 */
void dcu_poller_stop_simulation(dcu_poller_t *poller) {
    if (!poller || !poller->sim_running) return;
    
    pthread_mutex_lock(&poller->lock);
    
    poller->sim_running = false;
    uv_periodic_t *periodic = (uv_periodic_t *)poller->sim_timer_ptr;
    if (periodic) {
        uv_periodic_stop(periodic);
        // 执行异步 close 释放 periodic 内部的 timer 句柄，并在系统回调中安全 free 物理内存
        uv_close((uv_handle_t *)&periodic->timer, (uv_close_cb)free);
        poller->sim_timer_ptr = NULL;
    }
    
    pthread_mutex_unlock(&poller->lock);
}
