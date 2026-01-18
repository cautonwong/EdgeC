#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "datahub.h"
#include "stream_engine.h"

typedef struct {
    uint32_t target_tag;
    datahub_t *hub;
} rms_cfg_t;

stage_ret_t dsp_rms_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (!ev->payload.ref.ptr) return STAGE_DROP;
    rms_cfg_t *cfg = (rms_cfg_t *)s->priv_cfg;
    if (!cfg) return STAGE_DROP;

    float *samples = (float *)ev->payload.ref.ptr;
    double sum_sq = 0;
    for (size_t i = 0; i < 400; i++) {
        float val = samples[i];
        sum_sq += (double)val * (double)val;
    }
    float rms = (float)sqrt(sum_sq / 400.0);

    if (cfg->hub) {
        pipe_msg_t res = {0};
        pipe_msg_init_scalar(&res, cfg->target_tag, (double)rms, ev->ts);
        datahub_publish(cfg->hub, &res);
    }
    return STAGE_OK;
}

stage_ret_t dma_source_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    (void)s;
    static int current_fd = -1;
    static void *map_ptr = NULL;
    if (ev->payload.ref.fd > 0 && ev->payload.ref.fd != current_fd) {
        if (map_ptr) munmap(map_ptr, 4096);
        map_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, ev->payload.ref.fd, 0);
        current_fd = ev->payload.ref.fd;
    }
    if (map_ptr) {
        ev->type = PIPEMSG_TYPE_REF;
        ev->payload.ref.ptr = map_ptr;
        ev->payload.ref.len = 1600;
        return STAGE_OK;
    }
    return STAGE_DROP;
}

// [Sprint 4] 公开的仿真管线注册接口
void stream_engine_register_sim_pipeline(stream_engine_t *engine, datahub_t *hub,
                                         uint32_t trigger_tag, uint32_t target_tag) {
    pipeline_t *p = stream_engine_add_pipeline(engine, "omnis_sim", trigger_tag);
    pipeline_add_stage(p, "dma", dma_source_proc, NULL, NULL, NULL, NULL);

    rms_cfg_t *cfg = malloc(sizeof(rms_cfg_t));
    cfg->target_tag = target_tag;
    cfg->hub = hub;
    pipeline_add_stage(p, "rms", dsp_rms_proc, NULL, NULL, cfg, free);
}
