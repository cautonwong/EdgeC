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
    inv->next_slot = 0;
    inv->next_config_seq = 1;
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

static uint32_t _baud_from_code(uint8_t code);
static uint8_t _baud_to_code(uint32_t baud);
static void _copy_text(char* dst, size_t cap, const char* src);
static uint8_t _proto_to_code(const char* proto);
static const char* _proto_from_code(uint8_t code);
static int _find_seq(const dcu_inventory_t* inv, uint16_t seq);
static void _copy_unit_fields(production_meter_t* m, const dcu_archive_unit_t* unit);

int dcu_inventory_add(dcu_inventory_t* inv, const char* addr, const char* proto, uint32_t baud,
                      uint8_t channel) {
    if (!inv || !addr || !proto) return -1;

    dcu_archive_unit_t unit = {0};
    strncpy(unit.address, addr, MAX_ADDR_LEN - 1);
    unit.baud_code = _baud_to_code(baud);
    unit.protocol_code = _proto_to_code(proto);
    unit.port = channel;
    unit.wiring_mode = 0;
    int rc = dcu_inventory_add_unit(inv, &unit);
    if (rc == 0) {
        production_meter_t* m = dcu_inventory_get(inv, addr);
        if (m) {
            _copy_text(m->protocol, sizeof(m->protocol), proto);
            m->protocol_code = _proto_to_code(proto);
        }
    }
    return rc;
}

/* 标准 A.7 Basic_object 的波特率 enum → 产品 bps。 */
static const uint32_t kBaudTable[11] = {300,   600,   1200,  2400,  4800, 7200,
                                        9600,  19200, 38400, 57600, 115200};

static uint32_t _baud_from_code(uint8_t code) {
    if (code < sizeof(kBaudTable) / sizeof(kBaudTable[0])) return kBaudTable[code];
    return code == 255 ? 9600u : 0u; /* 自适应按 9600 落地 */
}

static uint8_t _baud_to_code(uint32_t baud) {
    for (uint8_t i = 0; i < sizeof(kBaudTable) / sizeof(kBaudTable[0]); i++) {
        if (kBaudTable[i] == baud) return i;
    }
    return 255;
}

static const char* _proto_from_code(uint8_t code) {
    if (code == 3) return "DLT698";
    if (code >= 1 && code <= 4) return "DLT645";
    return NULL;
}

static uint8_t _proto_to_code(const char* proto) {
    if (strcasecmp(proto, "DLT698") == 0) return 3;
    if (strcasecmp(proto, "DLT645") == 0) return 2;
    return 0;
}

static int _find_seq(const dcu_inventory_t* inv, uint16_t seq) {
    for (uint16_t i = 0; i < inv->count; i++) {
        if (inv->meters[i].config_seq == seq) return (int)i;
    }
    return -1;
}

static void _copy_text(char* dst, size_t cap, const char* src) {
    size_t n = src ? strlen(src) : 0;
    if (n >= cap) n = cap - 1;
    if (n > 0) memcpy(dst, src, n);
    dst[n] = '\0';
}

static void _copy_unit_fields(production_meter_t* m, const dcu_archive_unit_t* unit) {
    _copy_text(m->address, sizeof(m->address), unit->address);
    _copy_text(m->protocol, sizeof(m->protocol), _proto_from_code(unit->protocol_code));
    m->protocol_code = unit->protocol_code;
    m->baud_code = unit->baud_code;
    m->baud_rate = _baud_from_code(unit->baud_code);
    m->channel_id = unit->port;
    m->wiring_mode = unit->wiring_mode;
    _copy_text(m->collector_addr, sizeof(m->collector_addr), unit->collector_addr);
    _copy_text(m->asset_no, sizeof(m->asset_no), unit->asset_no);
    m->pt_ratio = unit->pt_ratio;
    m->ct_ratio = unit->ct_ratio;
    _copy_text(m->password, sizeof(m->password), unit->password);
    m->tariff_count = unit->tariff_count;
    m->user_type = unit->user_type;
    m->rated_voltage = unit->rated_voltage;
    m->rated_current = unit->rated_current;
}

