#include "dcu_inventory.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "dcu_driver_adapter.h"
#include "edge_log.h"
#include "sqlite_service.h"

/**
 * @brief 初始化电表档案容器 (以接地气的中文添加注释)
 */
int dcu_inventory_init(dcu_inventory_t* inv) {
    if (!inv) return -1;

    inv->count = 0;
    inv->revision = 0;
    memset(inv->meters, 0, sizeof(inv->meters));

    return 0;
}

/**
 * @brief 注册新电表，具备去重和边界保障
 */
/* 分配一个设备槽位号。
 *
 * 优先单调递增（不复用）：复用会让新表继承旧表在影子表里的残留值，
 * 首次抄表前读到的是上一只表的数——那是"错数据"，比"没数据"更难查。
 * 只有 4096 个号用尽后才回收空洞（那时旧值早已被覆盖过很多轮）。 */
static int _alloc_slot(dcu_inventory_t* inv) {
    if (inv->next_slot < MAX_METERS) {
        return (int)inv->next_slot++;
    }
    for (uint32_t slot = 0; slot < MAX_METERS; slot++) {
        bool used = false;
        for (uint16_t i = 0; i < inv->count; i++) {
            if (inv->meters[i].slot_id == slot) {
                used = true;
                break;
            }
        }
        if (!used) return (int)slot;
    }
    return -1;
}

const production_meter_t* dcu_inventory_by_slot(const dcu_inventory_t* inv, uint32_t slot_id) {
    if (!inv) return NULL;
    for (uint16_t i = 0; i < inv->count; i++) {
        if (inv->meters[i].slot_id == slot_id) return &inv->meters[i];
    }
    return NULL;
}

int dcu_inventory_add(dcu_inventory_t* inv, const char* addr, const char* proto, uint32_t baud,
                      uint8_t channel) {
    if (!inv || !addr || !proto) return -1;

    // 1. 去重校验，确保不能有重复的 TSA 地址
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            return -2;  // 档案已存在
        }
    }

    // 2. 容量检查
    if (inv->count >= MAX_METERS) {
        return -3;  // 容器已满
    }

    // 3. 边界安全填充
    production_meter_t* m = &inv->meters[inv->count];

    strncpy(m->address, addr, MAX_ADDR_LEN - 1);
    m->address[MAX_ADDR_LEN - 1] = '\0';

    strncpy(m->protocol, proto, sizeof(m->protocol) - 1);
    m->protocol[sizeof(m->protocol) - 1] = '\0';

    m->baud_rate = baud;
    m->channel_id = channel;
    m->is_online = true;      // 新增时初始置为在线状态 (ACTIVE)
    m->last_poll_time_s = 0;  // 上一次抄表时间戳初始化为0

    int slot = _alloc_slot(inv);
    if (slot < 0) return -4;  // 槽位耗尽：宁可拒绝，也不能让两只表共用一个影子槽
    m->slot_id = (uint32_t)slot;

    inv->count++;
    inv->revision++;

    return 0;
}

/**
 * @brief 删除指定表计
 */
int dcu_inventory_remove(dcu_inventory_t* inv, const char* addr) {
    if (!inv || !addr) return -1;

    int index = -1;
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return -2;  // 没找到该表计
    }

    /* 用最后一个元素覆盖删除位，保持紧凑布局。
     * 注意：**数组下标会变，但 slot_id 跟着表走**（整体 memcpy 一起搬），
     * 所以被挪动的那只表在影子表里的位置不变。 */
    if (index < inv->count - 1) {
        memcpy(&inv->meters[index], &inv->meters[inv->count - 1], sizeof(production_meter_t));
    }

    // 清空最后一个元素
    memset(&inv->meters[inv->count - 1], 0, sizeof(production_meter_t));
    inv->count--;
    inv->revision++;

    return 0;
}

/**
 * @brief 根据 TSA 查找表计条目 (线程安全地获取内部地址指针)
 */
