/**
 * @file dcu_698_object_model.c
 * @brief DCU 698 静态对象表与类处理器（D1-03 从 dcu_router.c 迁移）
 *
 * 表是文件作用域静态存储：编译期给 oi/class/scope/mask/处理器，装配期只填
 * instance。查表无锁、无动态内存。
 */

#include "dcu_698_object_model.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "datahub.h"
#include "edge_log.h"

/* 256 位位图的静态初始化：按字节下标与位号填，避免手算十六进制。 */
#define BIT(id) ((uint8_t)(1U << ((id)&7)))
#define BYTE(id) ((id) >> 3)

/* 先声明后定义，便于表内引用。 */
static int _oi_6000_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val);
static int _oi_6000_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result);
static int _oi_6012_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val);
static int _oi_6012_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result);
static int _oi_variable_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                             void *out_val);
static int _oi_5004_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val);
static int _oi_5004_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result);
static int _oi_8000_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result);
static int _oi_f100_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result);
static int _oi_f200_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val);
static int _oi_f200_write(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          const void *in_val);
static int _oi_6002_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val);
static int _oi_6002_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result);
static int _oi_6003_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val);
static int _oi_event_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val);
static int _oi_event_write(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                           const void *in_val);
static int _oi_event_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result);

/* ==============================================================================
 * 静态对象表（12 个 OI）。instance 在 init 时填入，其余字段初始化后只读。
 * ============================================================================== */
static dcu_698_object_t g_objects[] = {
    /* 1. 0x6000 采集档案配置表 (Class 10) */
    {.oi = OI_METER_ARCHIVE,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SHARED,
     .read = _oi_6000_read,
     .method = _oi_6000_execute,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method_mask = {[BYTE(1)] = BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),
                     [BYTE(127)] = BIT(127),
                     [BYTE(128)] = BIT(128)}},

    /* 2. 0x6012 任务配置表 (Class 10) */
    {.oi = OI_TASK_SCHEDULER,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SHARED,
     .read = _oi_6012_read,
     .method = _oi_6012_execute,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method_mask = {[BYTE(1)] = BIT(1)}},

    /* 3. 0x2000 电压 / 0x2001 电流 (Class 3)，每块表一个实例 */
    {.oi = (uint16_t)((OAD_VOLTAGE >> 16) & 0xFFFF),
     .class_id = IC_VARIABLE,
     .scope = DCU_OBJ_PER_METER,
     .read = _oi_variable_read,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = (uint16_t)((OAD_CURRENT >> 16) & 0xFFFF),
     .class_id = IC_VARIABLE,
     .scope = DCU_OBJ_PER_METER,
     .read = _oi_variable_read,
     .attr_mask = {[BYTE(2)] = BIT(2)}},

    /* 4. 0x5004 日冻结记录表 (Class 9)：方法 3/4/5/7/8 已登记，实现随 D1-15。 */
    {.oi = (uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF),
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .read = _oi_5004_read,
     .method = _oi_5004_execute,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},

    /* 5. 0x8000 远程负荷拉闸动作 (Class 13) */
    {.oi = OI_REMOTE_CONTROL,
     .class_id = IC_CONTROL,
     .scope = DCU_OBJ_SINGLETON,
     .method = _oi_8000_execute,
     .method_mask = {[BYTE(1)] = BIT(1)}},

    /* 6. 0xF100 ESAM 安全 (Class 10) */
    {.oi = OI_ESAM_SECURITY,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SINGLETON,
     .method = _oi_f100_execute,
     .method_mask = {[BYTE(1)] = BIT(1)}},

    /* 7. 0xF200 外设串口参数 (Class 10) */
    {.oi = OI_CHANNEL_CONFIG,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SINGLETON,
     .read = _oi_f200_read,
     .write = _oi_f200_write,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},

    /* 8. 0x6002 搜表控制 / 0x6003 搜表结果 (Class 10) */
    {.oi = OI_METER_DISCOVERY,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SINGLETON,
     .read = _oi_6002_read,
     .method = _oi_6002_execute,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)},
     .method_mask = {[BYTE(127)] = BIT(127)}},
    {.oi = OI_DISCOVERY_RESULT,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SINGLETON,
     .read = _oi_6003_read,
     .attr_mask = {[BYTE(2)] = BIT(2)}},

    /* 9. 0x3000 失压 / 0x3005 过流事件 (Class 7) */
    {.oi = OI_VOLTAGE_DROP,
     .class_id = IC_EVENT,
     .scope = DCU_OBJ_SHARED,
     .read = _oi_event_read,
     .write = _oi_event_write,
     .method = _oi_event_execute,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),
                   [BYTE(8)] = BIT(8) | BIT(9) | BIT(10)},
     .method_mask = {[BYTE(1)] = BIT(1)}},
    {.oi = OI_OVER_CURRENT,
     .class_id = IC_EVENT,
     .scope = DCU_OBJ_SHARED,
     .read = _oi_event_read,
     .write = _oi_event_write,
     .method = _oi_event_execute,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6),
                   [BYTE(8)] = BIT(8) | BIT(9) | BIT(10)},
     .method_mask = {[BYTE(1)] = BIT(1)}},
};

