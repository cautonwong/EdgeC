#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stream_engine.h"

typedef struct { uint32_t tag_id; double current_acc; uint32_t sample_count; uint64_t last_ts; } aggregator_state_t;

stage_ret_t stage_accumulator(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK; 
    aggregator_state_t *state = (aggregator_state_t *)stage->priv_state;
    state->current_acc += ev->payload.val; state->sample_count++; state->last_ts = ev->calendar_ms;
    return STAGE_OK;
}

stage_ret_t stage_freeze_handler(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_TIME_PULSE || ev->payload.tick_kind != 1) return STAGE_OK; 
    aggregator_state_t *state = (aggregator_state_t *)stage->priv_state;
    if (state->sample_count == 0) return STAGE_DROP;
    printf("[Aggregator] MINUTE_FREEZE: Tag=%u, Val=%.3f, Samples=%u, TS=%lu\n",
           state->tag_id, state->current_acc / state->sample_count, state->sample_count, ev->calendar_ms);
    state->current_acc = 0; state->sample_count = 0;
    return STAGE_OK;
}

void add_minute_aggregator_pipeline(stream_engine_t *engine, uint32_t tag_id) {
    char pipe_id[32]; snprintf(pipe_id, 32, "agg_pipe_%u", tag_id);
    pipeline_t *p = stream_engine_add_pipeline(engine, pipe_id, tag_id);
    if (!p) return;
    aggregator_state_t *state = calloc(1, sizeof(aggregator_state_t));
    state->tag_id = tag_id;
    // 适配 v2.0：注入清理回调
    pipeline_add_stage(p, "acc", stage_accumulator, state, NULL, NULL, NULL);
    pipeline_add_stage(p, "freeze", stage_freeze_handler, state, free, NULL, NULL);
}