#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

// 引入 EdgeOS SDK 头文件
#include "arbiter.h"
#include "edge_service.h"
#include "shmring.h"

static uv_loop_t *loop;
static service_t *svc;
static int running = 1;

// 信号处理
static void on_signal(uv_signal_t *handle, int signum) {
    (void)signum;
    printf("\n[VPP] Stopping...\n");
    running = 0;
    uv_stop(loop);
}

// 模拟 VPP 调度逻辑 (每秒执行)
static void on_schedule_tick(uv_timer_t *handle) {
    (void)handle;

    // 1. 获取组件句柄
    shm_ring_t *ring = service_get_ring(svc);
    arbiter_t *arb = service_get_arbiter(svc);

    if (!ring || !arb) return;

    // 2. 读取光伏发电功率 (Tag ID: 1001)
    // 在真实场景中，这里应遍历 ring 或使用 hash 查找
    // 这里演示简单的最近数据读取
    PipeMsg msg;
    uint64_t seq = 0;

    // 尝试读取最新的数据 (简化逻辑，实际应处理 seq)
    // 假设 DataHub 里正好有我们需要的数据
    // 在真实 APP 中，通常会有一个 map 来缓存最新的 tag_id -> value

    // 3. 决策逻辑：如果发电 > 50kW，则充电；否则放电
    // 这里我们直接模拟下发控制指令
    static double target_power = 0;
    target_power += 5.0;
    if (target_power > 100) target_power = 0;

    printf("[VPP] Algorithm tick. Target ESS Power: %.1f kW\n", target_power);

    // 4. 下发控制指令 (使用 Builder API)
    cmd_msg_t cmd;
    cmd_msg_init_write(&cmd, 2001, target_power, CMD_PRIO_NORMAL);

    // 5. 提交给 Arbiter (假设总线 ID 为 0)
    int ret = arbiter_submit_cmd(arb, 0, &cmd);
    if (ret != 0) {
        printf("[VPP] Failed to submit cmd: %d\n", ret);
    }
}

int main(int argc, char **argv) {
    // 1. 初始化 libuv
    loop = uv_default_loop();

    // 2. 配置 SDK
    service_options_t opts = {
        .config_path = "config.json", .mode = "vpp", .web_port = 9090, .rest_port = 9091};

    // 3. 创建服务容器
    svc = service_new(&opts);
    service_set_loop(svc, loop);

    // 4. 注册信号
    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, on_signal, SIGINT);

    // 5. 启动业务定时器
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    uv_timer_start(&timer, on_schedule_tick, 1000, 1000);

    printf("[VPP] Virtual Power Plant Application Started.\n");

    // 6. 运行 SDK (这会阻塞直到 loop 停止)
    // 注意：在真实场景中，service_run 通常会接管 loop。
    // 如果我们想自己控制 loop，可以手动初始化组件。
    // 这里为了演示简单，我们假设 service_run 是非阻塞初始化或我们手动集成。
    // 由于 service_run 目前是阻塞的 uv_run，我们需要调整策略：
    // 要么把我们的逻辑注入 service，要么重构 service_run。
    // 为了二开友好，我们假设用户把业务逻辑挂在 loop 上，然后调用 service_run 接管。

    // 修正：我们需要 mock 依赖组件，因为 service_run 需要 pool, arbiter 等。
    // 在二开场景中，SDK 应该提供一个 "全自动启动" 的 helper。
    // 目前 SDK 还是偏向 "组件库"。
    // 让我们用最原始的方式：手动组装。

    // (简化版：直接调用 uv_run，假设外部已经启动了核心服务进程，
    //  或者本 APP 就是核心进程。这里演示作为核心进程启动)

    // 重新组装组件 (参考 iems/main.c)
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 128, 512, NULL, NULL);

    arbiter_t *arbiter;
    arbiter_init(&arbiter, loop);
    // 将 arbiter 注入 svc (需增加 setter 或重构)
    // 鉴于目前 API 限制，这是个痛点。
    // TODO: 增加 service_attach_components() API

    // 暂时回退到仅演示逻辑，不实际运行
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