#define OBJECT_COUNT (sizeof(g_objects) / sizeof(g_objects[0]))

static bool g_initialized = false;
static router_bus_ops_t g_router_bus_ops;
static router_sink_ops_t g_router_sink_ops;
static dcu_inventory_t *s_router_inv = NULL;
static dcu_control_sink_t s_control_sink;
static void *s_control_ctx;

/* ==============================================================================
 * 1. OI 0x6000 采集档案配置表 (Class 10)
 * ============================================================================== */
static int _oi_6000_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val) {
    (void)elem_idx;
    dcu_inventory_t *inv = (dcu_inventory_t *)obj->instance;
    if (!inv || !out_val) return -1;

    // 属性 2: 档案中的在线电能表总数
    if (attr_id == 2) {
        // 档案只在 uv 循环线程上被读写，这里不需要锁（见 dcu_inventory.h 的线程所有权说明）
        *(uint16_t *)out_val = inv->count;
        return 0;
    }
    return -2;  // 属性未定义
}

static int _oi_6000_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result) {
    dcu_inventory_t *inv = (dcu_inventory_t *)obj->instance;
    if (!inv) return -1;

    // [PRD Alignment] 方法 1: 添加 (Add)
    if (method_id == 1) {
        const char *addr = (const char *)param;
        int ret = dcu_inventory_add(inv, addr, "DLT698", 9600, 1);
        if (result) *(int *)result = ret;
        return ret;
    }
    // [PRD Alignment] 方法 2: 批量添加 (AddBatch) 尚未实现
    if (method_id == 2) {
        return -4;  // 方法存在但拒绝执行：不得返回 0 让主站误以为已写入
    }
    // [PRD Alignment] 方法 3: 更新基本/扩展信息 (Update) 尚未实现
    if (method_id == 3) {
        return -4;  // 方法存在但拒绝执行：不得返回 0 让主站误以为已更新
    }
    // [PRD Alignment] 方法 4: 按序号或地址删除 (Delete)
    if (method_id == 4) {
        const char *addr = (const char *)param;
        int ret = dcu_inventory_remove(inv, addr);
        if (result) *(int *)result = ret;
        return 0;
    }
    // [PRD Alignment] 方法 5: 清空档案表 (Clear)
    if (method_id == 5) {
        memset(inv->meters, 0, sizeof(inv->meters));
        inv->count = 0;
        inv->revision++;  // 清档也是一次变更：poller 靠它退休表级状态
        if (result) *(int *)result = 0;
        return 0;
    }

    // [Legacy/Testing mapping] 保留原来的 127/128 映射以免中断外部测试用例
    if (method_id == 127) {
        int ret = dcu_inventory_add(inv, (const char *)param, "DLT698", 9600, 1);
        if (result) *(int *)result = ret;
        return ret;
    }
    if (method_id == 128) {
        int ret = dcu_inventory_remove(inv, (const char *)param);
        if (result) *(int *)result = ret;
        return ret;
    }

    return -2;  // 方法未定义
}

