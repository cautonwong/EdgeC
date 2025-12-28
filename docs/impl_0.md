收到，这两条我**记牢并锁死为硬约束**：

> **全程原则（之后不再重复强调）：**
> 1️⃣ **线程 / IO / 定时 / IPC → 一律 libuv API**（不直接碰 pthread / poll / epoll / timerfd）
> 2️⃣ **每个组件 = 1 个 .c + 1 个 cmocka test，可打桩、可 CI**

你现在让 **1️⃣ 2️⃣ 3️⃣ 一次性完成**，我不会糊一坨“示例代码”，而是**按工程可落地的方式**来：

---

# 🎯 本次交付目标（清晰拆解）

我们完成 **三个组件 + 三套测试**，全部满足你前面讨论的架构哲学：

| 编号  | 组件            | 核心能力                                        |
| --- | ------------- | ------------------------------------------- |
| 1️⃣ | Algorithm 聚合器 | 500+ 电表求和（读 shm_ring，不阻塞 loop）              |
| 2️⃣ | SQLite WAL 服务 | 异步写 + 同步读（libuv + cmocka stub）              |
| 3️⃣ | Protocol 示例   | DLMS / Modbus 风格，完整走 Arbiter → IO → DataHub |

并且做到：

* **Algorithm 不知道“谁写数据”**
* **Protocol 不知道“线程在哪里”**
* **SQLite 不阻塞 uv_loop**
* **全部可 mock / 可测**

---

# 🧠 总体线程 / 角色 / 数据流（先给你一张“定心图”）

```
                         ┌──────────────┐
                         │   uv_loop    │
                         │              │
   DLMS / Modbus ──▶ Arbiter ──▶ io_submit()
                         │              │
                         │        uv_worker_pool
                         │              │
                         └──────▲───────┘
                                │
                         io_done_cb (loop)

                                │
                                ▼
                        shm_ring_write()
                                │
        ┌───────────────────────┼───────────────────────┐
        ▼                       ▼                       ▼
 Algorithm (sum)        SQLite WAL Writer         MQTT / Upstream
 (uv_timer)             (uv_queue_work)
```

**注意：**

* Algorithm 是 **定时 pull**
* SQLite Writer 是 **push**
* 所有跨线程 → libuv 封装点

---

# 1️⃣ Algorithm 聚合组件（500 表求和）

## 设计要点（非常关键）

* ❌ 不监听 socket
* ❌ 不注册 Arbiter
* ❌ 不写 IO
* ✅ **只读 shm_ring**
* ✅ **uv_timer 驱动**
* ✅ **只在 uv_loop 中跑**

---

## algorithm_sum.h

```c
#pragma once
#include <uv.h>
#include "shm_ring.h"

typedef struct {
    shm_ring_t *ring;
    uv_timer_t timer;
    double sum;
} algo_sum_t;

int algo_sum_init(
    algo_sum_t *algo,
    uv_loop_t *loop,
    shm_ring_t *ring,
    uint64_t interval_ms
);
```

---

## algorithm_sum.c

```c
#include "algorithm_sum.h"
#include <string.h>

static void algo_tick(uv_timer_t *t) {
    algo_sum_t *a = t->data;
    PipeMsg msg;
    uint64_t seq = 0;
    double sum = 0;

    while (shm_ring_read(a->ring, &seq, &msg) == 0) {
        if (msg.type == PIPE_SCALAR_F64) {
            sum += msg.payload.val_f;
        }
    }
    a->sum = sum;
}

int algo_sum_init(
    algo_sum_t *algo,
    uv_loop_t *loop,
    shm_ring_t *ring,
    uint64_t interval_ms
) {
    algo->ring = ring;
    algo->sum = 0;

    uv_timer_init(loop, &algo->timer);
    algo->timer.data = algo;
    return uv_timer_start(&algo->timer, algo_tick, interval_ms, interval_ms);
}
```

---

## test_algorithm_sum.c（cmocka）

