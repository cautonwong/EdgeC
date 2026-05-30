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
    if (uv_mutex_init(&inv->lock) != 0) {
        return -2;
    }
    
    return 0;
}

/**
 * @brief 注册新电表，具备去重和边界保障
 */
int dcu_inventory_add(dcu_inventory_t *inv, const char *addr, const char *proto, uint32_t baud, uint8_t channel, const driver_contract_injection_t *inject) {
    if (!inv || !addr || !proto) return -1;
    
    uv_mutex_lock(&inv->lock);
    
    // 1. 去重校验，确保不能有重复的 TSA 地址
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            uv_mutex_unlock(&inv->lock);
            return -2; // 档案已存在
        }
    }
    
    // 2. 容量检查
    if (inv->count >= MAX_METERS) {
        uv_mutex_unlock(&inv->lock);
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
    m->last_poll_time_s = 0; // 上一次抄表时间戳初始化为0
    m->inject = inject;    // [Nexus] 挂接契约注入
    
    inv->count++;
    
    uv_mutex_unlock(&inv->lock);
    return 0;
}

/**
 * @brief 删除指定表计
 */
int dcu_inventory_remove(dcu_inventory_t *inv, const char *addr) {
    if (!inv || !addr) return -1;
    
    uv_mutex_lock(&inv->lock);
    
    int index = -1;
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        uv_mutex_unlock(&inv->lock);
        return -2; // 没找到该表计
    }
    
    // 用最后一个元素移到当前位置进行覆盖，保证紧凑布局
    if (index < inv->count - 1) {
        memcpy(&inv->meters[index], &inv->meters[inv->count - 1], sizeof(production_meter_t));
    }
    
    // 清空最后一个元素
    memset(&inv->meters[inv->count - 1], 0, sizeof(production_meter_t));
    inv->count--;
    
    uv_mutex_unlock(&inv->lock);
    return 0;
}

/**
 * @brief 根据 TSA 查找表计条目 (线程安全地获取内部地址指针)
 */
production_meter_t* dcu_inventory_get(dcu_inventory_t *inv, const char *addr) {
    if (!inv || !addr) return NULL;
    
    uv_mutex_lock(&inv->lock);
    
    production_meter_t *found = NULL;
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            found = &inv->meters[i];
            break;
        }
    }
    
    uv_mutex_unlock(&inv->lock);
    return found;
}

/**
 * @brief 资源回收与销毁容器
 */
#include <sqlite3.h>
#include <stdio.h>
#include "contract_registry.h"
#include "sqlite_service.h"

int dcu_inventory_load_from_db(dcu_inventory_t *inv, const char *db_path) {
    sqlite3 *db;
    if (sqlite3_open(db_path, &db) != SQLITE_OK) return -1;

    // [Professional Schema] 对齐 PRD 6001 完整字段
    const char *init_sql = "CREATE TABLE IF NOT EXISTS meter_inventory ("
                           "address TEXT PRIMARY KEY, protocol TEXT, baud INTEGER, channel INTEGER, "
                           "wiring_mode INTEGER, collector_addr TEXT, asset_no TEXT, "
                           "pt_ratio INTEGER, ct_ratio INTEGER);";
    sqlite3_exec(db, init_sql, NULL, NULL, NULL);

    const char *sql = "SELECT address, protocol, baud, channel, wiring_mode, collector_addr, asset_no, pt_ratio, ct_ratio "
                      "FROM meter_inventory;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return -2;
    }

    int loaded = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *addr = (const char *)sqlite3_column_text(stmt, 0);
        const char *proto = (const char *)sqlite3_column_text(stmt, 1);
        int baud = sqlite3_column_int(stmt, 2);
        int chan = sqlite3_column_int(stmt, 3);
        
        // 自动查找契约注入点
        const driver_contract_injection_t *inj = edge_model_find(proto);
        
        // 线程安全地添加到内存容器
        uv_mutex_lock(&inv->lock);
        if (inv->count < MAX_METERS) {
            production_meter_t *m = &inv->meters[inv->count++];
            strncpy(m->address, addr, MAX_ADDR_LEN - 1);
            strncpy(m->protocol, proto, 15);
            m->baud_rate = (uint32_t)baud;
            m->channel_id = (uint8_t)chan;
            m->inject = inj;
            m->is_online = true;

            // [PRD 补完]
            m->wiring_mode = (uint8_t)sqlite3_column_int(stmt, 4);
            strncpy(m->collector_addr, (const char*)sqlite3_column_text(stmt, 5), 15);
            strncpy(m->asset_no, (const char*)sqlite3_column_text(stmt, 6), 31);
            m->pt_ratio = (uint32_t)sqlite3_column_int(stmt, 7);
            m->ct_ratio = (uint32_t)sqlite3_column_int(stmt, 8);
        }
        uv_mutex_unlock(&inv->lock);
        loaded++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return loaded;
}

void dcu_inventory_destroy(dcu_inventory_t *inv) {
    if (!inv) return;
    uv_mutex_destroy(&inv->lock);
}
