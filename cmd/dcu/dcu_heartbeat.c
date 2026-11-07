#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dcu_heartbeat.h"
#include "dcu_inventory.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "edge_log.h"

static void _self_terminate(dcu_heartbeat_t *hb) {
    if (hb->node) {
        dcu_storage_flush(dcu_node_get_storage(hb->node));
    }
    if (hb->svc) service_stop(hb->svc);
}

static void _freeze_trigger_cb(uv_timer_t *handle) {
    dcu_heartbeat_t *hb = (dcu_heartbeat_t *)handle->data;
    dcu_inventory_t *inv = dcu_node_get_inventory(hb->node);
    dcu_storage_t *storage = dcu_node_get_storage(hb->node);
    if (!inv || !storage) return;

    /* 落库已在 dcu_node_record_reading()（驱动回调直达聚合根）里完成，
     * 这里只做轮次收尾：刷盘 + 冻结管线。不再反查 shadow。 */
    int saved = (int)dcu_node_take_saved(hb->node);

    if (saved > 0) {
        dcu_storage_flush(storage);
        LOG_INFO(LOG_CAT_SCHED, "✅ [STORAGE] SUCCESS! Captured %d meters truth data.", saved);
        printf("💓 [HEARTBEAT] Marathon Cycle %d Completed. Database Flushed.\n",
               hb->test_cycles + 1);

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

            // 💡 [8xxx Protocol Verification] 随机触发跳闸控制，验证控制闭环
            if (i > 0 && i % 200 == 0) {
                uint32_t relay_omd = (0x8000U << 16) | 0x0100;
                uint8_t action =
                    (hb->test_cycles % 2 == 0) ? 1 : 0;  // 0/2 cycle Trip, 1 cycle Restore
                printf("🎛️ [RELAY] Dev %d: Triggering Action %d (OI 8000 Attr 1)\n", i,
                       action);
                dcu_router_action(relay_omd, (void *)(uintptr_t)i, &action);
            }
        }

        dcu_storage_commit_trans(storage);
    }

    hb->test_cycles++;

    /* 演示模式默认跑 3 个心跳周期就自杀（Marathon）。验收要读北向 / 等主站连上来，
     * 不能跟一个 3 秒窗口赛跑 —— 那样的用例会"有时绿有时红"，且红的时候像产品坏了。
     * `DCU_HEARTBEAT_CYCLES=0` 表示一直跑，由平台收尾。 */
    static int max_cycles = -1;
    if (max_cycles < 0) {
        const char *env_cycles = getenv("DCU_HEARTBEAT_CYCLES");
        max_cycles = (env_cycles && env_cycles[0]) ? atoi(env_cycles) : 3;
    }
    if (max_cycles > 0 && hb->test_cycles >= max_cycles) {
        LOG_INFO(LOG_CAT_SCHED, "🏁 [TEST] Marathon Completed. Self-terminating...");
        _self_terminate(hb);
    }
}

static void _control_test_cb(uv_timer_t *handle) {
    (void)handle;
    LOG_INFO(LOG_CAT_BUS, "🏁 [TEST] Triggering Remote Control ACTION (8000) for Meter 10000001...");
    uint32_t omd = (0x8000U << 16) | 0x0100;
    dcu_router_action(omd, NULL, NULL);
}

void dcu_heartbeat_start(dcu_heartbeat_t *hb, uv_loop_t *loop, dcu_node_t *node,
                         stream_engine_t *se, service_t *svc) {
    if (!hb || !loop) return;
    hb->loop = loop;
    hb->node = node;
    hb->se = se;
    hb->svc = svc;
    hb->test_cycles = 0;

    uv_timer_init(loop, &hb->freeze_timer);
    hb->freeze_timer.data = hb;
    uv_timer_start(&hb->freeze_timer, _freeze_trigger_cb, 5000, 5000);

    uv_timer_init(loop, &hb->control_timer);
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