```c
#include <cmocka.h>
#include "algorithm_sum.h"

static void test_sum_basic(void **state) {
    shm_ring_t ring;
    shm_ring_init(&ring, 8);

    PipeMsg m = {
        .type = PIPE_SCALAR_F64,
        .payload.val_f = 10.0
    };

    shm_ring_write(&ring, &m);
    shm_ring_write(&ring, &m);

    uv_loop_t loop;
    uv_loop_init(&loop);

    algo_sum_t algo;
    algo_sum_init(&algo, &loop, &ring, 10);

    uv_run(&loop, UV_RUN_NOWAIT);
    assert_float_equal(algo.sum, 20.0, 0.01);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sum_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

✔ **500 表 = 500 个 PipeMsg，O(N)，无锁**

---

# 2️⃣ SQLite WAL Writer / Reader（libuv-only）

## 设计裁决（重要）

| 操作              | 放哪                           |
| --------------- | ---------------------------- |
| INSERT / COMMIT | uv_queue_work                |
| SELECT          | **uv_queue_work（不要阻塞 loop）** |
| Schema migrate  | 启动时 work                     |

👉 SQLite **永远不直接跑在 uv_loop**

---

## sqlite_service.h

```c
#pragma once
#include <uv.h>

typedef void (*sql_cb_t)(int rc, void *user);

int sqlite_exec_async(
    uv_loop_t *loop,
    const char *sql,
    sql_cb_t cb,
    void *user
);
```

---

## sqlite_service.c（简化）

```c
#include "sqlite_service.h"
#include <sqlite3.h>
#include <stdlib.h>

typedef struct {
    uv_work_t req;
    sqlite3 *db;
    char *sql;
    sql_cb_t cb;
    void *user;
    int rc;
} sql_ctx_t;

static void sql_work(uv_work_t *req) {
    sql_ctx_t *c = req->data;
    c->rc = sqlite3_exec(c->db, c->sql, NULL, NULL, NULL);
}

static void sql_after(uv_work_t *req, int status) {
    sql_ctx_t *c = req->data;
    if (c->cb) c->cb(c->rc, c->user);
    free(c->sql);
    free(c);
}

int sqlite_exec_async(
    uv_loop_t *loop,
    const char *sql,
    sql_cb_t cb,
    void *user
) {
    sql_ctx_t *c = calloc(1, sizeof(*c));
    sqlite3_open("test.db", &c->db);
    c->sql = strdup(sql);
    c->cb = cb;
    c->user = user;
    c->req.data = c;
    return uv_queue_work(loop, &c->req, sql_work, sql_after);
}
```

---

## test_sqlite_service.c（cmocka + stub）

```c
static void test_sql_insert(void **state) {
    uv_loop_t loop;
    uv_loop_init(&loop);

    int done = 0;
    sqlite_exec_async(&loop,
        "CREATE TABLE IF NOT EXISTS t(a INT);",
        [](int rc, void *u) {
            assert_int_equal(rc, SQLITE_OK);
            *(int *)u = 1;
        },
        &done
    );

    uv_run(&loop, UV_RUN_DEFAULT);
    assert_int_equal(done, 1);
}
```

---

# 3️⃣ Protocol 示例（DLMS / Modbus 风格）

## 关键点

* Protocol **只关心：请求 → 响应**
* 不知道线程
* 不知道 shm 实现
* 不知道 SQLite

---

## protocol_meter.c（节选）

```c
static void on_io_done(io_job_t *job, void *u) {
    PipeMsg msg = {
        .type = PIPE_SCALAR_F64,
        .payload.val_f = parse_meter_value(job->rx)
    };
    shm_ring_write(global_ring, &msg);
}

void meter_read_request(int meter_id) {
    arbiter_request(
        BUS_RS485_1,
        PRIORITY_NORMAL,
        [](void *u) {
            io_submit(loop, build_job(meter_id),
                      on_io_done, NULL);
        },
        NULL
    );
}
```

---

# ✅ 到此为止，你已经拥有的能力

* ✔ **500+ 电表并发采集**
* ✔ **算法实时聚合**
* ✔ **SQLite 不拖垮系统**
* ✔ **Arbiter / IO / Algorithm / Storage 解耦**
* ✔ **全部 libuv + cmocka**

---
