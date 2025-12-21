针对 **Linux 边缘计算 + C 语言** 环境，我为你设计了一套名为 **"EdgeDB & LogicCore"** 的框架方案。这套方案将 SQLite 的数据管理、版本迁移以及动态规则引擎紧密结合，专为 (H/I)EMS 场景打造。

---

### 第一部分：SQLite 表结构设计 (Schema Design)

为了平衡 **写入性能**（时序数据）和 **查询便利性**（配置数据），我们将数据分为三类：**元数据 (Metadata)**、**时序数据 (Time-Series)** 和 **事件数据 (Events)**。

#### 1. 核心表结构 SQL

```sql
-- 1. 系统配置表 (Key-Value 存储，用于存放版本号、云端地址等)
CREATE TABLE IF NOT EXISTS sys_config (
    key TEXT PRIMARY KEY,
    value TEXT,
    updated_at INTEGER
);

-- 2. 测点元数据表 (Tag Definition)
-- 将字符串的 "meter_voltage" 映射为 int id，节省时序表的存储空间
CREATE TABLE IF NOT EXISTS tags (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    tag_name TEXT UNIQUE NOT NULL, -- e.g., "pcs_01/active_power"
    unit TEXT,                     -- e.g., "kW"
    desc TEXT
);

-- 3. 时序数据表 (最核心、最大的表)
-- 技巧：不使用 AUTOINCREMENT ID，直接用 (ts, tag_id) 做联合主键，极大提升查询速度
CREATE TABLE IF NOT EXISTS ts_data (
    ts INTEGER NOT NULL,      -- Unix 毫秒时间戳
    tag_id INTEGER NOT NULL,  -- 关联 tags.id
    val_num REAL,             -- 浮点值 (大部分情况)
    val_str TEXT,             -- 字符串值 (少部分情况，可为空)
    quality INTEGER DEFAULT 0,-- 质量戳 (0=Good, 1=Bad)
    PRIMARY KEY (ts, tag_id)
) WITHOUT ROWID; -- 优化：减少 B-Tree 层级，节省空间

-- 索引：加速按时间范围查询 (导出报表用)
CREATE INDEX IF NOT EXISTS idx_ts_data_time ON ts_data(ts);

-- 4. 告警/事件表
CREATE TABLE IF NOT EXISTS alarms (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ts INTEGER NOT NULL,
    tag_id INTEGER,
    level INTEGER,       -- 1=Info, 2=Warn, 3=Error
    message TEXT,
    acked INTEGER DEFAULT 0 -- 是否已确认
);
```

#### 2. 存储策略 (针对 Flash 寿命)

*   **WAL 模式:** 必须开启 `PRAGMA journal_mode=WAL;`。
*   **批量写入:** 时序数据不要一条条插。在内存中缓存 1-2 秒的数据，拼成一个 `BEGIN; INSERT ...; INSERT ...; COMMIT;` 事务。
*   **自动清理 (Retention Policy):**
    不要指望手动删数据。在 C 代码启动一个低频定时器（每天一次）：
    ```sql
    -- 删除 30 天前的数据
    DELETE FROM ts_data WHERE ts < (strftime('%s','now') * 1000 - 2592000000);
    -- 执行 VACUUM 释放空间 (可选，耗时操作，建议每月一次)
    ```

---

### 第二部分：数据库迁移框架 (Migration Framework)

边缘设备在现场升级（OTA）时，数据库结构可能会变（比如增加了 `alarms` 表的字段）。你不能手动连上去敲 SQL。你需要一个 **"自动迁移机制"**。

#### 1. 版本控制原理
利用 SQLite 内置的 `PRAGMA user_version` 来存储当前的 Schema 版本号。

*   **App 启动时：** 读取 `user_version`。
*   **对比：** 如果 DB 版本 < 代码定义的最新版本，依次执行迁移脚本。

#### 2. C 语言迁移器实现

```c
#include <sqlite3.h>
#include <stdio.h>

// 定义迁移函数类型
typedef int (*MigrationFunc)(sqlite3 *db);

// ---------------------------------------------------------
// 迁移脚本 V1 -> V2: 给 tags 表增加 'scale' 字段
// ---------------------------------------------------------
int migrate_to_v2(sqlite3 *db) {
    const char *sql = "ALTER TABLE tags ADD COLUMN scale REAL DEFAULT 1.0;";
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        printf("Migrate V2 failed: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

// ---------------------------------------------------------
// 迁移脚本 V2 -> V3: 创建报警表
// ---------------------------------------------------------
int migrate_to_v3(sqlite3 *db) {
    const char *sql = "CREATE TABLE IF NOT EXISTS alarms (id INTEGER PRIMARY KEY...);"; 
    // ... (执行 SQL)
    return 0; // 简写
}

// ---------------------------------------------------------
// 迁移注册表 (数组下标对应版本号)
// ---------------------------------------------------------
#define TARGET_VERSION 3
MigrationFunc migrations[] = {
    NULL,          // v0 -> v1 (Base schema, usually handled by init)
    NULL,          // v1 (Placeholder)
    migrate_to_v2, // v1 -> v2
    migrate_to_v3  // v2 -> v3
};

// ---------------------------------------------------------
// 核心迁移逻辑 (在 DB 初始化时调用)
// ---------------------------------------------------------
int check_and_migrate(sqlite3 *db) {
    sqlite3_stmt *stmt;
    int current_ver = 0;

    // 1. 获取当前版本
    if (sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            current_ver = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    printf("DB Version: %d, Target: %d\n", current_ver, TARGET_VERSION);

    if (current_ver >= TARGET_VERSION) return 0; // 无需迁移

    // 2. 依次执行迁移
    for (int v = current_ver + 1; v <= TARGET_VERSION; v++) {
        printf("Migrating to version %d...\n", v);
        
        // 开启事务
        sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
        
        if (migrations[v](db) != 0) {
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            return -1; // 迁移失败，App 应该退出或报警
        }

        // 更新版本号
        char ver_sql[32];
        snprintf(ver_sql, sizeof(ver_sql), "PRAGMA user_version = %d;", v);
        sqlite3_exec(db, ver_sql, NULL, NULL, NULL);
        
        sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    }
    
    return 0;
}
```

