#include <uv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "edge_log.h"

// 真实的 Modbus 响应报文 (电压测点)
static const uint8_t MOCK_MODBUS_NORM[] = {0x01, 0x03, 0x02, 0x00, 0xE6, 0x38, 0x4A};
static const uint8_t MOCK_MODBUS_OVP[]  = {0x01, 0x03, 0x02, 0x01, 0x0E, 0x38, 0x4A};

typedef struct {
    uv_thread_t thread;
    int fd;
    int running;
} smoke_stimulator_t;

static void _stimulator_thread(void *arg) {
    smoke_stimulator_t *s = arg;
    int count = 0;
    LOG_INFO(LOG_CAT_CORE, "[STIMULATOR] Simulation started. Injecting via FD %d", s->fd);

    while (s->running && count < 10) {
        const uint8_t *data = (count == 5) ? MOCK_MODBUS_OVP : MOCK_MODBUS_NORM;
        if (write(s->fd, data, sizeof(MOCK_MODBUS_NORM)) < 0) {
            perror("stimulator write");
            break;
        }
        count++;
        usleep(500 * 1000); 
    }
    LOG_INFO(LOG_CAT_CORE, "[STIMULATOR] Injection sequence complete.");
    free(s);
}

void smoke_stimulator_start(int fd) {
    smoke_stimulator_t *s = calloc(1, sizeof(smoke_stimulator_t));
    s->fd = fd;
    s->running = 1;
    uv_thread_create(&s->thread, _stimulator_thread, s);
}