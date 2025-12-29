#ifndef STREAM_ENGINE_PRIV_H
#define STREAM_ENGINE_PRIV_H

#include "stream_engine.h"

#define MAX_PENDING_QUEUE 256
#define MAX_PIPELINES 32
#define TIME_JUMP_THRESHOLD_MS 5000

typedef enum { PIPE_IDLE, PIPE_RUNNING, PIPE_SUSPENDED } pipeline_state_t;

struct pipeline_s {
    char id[64];
    uint32_t trigger_tag;
    pipeline_state_t state;

    pipeline_stage_t *head;
    pipeline_stage_t *tail;  // 新增 tail 指针
    pipeline_stage_t *resume_point;
    pipe_event_t current_event;
    pipe_event_t queue[MAX_PENDING_QUEUE];
    int q_head, q_tail;
};

struct stream_engine_s {
    uv_loop_t *loop;
    datahub_t *hub;
    uv_thread_t thread;
    bool thread_started;

    pipeline_t *pipelines[MAX_PIPELINES];
    int pipe_count;

    uv_async_t notify_handle;
    uv_timer_t time_guard;
    uv_mutex_t pipe_mutex;

    engine_time_source_t time_src;
    uint64_t last_calendar_min;
    uint64_t bus_read_seq;
    int64_t mono_to_cal_offset_ms;

    bool notify_inited;
    bool timer_inited;
};

#endif
