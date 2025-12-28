**集中器 (Data Concentrator Unit, DCU)** 是智能电网架构中的“腰部”核心设备。

与普通的 IoT 网关不同，集中器的核心职责是 **“Store-and-Forward” (存储转发)**：它不是简单的透传数据，而是主动把几百个电表的数据抄回来，存在本地，整理好，等待主站（HES）来取。

针对 **DLMS 集中器**，结合你的 **Linux + C (Edge SDK)** 架构，这是一套**硬核**的生产级实施方案。

---

### 一、 核心架构：双面 DLMS 引擎 (Dual-Face DLMS Engine)

集中器在逻辑上必须同时扮演两个角色：
1.  **对下 (Southbound):** 它是 **DLMS Client**。负责轮询、管理 500+ 个子电表。
2.  **对上 (Northbound):** 它是 **DLMS Server**。主站把集中器看作一个“超级电表”，读取集中器里的数据。

```text
+-----------------------------------------------------------------------+
|                 Layer 3: 集中器业务逻辑 (DCU Logic)                    |
| [ 自动抄表调度 ]    [ 拓扑管理 ]    [ 档案管理 ]    [ 事件上报 ]       |
+-----------------------------------------------------------------------+
|                 Layer 2: 虚拟对象层 (Virtual Objects)                  |
| [ COSEM Server (North) ] <--- 映射 ---> [ SQLite (Data Center) ]      |
| (模拟 Profile Generic)                  (存储 500 个表的冻结数据)      |
+-----------------------------------------------------------------------+
|                 Layer 1: 协议适配层 (Protocol Adaptor)                 |
| [ DLMS Server Stack ]                   [ DLMS Client Stack ]         |
| (响应主站 HES)                          (Gurux Polling Engine)        |
+-----------------------------------------------------------------------+
           ^                                         ^
           | (4G/Ethernet)                           | (HPLC/RS485/RF)
      [ 主站 HES ]                              [ 500x 智能电表 ]
```

---

### 二、 核心模块详解与 C 实现建议

#### 1. 下行采集引擎 (The Polling Engine)

这是最累的模块。它需要维护一个 **任务队列**，通过 HPLC (载波) 或 RS485 挨个去读表。

*   **SDK 复用:** `Task Scheduler` + `Gurux.DLMS.c (Client Mode)`.
*   **并发策略:**
    *   如果是 **RS485**：必须串行轮询 (Serial Polling)。
    *   如果是 **HPLC (宽带载波)**：HPLC 模块通常支持并发抄读（集中器发一个指令给 CCO，CCO 搞定一切）。你的 C 代码只需发送“抄读列表”指令。

**数据落地 (SQLite):**
集中器最大的挑战是**数据量**。500 个表，每 15 分钟存一次“负荷曲线 (Load Profile)”。

```sql
-- 电表档案表
CREATE TABLE meters (
    meter_id INTEGER PRIMARY KEY,
    logic_addr TEXT UNIQUE, -- e.g., "123456789012"
    comm_port INTEGER,      -- 0=RS485-1, 1=HPLC
    protocol_type INTEGER   -- 0=DLMS, 1=Modbus
);

-- 曲线数据表 (Profile Data)
-- 注意：这里使用 blob 存储 buffer，或者按 OBIS 分列
CREATE TABLE load_profile (
    ts INTEGER,             -- 时间戳
    meter_id INTEGER,       -- 关联 meters
    obis_code TEXT,         -- e.g. "1.0.99.1.0.255"
    value REAL,
    status INTEGER,
    PRIMARY KEY (ts, meter_id, obis_code)
);
```

#### 2. 上行响应引擎 (The Virtual Server)

这是集中器区别于网关的关键。当主站读取集中器时，**集中器不应该再去读表**，而是直接把 SQLite 里的数据吐给主站。

*   **技术核心:** 使用 `Gurux.DLMS.c` 的 **Server Mode**。
*   **对象模拟 (The "Profile Generic" Trick):**
    DLMS 中有一个核心对象叫 `Profile Generic` (Class 7)。它相当于一个数据库视图。
    *   主站请求：`Get Profile Generic (1.0.99.1.0.255) where date between T1 and T2`。
    *   **你的 C 代码行为:**
        1.  解析请求的 `range_descriptor` (时间范围)。
        2.  生成 SQL: `SELECT * FROM load_profile WHERE ts BETWEEN T1 AND T2`.
        3.  将 SQLite 结果集打包成 DLMS Array 格式。
        4.  通过 `net_buf` 零拷贝发送给主站。

