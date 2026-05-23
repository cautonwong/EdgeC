#include "dcu_poller.h"
#include <string.h>
#include <stdlib.h>

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
    
    // 2. 线程安全地遍历已注册电能表档案，并组装队列
    pthread_mutex_lock(&inventory->lock);
    
    for (uint16_t i = 0; i < inventory->count; i++) {
        production_meter_t *m = &inventory->meters[i];
        
        // 【核心生产级逻辑】：如果表计处于离线熔断状态 (SUSPENDED)，瞬间跳过 (Skip) 执行！
        // 这将保证坏表在物理总线抄表队列上占用的调度时间为 0，保全其余在线电表。
        if (!m->is_online) {
            continue; 
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
