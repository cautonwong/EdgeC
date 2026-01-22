#ifndef STREAM_ENGINE_PRIV_H
#define STREAM_ENGINE_PRIV_H

#include <uv.h>

#include "queue.h"
#include "stream_engine.h"

#define MAX_PENDING_QUEUE 256
#define TIME_JUMP_THRESHOLD_MS 5000

typedef enum { 
    PIPE_STATE_IDLE, 
    PIPE_STATE_RUNNING, 
    PIPE_STATE_SUSPENDED 
} pipeline_state_t;

struct pipeline_s {
    struct uv__queue node;
    char id[64];
    uint32_t trigger_tag;
    pipeline_state_t state;
    struct stream_engine_s *engine; // Back-pointer to parent engine

    pipeline_stage_t *head;
    pipeline_stage_t *tail;

    // [SE-04] Fields for resumable state machine
    pipeline_stage_t *resume_point;
    pipe_msg_t current_event;
    
    // [SE-05] Fields for back-pressure queue
    pipe_msg_t queue[MAX_PENDING_QUEUE];
    int q_head, q_tail;
};

struct stream_engine_s {
    uv_loop_t *loop;
    datahub_t *hub;
    uv_thread_t thread;
    bool thread_started;

    struct uv__queue pipelines;
    int pipe_count;

    uv_async_t notify_handle;
    uv_async_t stop_async;
    uv_sem_t start_sem;
    uv_mutex_t pipe_mutex;

    engine_time_source_t time_src;
    uint64_t last_calendar_min;
    uint64_t bus_read_seq;
    int64_t mono_to_cal_offset_ms;

    bool notify_inited;
    bool timer_inited;
};

#endif