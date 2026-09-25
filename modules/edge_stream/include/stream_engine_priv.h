#ifndef STREAM_ENGINE_PRIV_H
#define STREAM_ENGINE_PRIV_H

#include <uv.h>

#include "queue.h"
#include "stream_engine.h"

#define MAX_PENDING_QUEUE 16
#define MAX_EXTRA_TRIGGERS_PER_PIPE 16
#define MAX_PIPELINES_PER_ENGINE 32
#define TIME_JUMP_THRESHOLD_MS 5000

typedef enum { PIPE_STATE_IDLE, PIPE_STATE_RUNNING, PIPE_STATE_SUSPENDED } pipeline_state_t;

struct pipeline_s {
    struct uv__queue node;
    char id[64];
    uint32_t trigger_tag;
    uint32_t extra_triggers[MAX_EXTRA_TRIGGERS_PER_PIPE];      // [Omnis 扩展]
    uint32_t extra_trigger_count;  // [Omnis 扩展]
    pipeline_state_t state;
    bool in_use;
    struct stream_engine_s *engine;  // Set the back-pointer to parent engine

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
    uv_loop_t compute_loop_inst;
    pipeline_t pipe_pool[MAX_PIPELINES_PER_ENGINE];
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

    const edge_stage_factory_entry_t *registry;
};

#endif