production_meter_t* dcu_inventory_get(dcu_inventory_t* inv, const char* addr) {
    if (!inv || !addr) return NULL;

    production_meter_t* found = NULL;
    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, addr) == 0) {
            found = &inv->meters[i];
            break;
        }
    }

    return found;
}

const char* dcu_inventory_channel_protocol(const dcu_inventory_t* inv, uint8_t channel_id) {
    if (!inv) return NULL;

    const char* proto = NULL;
    for (uint16_t i = 0; i < inv->count; i++) {
        const production_meter_t* m = &inv->meters[i];
        if (m->channel_id != channel_id || m->protocol[0] == '\0') continue;

        if (!proto) {
            proto = m->protocol;
        } else if (strcasecmp(proto, m->protocol) != 0) {
            LOG_WARN(LOG_CAT_SCHED, "⚠️ [ARCHIVE] channel %u mixes protocols (%s vs %s); using %s",
                     channel_id, proto, m->protocol, proto);
            break;  // 只告警一次
        }
    }
    return proto;
}

/**
 * @brief 资源回收与销毁容器
 */
int dcu_inventory_load_from_db(dcu_inventory_t* inv, const char* db_path) {
    sqlite3* db;
    if (sqlite3_open(db_path, &db) != SQLITE_OK) return -1;

    // [Professional Schema] 对齐 PRD 6001 完整字段
    const char* init_sql =
        "CREATE TABLE IF NOT EXISTS meter_inventory ("
        "address TEXT PRIMARY KEY, protocol TEXT, baud INTEGER, channel INTEGER, "
        "wiring_mode INTEGER, collector_addr TEXT, asset_no TEXT, "
        "pt_ratio INTEGER, ct_ratio INTEGER);";
    sqlite3_exec(db, init_sql, NULL, NULL, NULL);

    const char* sql =
        "SELECT address, protocol, baud, channel, wiring_mode, collector_addr, asset_no, pt_ratio, "
        "ct_ratio "
        "FROM meter_inventory "
        /* 顺序必须确定：slot_id 按加载顺序分配，顺序一变，重启后同一只表的
         * 影子槽位就换了。 */
        "ORDER BY address ASC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return -2;
    }

    int loaded = 0;
    uint32_t inserted = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* addr = (const char*)sqlite3_column_text(stmt, 0);
        const char* proto = (const char*)sqlite3_column_text(stmt, 1);
        int baud = sqlite3_column_int(stmt, 2);
        int chan = sqlite3_column_int(stmt, 3);

        if (inv->count < MAX_METERS) {
            production_meter_t* m = &inv->meters[inv->count++];
            strncpy(m->address, addr, MAX_ADDR_LEN - 1);
            m->address[MAX_ADDR_LEN - 1] = '\0';

            strncpy(m->protocol, proto, 15);
            m->protocol[15] = '\0';

            m->baud_rate = (uint32_t)baud;
            m->channel_id = (uint8_t)chan;
            m->is_online = true;
            m->slot_id = (uint32_t)_alloc_slot(inv);

            // [PRD 补完] 严防数据库空值
            m->wiring_mode = (uint8_t)sqlite3_column_int(stmt, 4);
            const char* c_addr = (const char*)sqlite3_column_text(stmt, 5);
            if (c_addr) {
                strncpy(m->collector_addr, c_addr, 15);
                m->collector_addr[15] = '\0';
            }

            const char* a_no = (const char*)sqlite3_column_text(stmt, 6);
            if (a_no) {
                strncpy(m->asset_no, a_no, 31);
                m->asset_no[31] = '\0';
            }

            m->pt_ratio = (uint32_t)sqlite3_column_int(stmt, 7);
            m->ct_ratio = (uint32_t)sqlite3_column_int(stmt, 8);
            inserted++;
        }
        loaded++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    inv->revision += inserted; // 只统计真的入库行：超出 MAX_METERS 被丢弃的不算变更
    return loaded;
}

void dcu_inventory_destroy(dcu_inventory_t* inv) {
    if (!inv) return;
}
