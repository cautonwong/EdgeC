#include <stdio.h>
#include "edge_common_types.h"
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "stream_engine.h"

typedef struct {
    uint32_t tag_id;
    double current_acc;
    uint32_t sample_count;
    uint64_t last_ts;
} aggregator_state_t;


/* File-local prototypes. */
stage_ret_t stage_accumulator(pipeline_stage_t *stage, pipe_msg_t *ev);
stage_ret_t stage_freeze_handler(pipeline_stage_t *stage, pipe_msg_t *ev);
void add_minute_aggregator_pipeline(stream_engine_t *engine, uint32_t tag_id);
stage_ret_t stage_accumulator(pipeline_stage_t *stage, pipe_msg_t *ev) {    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    aggregator_state_t *state = (aggregator_state_t *)stage->priv_state;
    state->current_acc += ev->payload.val.f;
    state->sample_count++;
    state->last_ts = ev->ts;
    return STAGE_OK;
}

stage_ret_t stage_freeze_handler(pipeline_stage_t *stage, pipe_msg_t *ev) {    // 假设时间脉冲通过 EVENT 类型下发，且 event_id = USER_CUSTOM(100), param = 1
    if (ev->type != PIPEMSG_TYPE_EVENT || ev->payload.event.event_id != SYS_EVENT_USER_CUSTOM ||
        ev->payload.event.param != 1) {
        return STAGE_OK;
    }

    aggregator_state_t *state = (aggregator_state_t *)stage->priv_state;
    if (state->sample_count == 0) return STAGE_DROP;

    LOG_INFO(LOG_CAT_CORE, "MINUTE_FREEZE: Tag=%u, Val=%.3f, Samples=%u, TS=%lu", state->tag_id,
             state->current_acc / state->sample_count, state->sample_count, ev->ts);

    state->current_acc = 0;
    state->sample_count = 0;
    return STAGE_OK;
}