/* ==============================================================================
 * 2. OI 0x6012 任务配置表 (Class 10)
 * ============================================================================== */
static int _oi_6012_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val) {
    (void)elem_idx;
    dcu_poller_t *poller = (dcu_poller_t *)obj->instance;
    if (!poller || !out_val) return -1;

    // 属性 2: 方案配置库数量
    if (attr_id == 2) {
        uv_mutex_lock(&poller->lock);
        *(uint16_t *)out_val = poller->scheme_count;
        uv_mutex_unlock(&poller->lock);
        return 0;
    }
    return -2;
}

static int _oi_6012_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result) {
    dcu_poller_t *poller = (dcu_poller_t *)obj->instance;
    if (!poller) return -1;

    // 方法 1: Add Task
    if (method_id == 1) {
        const dcu_scheme_t *s = (const dcu_scheme_t *)param;
        if (s->type != SCHEME_TYPE_NORMAL)
            return -1;  // Only support routing NORMAL schemes through this legacy path
        int ret = dcu_poller_add_scheme(poller, s->scheme_id, s->data.normal.capture_mode,
                                        s->data.normal.csd_list, s->data.normal.csd_count);
        if (result) *(int *)result = ret;
        return ret;
    }
    return -2;
}

/* ==============================================================================
 * 3. OI 0x2000 电压变量 / OI 0x2001 电流变量 (Class 3)
 * ============================================================================== */
static int _oi_variable_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                             void *out_val) {
    (void)elem_idx;
    if (!out_val) return -1;

    // 属性 2: 实时参量物理数值反馈 (Strict Physical Truth from DataHub)
    if (attr_id == 2) {
        float val = 0.0f;
        uint32_t target_oad = OAD_VOLTAGE;
        if (obj->oi == ((OAD_CURRENT >> 16) & 0xFFFF)) {
            target_oad = OAD_CURRENT;
        }

        if (g_router_bus_ops.read_shadow) {
            pipe_msg_t msg;
            memset(&msg, 0, sizeof(pipe_msg_t));
            if (g_router_bus_ops.read_shadow(g_router_bus_ops.ctx, target_oad, 0, &msg) == 0) {
                val = (float)msg.payload.val.f;
            }
        }
        *(float *)out_val = val;
        return 0;
    }
    return -2;
}

/* ==============================================================================
 * 4. OI 0x5004 日冻结记录表 (Class 9)
 * ============================================================================== */
static int _oi_5004_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val) {
    (void)obj;
    (void)elem_idx;
    if (!out_val) return -1;

    // 属性 2: 获取历史日冻结数据记录总数
    if (attr_id == 2) {
        if (g_router_sink_ops.count_freezes) {
            return g_router_sink_ops.count_freezes(g_router_sink_ops.ctx, (uint16_t *)out_val);
        } else {
            *(uint16_t *)out_val = 0;
            return 0;
        }
    }
    return -2;
}

/* Class 9 方法 3/4/5/7/8 已登记进 mask，但实现随 D1-15 的 GetRecord/RSD 落地。
 * 现在明确回 -4（存在但拒绝执行），不得返回 0 让主站误以为冻结已执行。 */
static int _oi_5004_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result) {
    (void)obj;
    (void)param;
    (void)result;
    if (method_id == 3 || method_id == 4 || method_id == 5 || method_id == 7 || method_id == 8) {
        return -4;
    }
    return -2;
}

/* ==============================================================================
 * 5. OI 0x8000 远程控制拉闸动作 (Class 13)
 * ============================================================================== */
