非常好，这个问题**正中 DataHub 的“灵魂”**。

结论先给出一句话版：

> **shm_ring 负责“数据可见性”，通知机制负责“谁该醒来”**
> 二者必须解耦；DataHub = shm_ring + Notification Plane

下面我从**原则 → 机制选型 → 代码级设计（libuv-only）→ 多线程/多进程**一步步把这件事讲清楚，并直接给你**可落地的 DataHub 通知模型**。

---

## 一、原则澄清（非常关键）

### ❌ 错误直觉

> “shm_ring 里有数据了，对方自然就知道了”

这是**共享内存新手最常见的误区**。

shm_ring / seqlock **只解决一致性，不解决调度**：

* ✔ 保证读到的数据是完整的
* ❌ 不会唤醒任何线程
* ❌ 不会触发任何事件
* ❌ 不会减少 CPU 空转

如果读者用 `while(1) { peek(); }`：

* IEMS 的 CPU 会被你烧穿
* cache line 会被你刷爆

---

## 二、正确模型：双平面设计（Data + Notify）

```
            ┌────────────┐
Producer ──►│ shm_ring   │───► Consumer (读数据)
            └────────────┘
                  ▲
                  │
            ┌────────────┐
            │ Notification│───► uv_loop 被唤醒
            └────────────┘
```

👉 **DataHub = shm_ring + Notification**

---

## 三、Notification Plane 的唯一正确选型（libuv 语境）

我们只考虑 **libuv API**（符合你定的铁律）：

| 场景              | 机制         | libuv API    |
| --------------- | ---------- | ------------ |
| 同进程 / 多线程       | 事件唤醒       | `uv_async_t` |
| 跨进程             | FD 级唤醒     | `uv_pipe_t`  |
| 多进程 + 无 uv_loop | 不支持（设计上禁止） |              |

**结论：**

* **SDK 内部线程 → `uv_async_t`**
* **SDK ↔ App / Algorithm 进程 → `uv_pipe_t`**

下面先把 **DataHub v1：uv_async** 给你补全。

---

## 四、DataHub 正确的通知模型（线程级）

### 1️⃣ DataHub 结构体升级

```c
typedef struct datahub_s {
    shm_ring_t *ring;

    uv_loop_t  *loop;

    uv_async_t  async;        // 唤醒用
    void      (*on_data)(void *user);
    void       *user_data;
} datahub_t;
```

---

### 2️⃣ 初始化（绑定 uv_loop）

```c
int datahub_init(datahub_t *hub,
                 uv_loop_t *loop,
                 size_t ring_slots,
                 void (*on_data)(void *),
                 void *user)
{
    hub->ring = shm_ring_create(ring_slots, sizeof(PipeMsg));
    if (!hub->ring) return -1;

    hub->loop = loop;
    hub->on_data = on_data;
    hub->user_data = user;

    uv_async_init(loop, &hub->async, [](uv_async_t *h){
        datahub_t *hub = h->data;
        if (hub->on_data)
            hub->on_data(hub->user_data);
    });
    hub->async.data = hub;

    return 0;
}
```

> ⚠ 注意：
>
> * `uv_async_send()` **是线程安全的**
> * callback **一定在 loop 线程执行**

这是 **libuv 给你的最强保证**

---

### 3️⃣ publish：写完就唤醒

```c
int datahub_publish(datahub_t *hub, PipeMsg *msg)
{
    int r = shm_ring_push(hub->ring, msg);
    if (r == 0) {
        uv_async_send(&hub->async);   // 唤醒 loop
    }
    return r;
}
```

> ⚠ 非常重要的设计点：
>
> * **不关心有没有人订阅**
> * **不关心唤醒次数**
> * uv_async 会自动合并（coalesce）

---

### 4️⃣ Consumer 侧：事件驱动读取（不 busy loop）

```c
static void algorithm_on_data(void *arg)
{
    datahub_t *hub = arg;
    PipeMsg msg;
    uint64_t seq = 0;

    while (shm_ring_peek(hub->ring, &msg, &seq) == 0) {
        // 处理数据
        process_msg(&msg);
    }
}
```

✔ 没数据时 **不会被调用**
✔ 有数据时 **批量 drain**
✔ cache / realtime / power 全赢

---

## 五、Algorithm 线程怎么接？（你关心的 500 表求和）

**注意：Algorithm 不是 uv_worker_pool，而是：**

> **Algorithm = 独立 uv_loop + DataHub subscriber**

### 正确形态

```c
void *algorithm_thread(void *arg)
{
    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_t *hub = arg;

    // hub->async 会把事件投递到这个 loop
    uv_run(&loop, UV_RUN_DEFAULT);
}
```

你得到的是：

* DataHub 写线程：485 / HPLC / Driver
* Algorithm 线程：被 async 唤醒
* 无锁
* 无 busy wait
* 无上下文污染

---

## 六、多进程版本（提前给你结论）

你后面一定会问，所以我直接给：

### 跨进程 DataHub = shm_ring + uv_pipe

```
Producer:
  shm_ring_push()
  write(pipe_fd, 1 byte)

Consumer:
  uv_read_start(pipe)
  -> drain shm_ring
```

> ⚠ 不用 eventfd、不用 futex、不用 pthread_cond
> **只用 libuv pipe**

等你说一句「进程版也补上」，我可以直接把 pipe 版 DataHub.c 给你。

---

## 七、一句话总结（架构级）

* **shm_ring**：数据一致性（What）
* **uv_async / uv_pipe**：调度通知（When）
* **libuv loop**：执行语义（Where）
* **DataHub**：把三者焊死成一个“事件化数据总线”

---