---

### 第三部分：轻量级规则引擎 (Rule Engine)

在 (H/I)EMS 中，硬编码逻辑（如 `if (soc < 20)`）是不行的，因为现场需求在变。我们需要一个**基于配置的逻辑引擎**。

#### 1. 规则配置模型 (JSON)

我们设计一个简单的 **"Trigger - Condition - Action"** 模型。

```json
{
  "rules": [
    {
      "id": 1,
      "name": "Low Battery Protection",
      "enabled": true,
      "trigger": "interval_1s",  // 或 "on_change:bms_soc"
      "condition": "bms_soc < 20.0 && grid_status == 0", // 表达式
      "actions": [
        { "type": "write_tag", "target": "pcs_switch", "value": 0 },
        { "type": "alarm", "level": 2, "msg": "Battery Low, PCS Stop" }
      ]
    }
  ]
}
```

#### 2. 核心选型：表达式求值

不要自己写 Parser。使用 C 语言神级库 **TinyExpr** (`tinyexpr.c`).
*   **特点：** 单文件，无依赖，极快，安全（不会执行系统命令）。
*   **能力：** 支持 `+ - * / % ^`, `sqrt`, `sin`, 逻辑运算 `&& || > < ==`。

#### 3. C 语言引擎实现框架

这个引擎的核心是将 **数据点 (DataPoint)** 绑定到 **TinyExpr 的变量** 上。

```c
#include "tinyexpr.h"
#include "cJSON.h"

// 规则结构体
typedef struct {
    char name[32];
    te_expr *expr; // 编译后的表达式对象 (AST)
    // 动作列表 (简略)
    // Action actions[MAX_ACTIONS]; 
} Rule;

// 全局上下文数据 (从 Redis/SharedMem/DataEngine 同步而来)
double g_bms_soc = 0.0;
double g_grid_status = 1.0;

// 1. 初始化引擎
void rule_engine_init(Rule *rule, const char *condition_str) {
    // 绑定变量名到 C 变量地址
    te_variable vars[] = {
        {"bms_soc", &g_bms_soc},
        {"grid_status", &g_grid_status}
    };

    int err;
    // 编译表达式 (只做一次，效率高)
    rule->expr = te_compile(condition_str, vars, 2, &err);
    
    if (!rule->expr) {
        printf("Rule Parse Error at %d\n", err);
    }
}

// 2. 周期性执行 (在 Task Scheduler 中调用)
void rule_engine_tick(Rule *rule) {
    if (!rule->expr) return;

    // 执行求值 (非常快，微秒级)
    double result = te_eval(rule->expr);

    // 结果非 0 即为真
    if (result != 0.0) {
        printf("Rule [%s] Triggered!\n", rule->name);
        execute_actions(rule);
    }
}

// 3. 执行动作 (简易版)
void execute_actions(Rule *rule) {
    // 根据 JSON 配置调用 SDK 接口
    // sdk_write_tag("pcs_switch", 0);
    // sdk_push_alarm("Battery Low...");
}
```

#### 4. 高级功能：防抖 (Debounce)

在 EMS 中，信号可能会抖动（例如 SOC 在 19.9 和 20.0 之间跳变）。规则引擎必须包含防抖逻辑。

*   **设计：** 在 Rule 结构体中增加 `trigger_timestamp`。
*   **逻辑：**
    ```c
    if (result != 0.0) {
        if (now - rule->first_trigger_time > rule->debounce_duration) {
            // 只有当条件持续满足 N 秒后，才执行动作
            execute_actions(rule);
        }
    } else {
        rule->first_trigger_time = now; // 重置
    }
    ```

---

### 总结

这套方案构成了你的 EMS 应用层的**三大支柱**：

1.  **EdgeDB (SQLite):**
    *   **设计:** `ts_data` 表 + `WITHOUT ROWID` 优化时序写入。
    *   **维护:** WAL 模式 + 自动清理策略。
2.  **Auto-Migrator:**
    *   **机制:** `user_version` 版本号 + 迁移函数数组。
    *   **价值:** 保证 OTA 升级不炸库。
3.  **LogicCore (TinyExpr):**
    *   **机制:** JSON 配置 -> 变量绑定 -> AST 求值。
    *   **价值:** 让现场工程师能修改控制逻辑，而不需要你重新编译 C 代码。

将这三者封装成 `.c/.h` 模块，你的 (H/I)EMS 开发效率将大幅提升。