static int _oi_8000_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result) {
    (void)obj;
    if (method_id != 1) return -2;

    uint32_t slot = param ? (uint32_t)(uintptr_t)param : 0;
    uint8_t action = result ? *(uint8_t *)result : 1; /* 1=跳闸 0=合闸允许 */

    const production_meter_t *m = dcu_inventory_by_slot(s_router_inv, slot);
    if (!m) {
        LOG_WARN(LOG_CAT_CORE, "OI 8000: slot %u 不在档案里，拒绝执行", slot);
        return -3;
    }

    /* 领域动作必须过聚合根：在线状态、遥控白名单、五防闭锁都在那边。
     * 路由直连总线会把"南向控制严禁穿透"破掉（架构原则 3）。 */
    if (!s_control_sink) {
        LOG_ERROR(LOG_CAT_CORE, "OI 8000: 控制下沉端口未注入，拒绝执行 dev=%s", m->address);
        return -4;
    }

    int rc = s_control_sink(s_control_ctx, m->address, (uint16_t)OI_REMOTE_CONTROL, action != 0);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "OI 8000: 聚合根拒绝/下发失败 dev=%s action=%u rc=%d", m->address,
                 action, rc);
    }
    return rc;
}

/* ==============================================================================
 * 6. OI 0xF100 ESAM 安全模块 (Class 10)
 * ============================================================================== */
static int _oi_f100_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result) {
    dcu_esam_t *esam = (dcu_esam_t *)obj->instance;
    if (!esam || !param || !result) return -1;

    // 方法 1: 身份鉴权 (Authenticate)
    if (method_id == 1) {
        return dcu_security_authenticate(esam, (const uint8_t *)param, (uint8_t *)result);
    }
    return -2;
}

/* ==============================================================================
 * 7. OI 0xF200 外设接口参数配置 (Class 10)
 * ============================================================================== */
static dcu_channel_cfg_t g_f200_channel_cfg = {.baudrate = 9600, .parity = 0};

static int _oi_f200_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val) {
    (void)obj;
    (void)elem_idx;
    if (!out_val) return -1;

    // 属性 2: Baudrate
    if (attr_id == 2) {
        *(uint32_t *)out_val = g_f200_channel_cfg.baudrate;
        return 0;
    }
    // 属性 3: Parity
    if (attr_id == 3) {
        *(uint8_t *)out_val = g_f200_channel_cfg.parity;
        return 0;
    }
    return -2;
}

static int _oi_f200_write(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          const void *in_val) {
    (void)obj;
    (void)elem_idx;
    if (!in_val) return -1;

    // 属性 2: Baudrate
    if (attr_id == 2) {
        g_f200_channel_cfg.baudrate = *(const uint32_t *)in_val;
        printf("🔧 [CHANNEL] 动态配置外设串口波特率调整为: %u\n", g_f200_channel_cfg.baudrate);
        return 0;
    }
    // 属性 3: Parity
    if (attr_id == 3) {
        g_f200_channel_cfg.parity = *(const uint8_t *)in_val;
        return 0;
    }
    return -2;
}

/* ==============================================================================
 * 8. OI 0x6002 搜表控制 & 0x6003 搜表结果 (Class 10)
 * ============================================================================== */
static int _oi_6002_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val) {
    (void)elem_idx;
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc || !out_val) return -1;

    // 属性 2: 搜表状态
    if (attr_id == 2) {
        uv_mutex_lock(&disc->lock);
        *(uint8_t *)out_val = disc->status;
        uv_mutex_unlock(&disc->lock);
        return 0;
    }
    // 属性 3: 发现数量
    if (attr_id == 3) {
        uv_mutex_lock(&disc->lock);
        *(uint16_t *)out_val = disc->count;
        uv_mutex_unlock(&disc->lock);
        return 0;
    }
    return -2;
}

static int _oi_6002_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result) {
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc) return -1;

    // 方法 127: 启动自动搜表
    if (method_id == 127) {
        uint32_t duration = param ? *(const uint32_t *)param : 15;
        int ret = dcu_discovery_start(disc, duration);
        if (result) *(int *)result = ret;
        return 0;
    }
    return -2;
}

