#include <stdio.h>

#include "sqlite_service.h"
#include "stream_engine.h"

typedef struct {
    double energy_acc;
    int sample_count;
} minute_acc_state_t;

// Stage 1: 累加器 (同步运行于 Stream Loop)
stage_ret_t stage_accumulate_power(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA) return STAGE_CONTINUE;

    minute_acc_state_t *s = (minute_acc_state_t *)stage->state;
    // 假设数据通过 PipeMsg 传递，修正访问路径
    s->energy_acc += ev->payload.val;
    s->sample_count++;

    return STAGE_CONTINUE;
}

// Stage 2: 冻结触发器 (处理时间事件)
stage_ret_t stage_minute_freeze_trigger(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_TIME_TICK || ev->tick_kind != 1) {
        return STAGE_DROP;  // 非分钟事件则不往下走 Sink
    }

    minute_acc_state_t *s = (minute_acc_state_t *)stage->state;
    printf("[Freeze] Minute boundary reached! Value: %.2f\n", s->energy_acc);

    // 准备数据传递给下一步 Sink，这里可以重用 ev->data 或使用私有上下文
    return STAGE_CONTINUE;
}

// Stage 3: SQLite 异步落库 (演示异步挂起)
stage_ret_t stage_sqlite_sink(pipeline_stage_t *stage, pipe_event_t *ev) {
    (void)stage;
    (void)ev;
    // 这里会调用 uv_queue_work
    // 返回 STAGE_ASYNC 表示 Pipeline 挂起，等待磁盘 IO
    return STAGE_CONTINUE;
}
