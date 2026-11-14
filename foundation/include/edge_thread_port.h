#ifndef EDGE_THREAD_PORT_H
#define EDGE_THREAD_PORT_H

/**
 * @brief 线程能力端口。由平台适配器在组合根注入，模块不直接创建线程。
 *
 * storage 指向模块自己的线程句柄存储，大小与布局由平台适配器决定。
 * Linux 适配器按 uv_thread_t 解释；其他平台按自己的句柄类型解释。
 */
typedef struct {
    void *ctx;
    int (*create)(void *ctx, void *storage, void (*entry)(void *), void *arg);
    int (*join)(void *ctx, void *storage);
} edge_thread_port_t;

/** 安装端口；必须在任何模块启动线程之前调用。 */
int edge_thread_port_install(const edge_thread_port_t *port);

int edge_thread_create(void *storage, void (*entry)(void *), void *arg);
int edge_thread_join(void *storage);

#endif /* EDGE_THREAD_PORT_H */