static int _oi_6003_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                         void *out_val) {
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc || !out_val) return -1;

    // 属性 2: 发现的电表列表
    if (attr_id == 2) {
        uv_mutex_lock(&disc->lock);
        if (elem_idx < disc->count) {
            strncpy((char *)out_val, disc->discovered_tsas[elem_idx], DISCOVERY_ADDR_LEN - 1);
            ((char *)out_val)[DISCOVERY_ADDR_LEN - 1] = '\0';
            uv_mutex_unlock(&disc->lock);
            return 0;
        }
        uv_mutex_unlock(&disc->lock);
        return -3;
    }
    return -2;
}

/* ==============================================================================
 * 9. Class 7 失压 & 过流事件对象
 * ============================================================================== */
static int _oi_event_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val) {
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt || !out_val) return -1;

    // 属性 2: 事件记录表 (按发生次序倒序读取)
    if (attr_id == 2) {
        if (elem_idx < evt->record_count) {
            *(dcu_event_record_t *)out_val = evt->records[elem_idx];
            return 0;
        }
        return -3;
    }
    // 属性 3: 关联对象属性表
    if (attr_id == 3) {
        if (elem_idx < evt->snapshot_count) {
            *(uint32_t *)out_val = evt->snapshot_oads[elem_idx];
        } else {
            *(uint32_t *)out_val = evt->snapshot_oads[0];
        }
        return 0;
    }
    // 属性 4: 当前记录数
    if (attr_id == 4) {
        *(uint16_t *)out_val = evt->record_count;
        return 0;
    }
    // 属性 5: 最大记录数
    if (attr_id == 5) {
        *(uint16_t *)out_val = MAX_EVENT_RECORDS;
        return 0;
    }
    // 属性 6: 配置参数 (门限阈值)
    if (attr_id == 6) {
        *(float *)out_val = evt->threshold;
        return 0;
    }
    // 属性 8: 上报标识
    if (attr_id == 8) {
        *(uint8_t *)out_val = evt->report_flag;
        return 0;
    }
    // 属性 9: 有效标识 (使能)
    if (attr_id == 9) {
        *(bool *)out_val = evt->is_enabled;
        return 0;
    }
    // 属性 10: 最近一次发生/恢复时标
    if (attr_id == 10) {
        uint64_t *times = (uint64_t *)out_val;
        times[0] = evt->last_start_time;
        times[1] = evt->last_end_time;
        return 0;
    }
    return -2;
}

static int _oi_event_write(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                           const void *in_val) {
    (void)elem_idx;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt || !in_val) return -1;

    // 属性 6: 在线配置判定门限
    if (attr_id == 6) {
        evt->threshold = *(const float *)in_val;
        printf("🔧 [EVENT] 动态配置事件 %04X 触发判定门限阈值调整为: %.2f\n", evt->oi,
               evt->threshold);
        return 0;
    }
    // 属性 8: 上报标识修改
    if (attr_id == 8) {
        evt->report_flag = *(const uint8_t *)in_val;
        return 0;
    }
    // 属性 9: 使能开关修改
    if (attr_id == 9) {
        evt->is_enabled = *(const bool *)in_val;
        return 0;
    }
    return -2;
}

static int _oi_event_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    (void)param;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt) return -1;

    // 方法 1: 复位清空历史记录表 (Reset)
    if (method_id == 1) {
        evt->record_count = 0;
        memset(evt->records, 0, sizeof(evt->records));
        evt->last_start_time = 0;
        evt->last_end_time = 0;
        if (result) *(int *)result = 0;
        printf("🧹 [EVENT] ACTION: 成功复位事件 %04X 的历史事件记录表。\n",
               evt->oi);
        return 0;
    }
    return -2;
}

/* ==============================================================================
 * 查表与分发
 * ============================================================================== */
static dcu_698_object_t *_find_mut(uint16_t oi) {
    for (size_t i = 0; i < OBJECT_COUNT; i++) {
        if (g_objects[i].oi == oi) return &g_objects[i];
    }
    return NULL;
}

