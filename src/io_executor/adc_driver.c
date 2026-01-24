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
void adc_driver_handle_connection(int sock_fd, datahub_t *hub) {
    adc_client_ctx_t ctx = {0};
    for (int i = 0; i < MAX_ADC_BUFFERS; i++) ctx.fds[i] = -1;

    // 1. 握手：读取元数据 (阻塞读)
    struct {
        uint32_t block_size;
        uint32_t buffer_count;
    } meta;

    if (read(sock_fd, &meta, sizeof(meta)) != sizeof(meta)) {
        LOG_ERROR(LOG_CAT_BUS, "ADC Driver: Handshake failed");
        close(sock_fd);
        return;
    }

    ctx.block_size = meta.block_size;
    ctx.buffer_count = (int)meta.buffer_count;
    printf("[ADC-DRV] Handshake: BlockSize=%zu, Buffers=%d\n", ctx.block_size, ctx.buffer_count);

    // 2. 接收循环 (阻塞读)
    while (1) {
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
        if (n <= 0) {
            LOG_WARN(LOG_CAT_BUS, "ADC Driver: Stream closed or error (%s)", strerror(errno));
            break;
        }

        // 提取传递的 FD
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            int received_fd = *((int *)CMSG_DATA(cmsg));

            // 我们从 64 字节消息的最末尾 4 字节读取 buf_idx
            int32_t buf_idx = 0;
            memcpy(&buf_idx, ((uint8_t *)&header_msg) + 60, 4);
            buf_idx %= MAX_ADC_BUFFERS;

            // 如果该槽位尚未映射，进行一次性映射 (Zero-Copy 准备)
            if (ctx.ptrs[buf_idx] == NULL) {
                ctx.fds[buf_idx] = received_fd;
                ctx.ptrs[buf_idx] =
                    mmap(NULL, ctx.block_size * 4, PROT_READ, MAP_SHARED, received_fd, 0);
                // 注册到 DataHub 以便后续跨进程访问 (如果需要)
                datahub_register_ref(hub, header_msg.tag_id, received_fd, ctx.block_size * 4);
            } else {
                // 已经映射过，直接关闭重复的 FD 避免泄漏
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