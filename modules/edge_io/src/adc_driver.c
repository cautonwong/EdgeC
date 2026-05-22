#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "datahub.h"
#include "edge_log.h"
#include "shmring.h"

#define MAX_ADC_BUFFERS 16

typedef struct {
    int fds[MAX_ADC_BUFFERS];
    void *ptrs[MAX_ADC_BUFFERS];
    size_t block_size;
    int buffer_count;
} adc_client_ctx_t;

/**
 * @brief 处理来自 Python 仿真器的高频数据流 (Task 5.6)
 */
/**
 * @brief 处理来自 Python 仿真器的高频数据流 (Task 5.6)
 */
extern volatile bool g_adc_running;

void adc_driver_handle_connection(int sock_fd, datahub_t *hub) {
    adc_client_ctx_t ctx = {0};
    for (int i = 0; i < MAX_ADC_BUFFERS; i++) ctx.fds[i] = -1;

    // [Nexus] 回归阻塞模式。ADC 线程是独立物理线程，阻塞读可以获得最低延迟。
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags & ~O_NONBLOCK);

    // 1. 握手：读取元数据
    struct {
        uint32_t block_size;
        uint32_t buffer_count;
    } meta;

    // 握手期间使用超时逻辑
    struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (read(sock_fd, &meta, sizeof(meta)) != sizeof(meta)) {
        LOG_ERROR(LOG_CAT_BUS, "ADC Driver: Handshake failed or timed out");
        close(sock_fd);
        return;
    }

    // 握手成功后恢复无限等待 (由 g_adc_running 控制退出)
    tv.tv_sec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ctx.block_size = meta.block_size;
    ctx.buffer_count = (int)meta.buffer_count;
    printf("[ADC-DRV] Handshake: BlockSize=%zu, Buffers=%d\n", ctx.block_size, ctx.buffer_count);

    // 2. 接收循环
    while (g_adc_running) {
        PipeMsg header_msg;
        struct msghdr msg = {0};
        struct iovec iov[1];
        char control[CMSG_SPACE(sizeof(int))];

        iov[0].iov_base = &header_msg;
        iov[0].iov_len = sizeof(header_msg);
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_control = control;
        msg.msg_controllen = sizeof(control);

        ssize_t n = recvmsg(sock_fd, &msg, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            LOG_WARN(LOG_CAT_BUS, "ADC Driver: Recv error (%s)", strerror(errno));
            break;
        } else if (n == 0) {
            LOG_INFO(LOG_CAT_BUS, "ADC Driver: Python simulator closed connection.");
            break;
        }

        // 提取传递的 FD
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            int received_fd = *((int *)CMSG_DATA(cmsg));

            // 我们通过 header_msg 最后的 4 字节负载传递 buffer_idx
            int32_t buf_idx = 0;
            memcpy(&buf_idx, ((uint8_t *)&header_msg) + 60, 4);
            buf_idx %= MAX_ADC_BUFFERS;

            // [Nexus Fix] 回归一次性映射策略。在高频模式下，频繁 mmap 会导致严重性能问题。
            // 只有当该槽位从未被映射过时，才进行映射。
            if (ctx.ptrs[buf_idx] == NULL) {
                ctx.fds[buf_idx] = received_fd;
                ctx.ptrs[buf_idx] =
                    mmap(NULL, ctx.block_size * 4, PROT_READ, MAP_SHARED, received_fd, 0);
                // 注册到 DataHub
                datahub_register_ref(hub, header_msg.tag_id, received_fd, ctx.block_size * 4);
            } else {
                // 已经映射过，直接关闭重复的 FD 避免资源耗尽。
                // 注意：在 Linux SCM_RIGHTS 中，每次 recvmsg 都会产生一个新的 FD 指向相同文件。
                close(received_fd);
            }

            // 修正 PipeMsg 负载，指向本地映射的指针
            header_msg.type = PIPEMSG_TYPE_REF;
            header_msg.payload.ref.ptr = ctx.ptrs[buf_idx];
            header_msg.payload.ref.len = ctx.block_size;
            header_msg.payload.ref.fd = ctx.fds[buf_idx];

            // 3. 发布到 DataHub
            datahub_publish(hub, &header_msg);
        }
    }

    // 清理
    for (int i = 0; i < ctx.buffer_count; i++) {
        if (ctx.ptrs[i]) munmap(ctx.ptrs[i], ctx.block_size * 4);
        if (ctx.fds[i] >= 0) close(ctx.fds[i]);
    }
    close(sock_fd);
}