**实现伪代码:**

```c
// DLMS Server 回调：当主站读数据时触发
void on_read_request(gxServer *server, gxObject *obj, int attribute_index) {
    if (obj->objectType == GX_DLMS_OBJECT_TYPE_PROFILE_GENERIC) {
        // 1. 获取查询条件 (Buffer)
        buffer_struct *capture_buffer = &obj->buffer;

        // 2. 从 SQLite 查询数据
        // 这通常是一个耗时操作，必须异步处理！
        // 在实际 SDK 中，这里应该挂起请求，扔给 Worker 线程，
        // 待查完后通过 server_send_response 发回。

        DlmsDataRows *rows = db_query_profile(start_time, end_time);

        // 3. 序列化为 DLMS 格式
        gx_server_send_rows(server, rows);
    }
}
```

#### 3. 档案与拓扑管理 (Topology Management)

集中器必须知道“谁在下面”。

*   **自动发现 (Auto-Discovery):**
    *   **HPLC 场景:** CCO 模块会自动上报新加入的电表 MAC 地址。你的 SDK 需要监听 CCO 的事件报文，解析出电表地址，自动在 `meters` 表中创建记录。
    *   **白名单机制:** 很多 utility 要求，只有主站下发了档案的表才允许抄读。你需要实现一个 `Import Meter List` 的功能 (通常通过 DLMS `File Transfer` 对象下发)。

---

### 三、 集中器的三大高级特性 (Advanced Features)

要做到生产级，必须解决以下痛点：

#### 1. 补抄策略 (Retry Strategy)
电表通信成功率通常只有 95%-98%。
*   **逻辑:**
    1.  **正常抄读:** 00:00 发起全量抄读。
    2.  **一级补抄:** 00:30 查询 SQLite，找出 `status=MISSING` 的表，生成补抄队列。
    3.  **重点补抄:** 07:00 还没抄到的，提高优先级，甚至切断其他非核心业务，专心抄它。

#### 2. 事件上报 (Event Notification)
*   **场景:** 某个电表盖子被打开了 (Tamper)。
*   **流程:**
    1.  电表主动上报 (Push) 或 集中器轮询时发现 `Alarm Register` 变位。
    2.  集中器记录事件到 `events` 表。
    3.  集中器作为 Client，向主站发起 **Notification (Push)** 请求。
*   **注意:** 主站可能不在线。需要 **Queueing** 机制，等主站连上后批量补推。

#### 3. 透明通道 (Transparent Tunneling)
主站有时候想“穿透”集中器，直接配置某个电表的参数。
*   **DLMS 实现:** 使用 `Image Transfer` 或自定义的 `Tunnel` 对象。
*   **C 实现:**
    收到主站的 payload -> 查找 `meter_id` -> 封装成下行报文 -> 发给电表 -> 收到回复 -> 剥离 -> 发回主站。

---

### 四、 研发难点与避坑

1.  **内存爆炸**
    *   DLMS 的 `Profile Generic` 响应报文可能极大 (几百 KB)。
    *   **方案:** 必须支持 **Block Transfer (分块传输)**。`Gurux` 支持此功能，但你需要配置好 `Max PDU Size`。不要试图 malloc 一个 1MB 的 buffer，使用分片流式发送。

2.  **校时风暴**
    *   主站给集中器校时，集中器要给 500 个电表校时。
    *   **避坑:** 不要广播校时（容易失败且无法确认）。采用 **分批单点校时**，或者利用 HPLC 底层的广播校时能力。

3.  **安全性 (HLS)**
    *   集中器是密钥管理的重灾区。它存了 500 个表的密码。
    *   **方案:** 必须使用 **SE 安全芯片** 存储 Master Key。SQLite 中的电表密码字段必须是加密存储的（Key Wrapping）。

### 总结

基于你的 **Edge SDK** 开发 DLMS 集中器：

1.  **SQLite** 是核心中的核心，它充当了主站和电表之间的“缓冲池”。
2.  **Gurux** 要用两份：一份做 Client (抄表)，一份做 Server (应答)。
3.  **Task Scheduler** 负责复杂的补抄逻辑。
4.  **net_buf** 负责处理庞大的分块传输数据。

这是一个标准化的、高价值的工业级产品形态。
