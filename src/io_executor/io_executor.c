/**
 * @file io_executor.c
 * @brief 执行平面 I/O 调度核心
 */

#include "io_executor.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>

#include "datahub.h"
#include "edge_driver.h"
#include "edge_log.h"

/**
 * @brief 智能通道打开器 (V4.5 增强版)
 */
int io_channel_open(const char *path, int baud) {
    if (!path) return -1;

    // 1. 串口处理
    if (strstr(path, "/dev/") != NULL) {
        int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) return -1;

        struct termios tio;
        if (tcgetattr(fd, &tio) != 0) {
            close(fd);
            return -1;
        }
        cfsetospeed(&tio, baud == 115200 ? B115200 : B9600);
        cfsetispeed(&tio, baud == 115200 ? B115200 : B9600);
        tio.c_cflag = (tio.c_cflag & (unsigned int)~CSIZE) | CS8 | CLOCAL | CREAD;
        tcsetattr(fd, TCSANOW, &tio);
        return fd;
    }

    // 2. UDS 仿真通道处理 (带重试逻辑)
    struct stat st;
    int max_retries = 10;
    while (max_retries--) {
        if (stat(path, &st) == 0 && S_ISSOCK(st.st_mode)) {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd >= 0) {
                struct sockaddr_un addr;
                memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_UNIX;
                strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

                if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                    int flags = fcntl(fd, F_GETFL, 0);
                    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
                    LOG_INFO(LOG_CAT_IO, "Simulation Link Connected (UDS): %s", path);
                    return fd;
                }
                close(fd);
            }
        }
        usleep(200000);  // 等待 200ms
    }

    LOG_ERROR(LOG_CAT_IO, "Failed to connect to simulation channel after retries: %s", path);
    return -1;
}

io_executor_t *io_executor_create(edge_driver_t *driver, datahub_t *hub) {
    (void)driver;
    (void)hub;
    return NULL;  // Legacy V1 接口已废弃
}

void io_executor_run(io_executor_t *ctx) {
    (void)ctx;
}

void io_executor_destroy(io_executor_t *ctx) {
    (void)ctx;
}