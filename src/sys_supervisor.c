#include "sys_supervisor.h"
#include "edge_event.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

static void on_time_jump(uv_poll_t *handle, int status, int events) {
    (void)status; (void)events;
    sys_supervisor_t *sv = (sys_supervisor_t *)handle->data;
    
    uint64_t expirations;
    uv_buf_t buf = uv_buf_init((char*)&expirations, sizeof(expirations));
    uv_fs_t read_req;

    // Use libuv fs API for reading from the timerfd
    int r = uv_fs_read(sv->loop, &read_req, sv->tfd, &buf, 1, -1, NULL);
    uv_fs_req_cleanup(&read_req);

    if (r < 0) {
        // TFD_TIMER_CANCEL_ON_SET results in ECANCELED when the clock is set
        if (read_req.result == -ECANCELED || errno == ECANCELED) {
            printf("[Supervisor] CRITICAL: System time jump detected! Notifying system...\n");
            edge_event_publish(sv->ring, SYS_EVENT_TIME_CHANGED, 0);
            
            // Rearm the timerfd
            struct itimerspec new_value = {0};
            new_value.it_value.tv_sec = 3600; 
            timerfd_settime(sv->tfd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &new_value, NULL);
        }
    }
}

int sys_supervisor_init(sys_supervisor_t *sv, uv_loop_t *loop, shm_ring_t *ring) {
    sv->loop = loop;
    sv->ring = ring;

    // timerfd_create is Linux specific, no libuv equivalent for TFD_TIMER_CANCEL_ON_SET
    sv->tfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
    if (sv->tfd < 0) return -1;

    struct itimerspec its = {0};
    its.it_value.tv_sec = 3600; 
    if (timerfd_settime(sv->tfd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &its, NULL) != 0) {
        close(sv->tfd);
        return -1;
    }

    uv_poll_init(loop, &sv->tfd_poll, sv->tfd);
    sv->tfd_poll.data = sv;
    uv_poll_start(&sv->tfd_poll, UV_READABLE, on_time_jump);

    printf("[Supervisor] Time jump monitoring active.\n");
    return 0;
}

void sys_supervisor_close(sys_supervisor_t *sv) {
    if (!uv_is_closing((uv_handle_t*)&sv->tfd_poll)) {
        uv_poll_stop(&sv->tfd_poll);
        uv_close((uv_handle_t*)&sv->tfd_poll, NULL);
    }
    
    // Use libuv fs API for closing the fd
    uv_fs_t close_req;
    uv_fs_close(sv->loop, &close_req, sv->tfd, NULL);
    uv_fs_req_cleanup(&close_req);
}