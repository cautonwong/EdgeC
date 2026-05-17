/**
 * @file stage_radar_tracker.c
 * @brief 雷达多目标追踪算子
 */

#include "stream_engine.h"
#include "radar_algo.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>

#define MAX_TRACKS 32

typedef struct {
    radar_target_t tracks[MAX_TRACKS];
    uint32_t output_tag;
    uint64_t last_ts;
} radar_tracker_ctx_t;

static stage_ret_t _radar_tracker_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    radar_tracker_ctx_t *ctx = (radar_tracker_ctx_t *)stage->priv_state;
    
    // 期望输入是检测到的点迹 (x, y 数组对)
    if (ev->type == PIPEMSG_TYPE_BATCH && ev->payload.batch.count > 0) {
        uint32_t det_count = ev->payload.batch.count / 2;
        double *det_x = (double *)ev->payload.batch.ptr;
        double *det_y = det_x + det_count;

        uint64_t now = ev->ts;
        double dt = (ctx->last_ts == 0) ? 0.05 : (double)(now - ctx->last_ts) / 1000.0;
        ctx->last_ts = now;

        radar_tracker_update(ctx->tracks, MAX_TRACKS, det_x, det_y, det_count, dt);

        // 构造输出：活跃的目标
        // 此处简化处理：我们将活跃的目标写回 Batch
        uint32_t active_count = 0;
        double *out_ptr = (double *)ev->payload.batch.ptr;
        
        for (int i = 0; i < MAX_TRACKS; i++) {
            if (ctx->tracks[i].active && ctx->tracks[i].hits >= 2) {
                out_ptr[active_count * 2] = ctx->tracks[i].x;
                out_ptr[active_count * 2 + 1] = ctx->tracks[i].y;
                active_count++;
            }
        }
        
        ev->payload.batch.count = active_count * 2;
        ev->tag_id = ctx->output_tag;
        
        return (active_count > 0) ? STAGE_OK : STAGE_DROP;
    }

    return STAGE_DROP;
}

static void _radar_tracker_cleanup(void *priv) {
    if (priv) free(priv);
}

pipeline_stage_t *stage_radar_tracker_create(uint32_t output_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    radar_tracker_ctx_t *ctx = calloc(1, sizeof(radar_tracker_ctx_t));
    
    ctx->output_tag = output_tag;
    
    strncpy(s->name, "RADAR_TRACKER", 31);
    s->process = _radar_tracker_proc;
    s->priv_state = ctx;
    s->cleanup_state = _radar_tracker_cleanup;
    
    LOG_INFO(LOG_CAT_CORE, "Radar Tracker Stage created (Max Tracks: %d)", MAX_TRACKS);
    return s;
}

pipeline_stage_t *stage_radar_tracker_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    return stage_radar_tracker_create((uint32_t)def->params.u1);
}
