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
    uint32_t thd_tag;
    datahub_t *hub;
} dsp_cfg_t;

stage_ret_t dsp_rms_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (!ev->payload.ref.ptr) return STAGE_DROP;
    dsp_cfg_t *cfg = (dsp_cfg_t *)s->priv_cfg;
    if (!cfg) return STAGE_DROP;

    float *samples = (float *)ev->payload.ref.ptr;
    int N = 200; // 4.1 要求：以 200 个离散样本为一帧窗口
    double sum_sq = 0;
    double sum_sin = 0;
    double sum_cos = 0;

    for (int i = 0; i < N; i++) {
        float val = samples[i];
        sum_sq += (double)val * (double)val;
        
        // 假设 200 个样本是一个基波周期 (比如 50Hz，采样率 10kHz)
        double angle = 2.0 * M_PI * i / N;
        sum_sin += val * sin(angle);
        sum_cos += val * cos(angle);
    }
    
    double rms = sqrt(sum_sq / N);
    
    // DFT 提取基波振幅 (V1 = 2/N * sqrt(sum_sin^2 + sum_cos^2))
    double v1_amp = (2.0 / N) * sqrt(sum_sin * sum_sin + sum_cos * sum_cos);
    double v1_rms = v1_amp / sqrt(2.0);
    
    // 计算 THD: sqrt(RMS^2 - V1_rms^2) / V1_rms
    double thd = 0.0;
    if (v1_rms > 0.01 && rms > v1_rms) {
        thd = sqrt(rms * rms - v1_rms * v1_rms) / v1_rms;
    }

    if (cfg->hub) {
        pipe_msg_t res_rms = {0};
        pipe_msg_init_scalar(&res_rms, cfg->target_tag, rms, ev->ts);
        datahub_publish(cfg->hub, &res_rms);
        
        if (cfg->thd_tag > 0) {
            pipe_msg_t res_thd = {0};
            pipe_msg_init_scalar(&res_thd, cfg->thd_tag, thd, ev->ts);
            datahub_publish(cfg->hub, &res_thd);
        }
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

    dsp_cfg_t *cfg = malloc(sizeof(dsp_cfg_t));
    cfg->target_tag = target_tag;
    // 假设 THD tag 是 target_tag + 1（例如 RMS 是 201，THD 就是 202）
    cfg->thd_tag = target_tag + 1; 
    cfg->hub = hub;
    pipeline_add_stage(p, "rms_thd", dsp_rms_proc, NULL, NULL, cfg, free);
}
