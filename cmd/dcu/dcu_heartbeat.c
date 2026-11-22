#include "dcu_heartbeat.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dcu_698_object_internal.h"
#include "dcu_inventory.h"
#include "dcu_storage.h"
#include "edge_log.h"

/* 生产路径只保留真实职责：刷盘 + 心跳日志。
 * 冻结管线、随机跳闸、周期自退、遥控测试都是台架演示行为，
 * 只有 DCU_DEMO_MODE=1 时才启用。 */
static bool _env_enabled(const char *name) {
    const char *v = getenv(name);
    return v && v[0] && strcmp(v, "0") != 0;
}

static void _self_terminate(dcu_heartbeat_t *hb) {
    hb->stop_requested++;
    if (hb->node) {
        dcu_storage_flush(dcu_node_get_storage(hb->node));
    }
    if (hb->svc) service_stop(hb->svc);
}

void dcu_heartbeat_tick(dcu_heartbeat_t *hb) {
    if (!hb) return;

    dcu_storage_t *storage = hb->node ? dcu_node_get_storage(hb->node) : NULL;
    dcu_inventory_t *inv = hb->node ? dcu_node_get_inventory(hb->node) : NULL;
    int cycle = hb->test_cycles; /* 0 基，保留演示模式原有的奇偶跳闸语义 */

    /* 落库已在 dcu_node_record_reading()（驱动回调直达聚合根）里完成，
     * 这里只做轮次收尾：刷盘。生产模式每周期都刷盘，演示模式额外做冻结管线。 */
    if (storage) dcu_storage_flush(storage);
    uint32_t saved = hb->node ? dcu_node_take_saved(hb->node) : 0;
    hb->test_cycles++;

    LOG_INFO(LOG_CAT_SCHED, "💓 [HEARTBEAT] Cycle %d completed (saved=%u)", cycle + 1, saved);

    if (hb->demo_mode && saved > 0 && inv && storage) {
        printf("💓 [HEARTBEAT] Marathon Cycle %d Completed. Database Flushed.\n", cycle + 1);

        // 💡 [Industrial Fix] 开启大事务保护 5xxx 冻结管线的高频批量写入，防止 SQLite 疯狂分配/释放
        // heap 导致 C 库崩溃
        dcu_storage_begin_trans(storage);

        for (int i = 0; i < inv->count; i++) {
            pipe_msg_t trigger;
            memset(&trigger, 0, sizeof(pipe_msg_t));
            trigger.tag_id = OAD_DAILY_FREEZE;
            trigger.type = PIPEMSG_TYPE_SCALAR;
            trigger.device_id = (uint32_t)i;
            trigger.ts = (uint64_t)time(NULL);
            if (hb->se) stream_engine_push(hb->se, &trigger);
            hb->demo_freezes++;

            if (i > 0 && i % 200 == 0) {
                uint8_t action = (cycle % 2 == 0) ? 1 : 0;  // 0/2 cycle Trip, 1 cycle Restore
                printf("🎛️ [RELAY] Dev %d: Triggering Action %d\n", i, action);
                hb->demo_relays++;
                dcu_698_control_legacy_action((uint32_t)i, action);
            }
        }

        dcu_storage_commit_trans(storage);
    }

    /* 自退只在演示模式启用；生产模式永不因周期数退出。
     * `DCU_HEARTBEAT_CYCLES=0` 表示一直跑，由平台收尾。 */
    if (hb->demo_mode && hb->max_cycles > 0 && hb->test_cycles >= hb->max_cycles) {
        LOG_INFO(LOG_CAT_SCHED, "🏁 [TEST] Marathon Completed. Self-terminating...");
        _self_terminate(hb);
    }
}

void dcu_heartbeat_control_tick(dcu_heartbeat_t *hb) {
    if (!hb || !hb->demo_mode) return;
    hb->control_triggers++;
    LOG_INFO(LOG_CAT_BUS, "🏁 [TEST] Triggering demo relay control for Meter 10000001...");
    dcu_698_control_legacy_action(0, 1);
}

static void _freeze_trigger_cb(uv_timer_t *handle) {
    dcu_heartbeat_tick((dcu_heartbeat_t *)handle->data);
}

static void _control_test_cb(uv_timer_t *handle) {
    dcu_heartbeat_control_tick((dcu_heartbeat_t *)handle->data);
}

void dcu_heartbeat_start(dcu_heartbeat_t *hb, uv_loop_t *loop, dcu_node_t *node,
                         stream_engine_t *se, service_t *svc) {
    if (!hb || !loop) return;
    hb->loop = loop;
    hb->node = node;
    hb->se = se;
    hb->svc = svc;
    hb->test_cycles = 0;
    hb->demo_freezes = 0;
    hb->demo_relays = 0;
    hb->control_triggers = 0;
    hb->stop_requested = 0;
    hb->demo_mode = _env_enabled("DCU_DEMO_MODE");
    hb->max_cycles = 0;

    if (hb->demo_mode) {
        /* 演示模式默认跑 3 个心跳周期就自杀（Marathon）。验收要读北向 / 等主站连上来，
         * 不能跟一个 3 秒窗口赛跑 —— 那样的用例会"有时绿有时红"，且红的时候像产品坏了。
         * `DCU_HEARTBEAT_CYCLES=0` 表示一直跑，由平台收尾。 */
        hb->max_cycles = 3;
        const char *env_cycles = getenv("DCU_HEARTBEAT_CYCLES");
        if (env_cycles && env_cycles[0]) hb->max_cycles = atoi(env_cycles);
    }

    uv_timer_init(loop, &hb->freeze_timer);
    hb->freeze_timer.data = hb;
    uv_timer_start(&hb->freeze_timer, _freeze_trigger_cb, 5000, 5000);

    uv_timer_init(loop, &hb->control_timer);
    hb->control_timer.data = hb;
    if (hb->demo_mode) {
        /* 演示用的遥控触发。缺省 2 秒（第一轮抄表之后、3 个心跳周期之内），
         * 让"抄表 → 遥控"这条链在一次运行里就能被验到；
         * `DCU_CONTROL_DELAY_MS` 可覆盖（0 = 立刻触发，验收里用它把窗口压掉）。 */
        uint64_t ctrl_delay_ms = 2000;
        const char *env_ctrl = getenv("DCU_CONTROL_DELAY_MS");
        if (env_ctrl && env_ctrl[0]) {
            unsigned long v = strtoul(env_ctrl, NULL, 10);
            ctrl_delay_ms = (uint64_t)v;
        }
        uv_timer_start(&hb->control_timer, _control_test_cb, ctrl_delay_ms, 0);
    }
}
