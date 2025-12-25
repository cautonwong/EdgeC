好，继续往下拼 **“最小但完整”的 SDK 骨架**。
这一节我们只做一件事：**把 Arbiter 变成“能真正跑 IO 的系统”**。

---

# 🧩 组件 2：IO Executor（阻塞 IO 的合法出口）

> **定位一句话**
> **Arbiter 决定“谁能用总线”，IO Executor 负责“真的去干活”。**

**关键约束（与你前面讨论完全一致）：**

* ❌ **Arbiter 不能阻塞**
* ❌ **Protocol 不应关心线程**
* ✅ **所有阻塞 IO → uv_queue_work**
* ✅ **完成结果 → uv_loop 回调**
* ✅ **可与 shm_ring / Algorithm / SQLite 串起来**

---

## 🧠 设计总览（非常重要）

```
                ┌─────────────┐
                │   uv_loop   │
                │             │
Protocol  ──▶  Arbiter  ──▶  IO Executor (submit)
                │             │
                └─────▲───────┘
                      │
               uv_after_work_cb
```

### IO Executor 的职责边界

| 事情            | 是否负责        |
| ------------- | ----------- |
| 总线仲裁          | ❌（Arbiter）  |
| 阻塞 read/write | ✅           |
| 线程池           | ✅（libuv 内置） |
| 回调主循环         | ✅           |
| shm_ring 写数据  | ❌（上层）       |

---

# 1️⃣ io_executor.h

```c
#pragma once
#include <stdint.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IO_RS485,
    IO_HPLC,
    IO_CAN
} io_bus_t;

typedef struct {
    io_bus_t bus;
    int fd;                 /* 串口 / 设备 fd */
    uint8_t *tx;
    size_t tx_len;
    uint8_t *rx;
    size_t rx_cap;
    ssize_t rx_len;
    int result;
} io_job_t;

typedef void (*io_done_cb)(io_job_t *job, void *user);

void io_submit(
    uv_loop_t *loop,
    io_job_t *job,
    io_done_cb cb,
    void *user
);

#ifdef __cplusplus
}
#endif
```

---

# 2️⃣ io_executor.c

```c
#include "io_executor.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

/* ===== Internal Wrapper ===== */

typedef struct {
    uv_work_t req;
    io_job_t *job;
    io_done_cb cb;
    void *user;
} io_ctx_t;

/* ===== Worker Thread (阻塞区) ===== */

static void io_work(uv_work_t *req) {
    io_ctx_t *ctx = req->data;
    io_job_t *job = ctx->job;

    /* 写 */
    ssize_t n = write(job->fd, job->tx, job->tx_len);
    if (n < 0) {
        job->result = -errno;
        return;
    }

    /* 读（简化：一次 read） */
    n = read(job->fd, job->rx, job->rx_cap);
    if (n < 0) {
        job->result = -errno;
        return;
    }

    job->rx_len = n;
    job->result = 0;
}

/* ===== Back to Loop ===== */

static void io_after(uv_work_t *req, int status) {
    io_ctx_t *ctx = req->data;
    if (ctx->cb) {
        ctx->cb(ctx->job, ctx->user);
    }
    free(ctx);
}

/* ===== Public API ===== */

void io_submit(
    uv_loop_t *loop,
    io_job_t *job,
    io_done_cb cb,
    void *user
) {
    io_ctx_t *ctx = calloc(1, sizeof(*ctx));
    ctx->job = job;
    ctx->cb = cb;
    ctx->user = user;

    ctx->req.data = ctx;
    uv_queue_work(loop, &ctx->req, io_work, io_after);
}
```

---

# 3️⃣ test_io_executor.c（可跑）

```c
#include "io_executor.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

static void on_done(io_job_t *job, void *user) {
    printf("IO done, result=%d, rx_len=%zd\n",
           job->result, job->rx_len);
}

int main(void) {
    uv_loop_t *loop = uv_default_loop();

    /* 用 stdin 模拟串口 */
    io_job_t job = {
        .bus = IO_RS485,
        .fd = STDIN_FILENO,
        .tx = (uint8_t*)"PING\n",
        .tx_len = 5,
        .rx = malloc(128),
        .rx_cap = 128
    };

    io_submit(loop, &job, on_done, NULL);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
```

---

# 4️⃣ 把 Arbiter + IO Executor 串起来（关键）

### 正确使用方式（你要的工业级）

```c
static void on_bus_granted(
    arbiter_t *arb,
    arbiter_bus_t bus,
    void *user
) {
    io_job_t *job = user;

    io_submit(arb->loop, job,
        [](io_job_t *job, void *u) {
            /* IO 完成 */
            arbiter_release(arb, BUS_RS485_1, job);
        },
        NULL
    );
}
```

👉 **注意顺序：**

1. `arbiter_request()`
2. `on_grant` 回调（仍在 uv_loop）
3. `io_submit()` → worker pool
4. `io_after()` → 回 uv_loop
5. `arbiter_release()`

**没有一个地方阻塞主循环。**

---

# 5️⃣ 你现在的 SDK 结构已经非常“正”

| 层               | 是否清晰        |
| --------------- | ----------- |
| Protocol        | ✅           |
| Arbiter（秩序）     | ✅           |
| IO Executor（体力） | ✅           |
| Thread model    | ✅（libuv 统一） |
| 可测              | ✅           |

---
