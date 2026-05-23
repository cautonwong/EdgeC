#include "dcu_inventory.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief 初始化电表档案容器 (以接地气的中文添加注释)
 */
int dcu_inventory_init(dcu_inventory_t *inv) {
    if (!inv) return -1;
    
    inv->count = 0;
    memset(inv->meters, 0, sizeof(inv->meters));
    
    // 初始化互斥锁，确保线程安全
    if (pthread_mutex_init(&inv->lock, NULL) != 0) {
        return -2;
    }
    
    return 0;
}

/**
 * @brief 注册新电表，具备去重和边界保障
 */
int dcu_inventory_add(dcu_inventory_t *inv, const char *addr, const char *proto, uint32_t baud, uint8_t channel) {
    if (!inv || !addr || !proto) return -1;
    
    pthread_mutex_lock(&inv->lock);
    
    // 1. 去重校验，确保不能有重复的 TSA 地址
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            pthread_mutex_unlock(&inv->lock);
            return -2; // 档案已存在
        }
    }
    
    // 2. 容量检查
    if (inv->count >= MAX_METERS) {
        pthread_mutex_unlock(&inv->lock);
        return -3; // 容器已满
    }
    
    // 3. 边界安全填充
    production_meter_t *m = &inv->meters[inv->count];
    
    strncpy(m->address, addr, MAX_ADDR_LEN - 1);
    m->address[MAX_ADDR_LEN - 1] = '\0';
    
    strncpy(m->protocol, proto, sizeof(m->protocol) - 1);
    m->protocol[sizeof(m->protocol) - 1] = '\0';
    
    m->baud_rate = baud;
    m->channel_id = channel;
    m->is_online = true;   // 新增时初始置为在线状态 (ACTIVE)
    m->fail_count = 0;     // 超时计数清零
    
    inv->count++;
    
    pthread_mutex_unlock(&inv->lock);
    return 0;
}

/**
 * @brief 删除指定表计
 */
int dcu_inventory_remove(dcu_inventory_t *inv, const char *addr) {
    if (!inv || !addr) return -1;
    
    pthread_mutex_lock(&inv->lock);
    
    int index = -1;
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        pthread_mutex_unlock(&inv->lock);
        return -2; // 没找到该表计
    }
    
    // 用最后一个元素移到当前位置进行覆盖，保证紧凑布局
    if (index < inv->count - 1) {
        memcpy(&inv->meters[index], &inv->meters[inv->count - 1], sizeof(production_meter_t));
    }
    
    // 清空最后一个元素
    memset(&inv->meters[inv->count - 1], 0, sizeof(production_meter_t));
    inv->count--;
    
    pthread_mutex_unlock(&inv->lock);
    return 0;
}

/**
 * @brief 根据 TSA 查找表计条目 (线程安全地获取内部地址指针)
 */
production_meter_t* dcu_inventory_get(dcu_inventory_t *inv, const char *addr) {
    if (!inv || !addr) return NULL;
    
    pthread_mutex_lock(&inv->lock);
    
    production_meter_t *found = NULL;
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            found = &inv->meters[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&inv->lock);
    return found;
}

/**
 * @brief 资源回收与销毁容器
 */
void dcu_inventory_destroy(dcu_inventory_t *inv) {
    if (!inv) return;
    pthread_mutex_destroy(&inv->lock);
}