static void _set_instance(uint16_t oi, const void *instance) {
    dcu_698_object_t *obj = _find_mut(oi);
    if (obj) obj->instance = instance;
}

int dcu_698_object_model_init(dcu_inventory_t *inv, dcu_poller_t *poller,
                              router_sink_ops_t sink_ops, dcu_esam_t *esam,
                              dcu_discovery_t *discovery, dcu_event_obj_t *evt_3000,
                              dcu_event_obj_t *evt_3005, router_bus_ops_t bus_ops) {
    g_router_sink_ops = sink_ops;
    g_router_bus_ops = bus_ops;
    s_router_inv = inv;

    for (size_t i = 0; i < OBJECT_COUNT; i++) g_objects[i].instance = NULL;
    g_initialized = true;

    _set_instance(OI_METER_ARCHIVE, inv);
    _set_instance(OI_TASK_SCHEDULER, poller);
    _set_instance(OI_ESAM_SECURITY, esam);
    _set_instance(OI_METER_DISCOVERY, discovery);
    _set_instance(OI_DISCOVERY_RESULT, discovery);
    _set_instance(OI_VOLTAGE_DROP, evt_3000);
    _set_instance(OI_OVER_CURRENT, evt_3005);
    return 0;
}

void dcu_698_object_model_reset(void) {
    for (size_t i = 0; i < OBJECT_COUNT; i++) g_objects[i].instance = NULL;
    g_initialized = false;
    g_router_bus_ops = (router_bus_ops_t){0};
    g_router_sink_ops = (router_sink_ops_t){0};
    s_router_inv = NULL;
}

void dcu_698_object_model_set_control_sink(dcu_control_sink_t fn, void *ctx) {
    s_control_sink = fn;
    s_control_ctx = ctx;
}

const dcu_698_object_t *dcu_698_find(uint16_t oi) {
    if (!g_initialized) return NULL;
    return _find_mut(oi);
}

int dcu_698_read(uint32_t oad, void *out_val) {
    uint16_t oi = (uint16_t)((oad >> 16) & 0xFFFF);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj || !obj->read) return -1;
    if (!dcu_mask_test(obj->attr_mask, attr_id)) return -1;
    return obj->read(obj, attr_id, elem_idx, out_val);
}

int dcu_698_write(uint32_t oad, const void *in_val) {
    uint16_t oi = (uint16_t)((oad >> 16) & 0xFFFF);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj || !obj->write) return -1;
    if (!dcu_mask_test(obj->attr_mask, attr_id)) return -1;
    return obj->write(obj, attr_id, elem_idx, in_val);
}

int dcu_698_call(uint32_t omd, const void *param, void *result) {
    uint16_t oi = (uint16_t)((omd >> 16) & 0xFFFF);
    uint8_t method_id = (uint8_t)((omd >> 8) & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj || !obj->method) return -1;
    if (!dcu_mask_test(obj->method_mask, method_id)) return -1;
    return obj->method(obj, method_id, param, result);
}

int dcu_698_snapshot(uint16_t oi, uint32_t device_id, void *out_msgs, int max_count) {
    uint8_t *p_out = (uint8_t *)out_msgs;
    if (!g_router_bus_ops.read_shadow || !p_out || max_count <= 0) return 0;

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj) return 0;

    int captured = 0;

    // 💡 处理 Class 7 事件类抓拍
    if (obj->class_id == IC_EVENT) {
        dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
        if (!evt) return 0;
        for (int i = 0; i < evt->snapshot_count && captured < max_count; i++) {
            uint32_t oad = evt->snapshot_oads[i];
            void *dest = p_out + (size_t)captured * g_router_bus_ops.msg_size;
            if (g_router_bus_ops.read_shadow(g_router_bus_ops.ctx, oad, device_id, dest) == 0) {
                captured++;
            }
        }
    }
    // 💡 处理 Class 9 冻结类抓拍 (待后续扩展 Attr 3 结构)
    else if (obj->class_id == IC_RECORD) {
        // 目前先返回空，后续对接 Attr 3 结构体数组
    }

    return captured;
}