int dcu_inventory_add_unit(dcu_inventory_t* inv, const dcu_archive_unit_t* unit) {
    if (!inv || !unit || unit->address[0] == '\0') return -1;
    if (!_proto_from_code(unit->protocol_code) || _baud_from_code(unit->baud_code) == 0) return -1;

    for (uint16_t i = 0; i < inv->count; i++) {
        if (strcmp(inv->meters[i].address, unit->address) == 0) return -2;
    }
    if (unit->config_seq && _find_seq(inv, unit->config_seq) >= 0) return -2;
    if (inv->count >= MAX_METERS) return -3;

    int slot = _alloc_slot(inv);
    if (slot < 0) return -4;

    production_meter_t* m = &inv->meters[inv->count];
    memset(m, 0, sizeof(*m));
    _copy_unit_fields(m, unit);
    m->config_seq = unit->config_seq ? unit->config_seq : inv->next_config_seq;
    if (m->config_seq >= inv->next_config_seq) inv->next_config_seq = (uint16_t)(m->config_seq + 1);
    m->is_online = true;
    m->last_poll_time_s = 0;
    m->slot_id = (uint32_t)slot;

    inv->count++;
    inv->revision++;
    return 0;
}

int dcu_inventory_update_basic(dcu_inventory_t* inv, uint16_t config_seq,
                               const dcu_archive_unit_t* unit) {
    if (!inv || !unit || unit->address[0] == '\0') return -1;
    if (!_proto_from_code(unit->protocol_code) || _baud_from_code(unit->baud_code) == 0) return -1;

    int index = _find_seq(inv, config_seq);
    if (index < 0) return -1;
    for (uint16_t i = 0; i < inv->count; i++) {
        if ((int)i != index && strcmp(inv->meters[i].address, unit->address) == 0) return -2;
    }
    _copy_unit_fields(&inv->meters[index], unit);
    inv->meters[index].config_seq = config_seq;
    inv->revision++;
    return 0;
}

int dcu_inventory_update_extended(dcu_inventory_t* inv, uint16_t config_seq,
                                  const dcu_archive_unit_t* unit, bool has_annex) {
    if (!inv || !unit) return -1;
    int index = _find_seq(inv, config_seq);
    if (index < 0) return -1;
    if (has_annex) return -2; /* 附属属性值数组没有存储路径，不能假成功 */

    production_meter_t* m = &inv->meters[index];
    _copy_text(m->collector_addr, sizeof(m->collector_addr), unit->collector_addr);
    _copy_text(m->asset_no, sizeof(m->asset_no), unit->asset_no);
    m->pt_ratio = unit->pt_ratio;
    m->ct_ratio = unit->ct_ratio;
    inv->revision++;
    return 0;
}

int dcu_inventory_remove_seq(dcu_inventory_t* inv, uint16_t config_seq) {
    if (!inv) return -1;
    int index = _find_seq(inv, config_seq);
    if (index < 0) return -1;
    return dcu_inventory_remove(inv, inv->meters[index].address);
}

int dcu_inventory_clear(dcu_inventory_t* inv) {
    if (!inv) return -1;
    memset(inv->meters, 0, sizeof(inv->meters));
    inv->count = 0;
    inv->next_config_seq = 1;
    inv->revision++;
    return 0;
}

