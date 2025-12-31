## 给此项目推荐一个优秀的日志系统  符合linux边缘计算设备的最佳实践

鉴于你的 **EdgeOS SDK** 是基于 **C 语言 + libuv + 嵌入式 Linux** 的高性能工业级架构，且涉及 **HEMS (低资源)** 到 **SG-EL2025 (高性能)** 的跨度，我毫不犹豫地推荐：

### 🏆 核心推荐：**zlog**

**GitHub**: [HardySimpson/zlog](https://github.com/HardySimpson/zlog) (LGPL-2.1)

它是 C 语言界**最接近 Java Log4j** 的日志库，在中国的工业网关、电力终端领域有极高的占有率（久经考验）。

---

### 一、 为什么 zlog 是 EdgeOS 的绝配？

1.  **高性能与线程安全**：
    *   基于 C 编写，无 C++ 依赖（符合你的密封构建策略）。
    *   内部实现了高效的缓冲机制和线程锁，多线程并发写（Worker Threads + Main Loop）不会崩。
2.  **Flash 寿命友好（关键）**：
    *   **Log Rotation (日志轮转)**：原生支持按大小（如 10MB）和按日期切分。
    *   **自动清理**：支持只保留最近 N 个文件，自动删除旧日志。这对防止 `/var/log` 撑爆至关重要。
3.  **运行时热重载**：
    *   支持 `zlog_reload()`。你可以在不重启进程的情况下，通过修改配置文件，动态开启/关闭某些模块的 Debug 级别。这完美契合你的 **L1 Main Loop 配置热更** 机制。
4.  **分类明确**：
    *   支持 Category（分类）。你可以为 `Driver`, `Pipeline`, `Storage` 定义不同的日志策略（例如：Driver 写详细点，Storage 只写 Error）。

---

### 二、 针对 EdgeOS 的工程化集成方案

不要直接在业务代码里调 `zlog_info`。我们需要一层 **“防腐层封装”**，并结合你的 **libuv 架构** 进行优化。

#### 1. 封装层 (`include/edge_log.h`)

```c
#ifndef EDGE_LOG_H
#define EDGE_LOG_H

#include "zlog.h"

// 定义日志分类 (Category)
typedef zlog_category_t edge_logger_t;

// 获取日志句柄
// name: "driver.modbus", "core.sched", "app.iems"
edge_logger_t* edge_log_get(const char *name);

// 初始化 (在 Main Loop 启动前调用)
int edge_log_init(const char *conf_path);
void edge_log_fini(void);

// 动态修改级别 (运维特性)
void edge_log_set_level(const char *cat_name, int level);

// 宏封装 (保留文件名和行号)
#define log_info(cat, format, ...) \
    zlog_info(cat, format, ##__VA_ARGS__)

#define log_error(cat, format, ...) \
    zlog_error(cat, format, ##__VA_ARGS__)

#define log_debug(cat, format, ...) \
    zlog_debug(cat, format, ##__VA_ARGS__)

#endif
```

#### 2. Flash 保护配置 (`config/zlog.conf`)

这是**工业级配置**的精髓。我们需要将日志分为 **内存缓冲** 和 **持久化文件**。

```ini
[global]
strict init = true
buffer min = 1024
buffer max = 2MB  # 内存缓冲，防止频繁 IO 唤醒 Flash
rotate lock file = /tmp/zlog.lock

[formats]
# 工业标准格式: 时间 | 级别 | 线程ID | 模块 | 消息
simple = "%d(%m-%d %T) %-5V [%t] %c: %m%n"

[rules]
# 1. 默认策略：所有 INFO 以上日志存入文件
#    每个文件 10MB，最多保留 5 个 (共 50MB)，防止写满磁盘
*.*   "/var/log/edge/sys.log", 10MB * 5 ~ "sys.log.#r"; simple

# 2. 调试策略：控制台只输出 WARN 以上 (开发模式下可改为 DEBUG)
*.*   >stdout; simple

# 3. 关键业务分流：计费日志单独存 (EVSE 场景)
evse.billing.INFO  "/var/log/edge/billing.log", 1MB * 10; simple

# 4. 垃圾过滤：屏蔽过于频繁的底层驱动 Debug
driver.modbus.DEBUG  "/dev/null"
```

---

### 三、 极致性能优化：异步日志 (Async Logging)

在 **IEMS** 和 **SG-EL2025** 场景下，如果在 **Stream Engine Loop (L2)** 里直接写日志，可能会因为磁盘 IO 阻塞导致 Pipeline 抖动。

虽然 zlog 很快，但为了**极致的确定性**，我们建议引入 **“日志专列”**。

#### 方案：基于 `shm_ring` 的日志旁路

如果日志量极大（如开启了全量 Debug），不要让业务线程写磁盘。

1.  **架构**：
    ```text
    [ L2 Stream ] / [ T1 Driver ]
           |
           v (log_debug)
    [ Log Frontend ]
           | (格式化为字符串)
           v
    [ Log RingBuffer ] (非阻塞写入)
           |
    +------+----------------------+
    |      L4: Log Worker         | <--- 新增一个专门写日志的低优线程
    +-----------------------------+
           | (Batch Read)
           v
    [ zlog Backend ] ---> [ Disk ]
    ```

2.  **代码实现思路**：
    *   复用你的 `edge_shm_ring`。
    *   定义 `EDGE_LOG_RING_SIZE` (如 1MB)。
    *   `edge_log_init` 时启动一个 `uv_thread` 专门消费这个 Ring，调用 `zlog` 写入。
    *   如果 Ring 满了？**直接丢弃日志**。保证业务逻辑（采集、控制）永远不会因为日志卡死。

---

### 四、 针对五大产品的策略配置

| 产品 | 推荐策略 | 配置要点 |
| :--- | :--- | :--- |
| **HEMS** | **同步写 + 小文件** | 内存小，直接写文件。`1MB * 2`，避免占满 Flash。 |
| **IEMS** | **异步写 (Ring)** | 开启异步日志线程。日志级别默认 `WARN`，故障时通过云端下发指令改为 `DEBUG`。 |
| **SG-EL2025** | **审计分离** | `security.*` 和 `audit.*` 单独存文件，且不能被 rotate 删除（或保留更久）。 |
| **EVSE** | **关键路径落盘** | 计费日志配置 `fsync` 策略（zlog 支持），确保掉电不丢日志。 |
| **DCU** | **RAM Log** | 因为挂载点多，日志量大。建议平时日志写在 `/tmp` (内存)，仅严重错误写 Flash。 |

### 五、 总结

1.  **选型**：**zlog** 是 C 语言工业级开发的最佳选择。
2.  **集成**：必须配合 **Log Rotation** 配置文件，保护 Flash 寿命。
3.  **进阶**：对于高性能场景，复用你的 **RingBuffer** 实现**异步日志**。
4.  **规范**：严禁使用 `printf`，全员使用 `log_xxx(ctx->logger, ...)`。

## 任务

1. zlog的源码在/workspaces/edgeos/EdgeC/build/zlog-1.2.18.tar.gz，并按照.local目录下的规范 就地构建静态库(两个平台的)并安装到 `.local` 中
2. 研究zlog的使用最佳使用实践  构思和设计如何用到我们这个项目中来