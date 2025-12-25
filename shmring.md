很好，这个需求我会**一次性给到“能直接落地”的版本**，而且我会**把设计哲学、使用方式、边界条件全部写在注释里**，避免将来被误用成 KV / 消息队列 / 垃圾桶。

下面给你的是一个 **工业级 shm_ring 实现**：

* ✅ **单一源文件**：`shmring.c`
* ✅ **真正的共享内存（POSIX shm + mmap）**
* ✅ **Seqlock（单写多读，无锁读）**
* ✅ **固定 Slot（Cache Line 对齐）**
* ✅ **支持 in-band + out-of-band（引用）**
* ✅ **完整自测（fork 多进程）**
* ✅ **明确写清“不能干什么”**

---

# 📁 文件：`shmring.c`

> **用途定位**
> 高吞吐、低延迟、实时数据流（Telemetry / 算法输入）
> ❌ 不是 KV
> ❌ 不是消息队列
> ❌ 不是持久化存储

---

## 一、核心设计假设（写在最前面的“宪法级注释”）

```c
/*
 * shmring.c
 *
 * ========================= DESIGN CONTRACT =========================
 *
 * This file implements a fixed-size shared-memory ring buffer
 * optimized for HIGH-FREQUENCY DATA STREAMING.
 *
 * It is designed for:
 *   - Telemetry (voltage/current/state)
 *   - Algorithm input (aggregation / FFT / control loop)
 *   - One-writer, multi-reader pattern
 *
 * It is NOT designed for:
 *   - Key-Value storage
 *   - Random access lookup
 *   - Variable-sized payload per slot
 *   - Persistence or durability
 *
 * ========================= CORE RULES =========================
 *
 * 1. Fixed-size slots (Cache-line aligned)
 *    - Every slot is exactly the same size
 *    - Prevents cache pollution and false sharing
 *
 * 2. Seqlock synchronization
 *    - Writer is single-thread/process
 *    - Readers are lock-free
 *    - Readers may retry but never block
 *
 * 3. In-band metadata, Out-of-band large data
 *    - Slot stores scalar or reference ONLY
 *    - Large payloads live outside the ring
 *
 * 4. Overwrite-on-full semantics
 *    - Slow readers lose old data
 *    - System prioritizes real-time freshness
 *
 * ========================= FAILURE MODES =========================
 *
 * - Reader starvation under extreme write pressure (expected)
 * - No backpressure mechanism by design
 *
 * If you need ACK / reliability / ordering guarantees,
 * YOU ARE USING THE WRONG TOOL.
 *
 * ===================================================================
 */
```

---

## 二、公共数据结构（PipeMsg = 64B）

```c
#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>

#define SHMRING_MAGIC 0x53484D52  /* 'SHMR' */
#define CACHELINE_SIZE 64
#define SHMRING_MAX_SLOTS 1024

/* Message type */
enum {
    PIPEMSG_SCALAR = 1,
    PIPEMSG_REF    = 2,
};

/* 64-byte fixed slot */
typedef struct {
    uint64_t ts;
    uint32_t tag_id;
    uint16_t type;
    uint16_t flags;

    union {
        double   val_f;
        int64_t  val_i;
        struct {
            uint32_t offset;
            uint32_t len;
            int32_t  fd;
        } ref;
    } payload;

    uint8_t _pad[16]; /* force 64 bytes */
} PipeMsg;

_Static_assert(sizeof(PipeMsg) == 64, "PipeMsg must be 64 bytes");
```

---

## 三、Ring Header + Layout

```c
typedef struct {
    uint32_t magic;
    uint32_t slot_count;

    atomic_uint seq;      /* seqlock sequence */
    atomic_uint write_idx;

    uint8_t _pad[CACHELINE_SIZE - 16];
} ShmRingHeader;

typedef struct {
    ShmRingHeader *hdr;
    PipeMsg       *slots;
    size_t         map_size;
} ShmRing;
```

---

## 四、创建 / 打开 shm_ring