const production_meter_t* dcu_inventory_by_seq(const dcu_inventory_t* inv, uint16_t config_seq) {
    if (!inv) return NULL;
    int index = _find_seq(inv, config_seq);
    return index < 0 ? NULL : &inv->meters[index];
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

    /* 增量列：旧库只建过上面 9 列，缺列时逐条补，失败（已存在）即忽略。 */
    static const char* const k_migrate[] = {
        "ALTER TABLE meter_inventory ADD COLUMN config_seq INTEGER DEFAULT 0",
        "ALTER TABLE meter_inventory ADD COLUMN baud_code INTEGER DEFAULT 0",
        "ALTER TABLE meter_inventory ADD COLUMN password TEXT",
        "ALTER TABLE meter_inventory ADD COLUMN tariff_count INTEGER DEFAULT 0",
        "ALTER TABLE meter_inventory ADD COLUMN user_type INTEGER DEFAULT 0",
        "ALTER TABLE meter_inventory ADD COLUMN rated_voltage INTEGER DEFAULT 0",
        "ALTER TABLE meter_inventory ADD COLUMN rated_current INTEGER DEFAULT 0",
    };
    for (size_t i = 0; i < sizeof(k_migrate) / sizeof(k_migrate[0]); i++) {
        sqlite3_exec(db, k_migrate[i], NULL, NULL, NULL);
    }

    const char* sql =
        "SELECT address, protocol, baud, channel, wiring_mode, collector_addr, asset_no, pt_ratio, "
        "ct_ratio, config_seq, baud_code, password, tariff_count, user_type, rated_voltage, "
        "rated_current "
        "FROM meter_inventory "
        /* 顺序必须确定：slot_id 按加载顺序分配，顺序一变，重启后同一只表的
         * 影子槽位就换了。 */
        "ORDER BY address ASC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return -2;
    }

    /* 组合根可能只 memset 了节点、没调用 init：序号从 1 开始，0 保留给“未分配”。 */
    if (inv->next_config_seq == 0) inv->next_config_seq = 1;

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
            m->baud_code = (uint8_t)sqlite3_column_int(stmt, 10);
            m->protocol_code = _proto_to_code(m->protocol);

            /* 配置序号：旧库为 0 的按加载顺序补号，已有号的保留并推进 next。 */
            uint16_t seq = (uint16_t)sqlite3_column_int(stmt, 9);
            if (seq == 0) seq = inv->next_config_seq;
            m->config_seq = seq;
            if (seq >= inv->next_config_seq) inv->next_config_seq = (uint16_t)(seq + 1);

            const char* pwd = (const char*)sqlite3_column_text(stmt, 11);
            if (pwd) {
                strncpy(m->password, pwd, sizeof(m->password) - 1);
            }
            m->tariff_count = (uint8_t)sqlite3_column_int(stmt, 12);
            m->user_type = (uint8_t)sqlite3_column_int(stmt, 13);
            m->rated_voltage = (uint16_t)sqlite3_column_int(stmt, 14);
            m->rated_current = (uint16_t)sqlite3_column_int(stmt, 15);
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

int dcu_inventory_save_meter(sqlite3* db, const production_meter_t* m) {
    if (!db || !m) return -1;
    const char* sql =
        "INSERT INTO meter_inventory(address, protocol, baud, channel, wiring_mode, collector_addr, "
        "asset_no, pt_ratio, ct_ratio, config_seq, baud_code, password, tariff_count, user_type, "
        "rated_voltage, rated_current) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(address) DO UPDATE SET protocol=excluded.protocol, baud=excluded.baud, "
        "channel=excluded.channel, wiring_mode=excluded.wiring_mode, "
        "collector_addr=excluded.collector_addr, asset_no=excluded.asset_no, "
        "pt_ratio=excluded.pt_ratio, ct_ratio=excluded.ct_ratio, "
        "config_seq=excluded.config_seq, baud_code=excluded.baud_code, "
        "password=excluded.password, tariff_count=excluded.tariff_count, "
        "user_type=excluded.user_type, rated_voltage=excluded.rated_voltage, "
        "rated_current=excluded.rated_current";

    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, m->address, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, m->protocol, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, (int)m->baud_rate);
    sqlite3_bind_int(stmt, 4, (int)m->channel_id);
    sqlite3_bind_int(stmt, 5, (int)m->wiring_mode);
    sqlite3_bind_text(stmt, 6, m->collector_addr, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, m->asset_no, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, (int)m->pt_ratio);
    sqlite3_bind_int(stmt, 9, (int)m->ct_ratio);
    sqlite3_bind_int(stmt, 10, (int)m->config_seq);
    sqlite3_bind_int(stmt, 11, (int)m->baud_code);
    sqlite3_bind_text(stmt, 12, m->password, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 13, (int)m->tariff_count);
    sqlite3_bind_int(stmt, 14, (int)m->user_type);
    sqlite3_bind_int(stmt, 15, (int)m->rated_voltage);
    sqlite3_bind_int(stmt, 16, (int)m->rated_current);
    int rc = (sqlite3_step(stmt) == SQLITE_DONE) ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_inventory_delete_meter(sqlite3* db, const char* address) {
    if (!db || !address) return -1;
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM meter_inventory WHERE address=?", -1, &stmt, NULL) !=
        SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, address, -1, SQLITE_TRANSIENT);
    int rc = (sqlite3_step(stmt) == SQLITE_DONE) ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_inventory_clear_db(sqlite3* db) {
    if (!db) return -1;
    return sqlite3_exec(db, "DELETE FROM meter_inventory", NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}