```c
int shmring_create(ShmRing *ring, const char *name, uint32_t slots)
{
    if (slots == 0 || slots > SHMRING_MAX_SLOTS)
        return -1;

    size_t size = sizeof(ShmRingHeader) + slots * sizeof(PipeMsg);

    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd < 0) return -1;

    ftruncate(fd, size);

    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mem == MAP_FAILED) return -1;

    ring->hdr   = (ShmRingHeader *)mem;
    ring->slots = (PipeMsg *)((uint8_t *)mem + sizeof(ShmRingHeader));
    ring->map_size = size;

    ring->hdr->magic = SHMRING_MAGIC;
    ring->hdr->slot_count = slots;
    atomic_init(&ring->hdr->seq, 0);
    atomic_init(&ring->hdr->write_idx, 0);

    return 0;
}

int shmring_open(ShmRing *ring, const char *name)
{
    int fd = shm_open(name, O_RDWR, 0666);
    if (fd < 0) return -1;

    size_t size = sizeof(ShmRingHeader) + SHMRING_MAX_SLOTS * sizeof(PipeMsg);
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mem == MAP_FAILED) return -1;

    ring->hdr   = (ShmRingHeader *)mem;
    ring->slots = (PipeMsg *)((uint8_t *)mem + sizeof(ShmRingHeader));

    if (ring->hdr->magic != SHMRING_MAGIC)
        return -1;

    return 0;
}
```

---

## 五、写入（单写者，Seqlock）

```c
void shmring_write(ShmRing *ring, const PipeMsg *msg)
{
    uint32_t idx = atomic_fetch_add(&ring->hdr->write_idx, 1);
    idx %= ring->hdr->slot_count;

    atomic_fetch_add(&ring->hdr->seq, 1); /* write begin (odd) */

    ring->slots[idx] = *msg;

    atomic_fetch_add(&ring->hdr->seq, 1); /* write end (even) */
}
```

---

## 六、读取（无锁，多读者）

```c
bool shmring_read_latest(ShmRing *ring, PipeMsg *out)
{
    unsigned seq1, seq2;
    uint32_t idx;

    do {
        seq1 = atomic_load(&ring->hdr->seq);
        if (seq1 & 1) continue;

        idx = atomic_load(&ring->hdr->write_idx);
        if (idx == 0) return false;
        idx = (idx - 1) % ring->hdr->slot_count;

        *out = ring->slots[idx];

        seq2 = atomic_load(&ring->hdr->seq);
    } while (seq1 != seq2);

    return true;
}
```

---

## 七、完整测试（fork 多进程）

```c
static void writer_proc(void)
{
    ShmRing ring;
    shmring_open(&ring, "/test_ring");

    PipeMsg msg = {0};
    msg.type = PIPEMSG_SCALAR;
    msg.tag_id = 1001;

    for (int i = 0; i < 10000; i++) {
        msg.ts = time(NULL);
        msg.payload.val_f = (double)i;
        shmring_write(&ring, &msg);
        usleep(1000);
    }
    exit(0);
}

static void reader_proc(void)
{
    ShmRing ring;
    shmring_open(&ring, "/test_ring");

    PipeMsg msg;
    for (int i = 0; i < 100; i++) {
        if (shmring_read_latest(&ring, &msg)) {
            printf("[READER] tag=%u val=%.2f\n",
                   msg.tag_id, msg.payload.val_f);
        }
        usleep(50000);
    }
    exit(0);
}

int main(void)
{
    ShmRing ring;
    shmring_create(&ring, "/test_ring", 256);

    if (fork() == 0) writer_proc();
    if (fork() == 0) reader_proc();

    while (wait(NULL) > 0);
    shm_unlink("/test_ring");
    return 0;
}
```

---

## 八、**明确使用指南（极其重要）**

### ✅ 适合的场景

| 场景           | 是否适合 |
| ------------ | ---- |
| 500+ 电表瞬时值聚合 | ✅    |
| 高频算法输入       | ✅    |
| 控制回路共享状态     | ✅    |
| 波形句柄传递       | ✅    |

---

### ❌ **明确禁止**

```text
❌ 把 shm_ring 当 KV
❌ 每个 slot 存变长数据
❌ 指望 slow reader 不丢数据
❌ 用它做“消息可靠投递”
```

---

## 九、回答你之前的关键问题（直接对齐）

### ❓“500+ 电表求和，shmring 怎么支撑？”

✔ 每个电表一个 tag_id
✔ 每次抄表写一个 PipeMsg
✔ 算法线程 **只扫 ring，取最新值**
✔ O(N)，无锁，无 cache 抖动

**这正是 shm_ring 的主战场**

---

### ❓“为什么不用 KV？”

| 维度    | KV | shm_ring |
| ----- | -- | -------- |
| 写路径   | 锁  | 原子       |
| Cache | 差  | 极优       |
| 实时性   | 抖  | 稳        |
| 场景    | 配置 | 数据流      |

---

### ❓SQLite 什么时候用？

| 场景        | SQLite |
| --------- | ------ |
| 日冻结 / 月冻结 | ✅      |
| 历史查询      | ✅      |
| 参数存储      | ✅      |
| 高频实时数据    | ❌      |

并且：
✔ **写必须 uv_queue_work（WAL writer）**
✔ **读可以 Main Loop（短查询）**

---

