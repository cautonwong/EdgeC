这是一份针对 **DLMS** 协议栈的架构设计说明书（Design Specification）。

该文档旨在指导开发团队在 **EdgeOS** 基础设施（libuv, net_buf, Contract, OSAL）之上，构建一套超越 Gurux 的、工业级、高性能、零拷贝的 DLMS/COSEM 协议栈。

---

# 📘 DLMS 协议栈架构设计与实施规范 (v1.0)

**文档状态**：已冻结 (Frozen)
**适用范围**：EdgeOS 核心开发组（驱动层、协议层）
**目标架构**：Linux 网关 (Master) & MCU 终端 (Slave)

---

## 1. 设计哲学与核心原则 (Design Philosophy)

DLMS 的设计目标是解决传统协议栈（如 Gurux）在嵌入式环境中的 **内存碎片化、阻塞式 I/O、对象模型臃肿** 三大痛点。

1.  **零动态内存 (No-Malloc in Hot Path)**
    *   严禁在协议解析和组包过程中调用 `malloc/free`。
    *   **机制**：利用 `net_buf`（Slab Allocator）进行头部预留（Headroom）和引用计数管理。对于大数据（如固件、波形），传递物理内存引用（DMA/SHM），而非拷贝数据。
2.  **元数据驱动 (Contract-Driven)**
    *   严禁在代码中硬编码 OBIS 码、类 ID 或属性 ID。
    *   **机制**：所有对象字典由 `contract_compiler` 静态生成。运行时通过 `TagID` 查静态常量表（O(1) 或 O(logN)），消除字符串匹配开销。
3.  **全异步事件驱动 (Event-Driven FSM)**
    *   严禁使用阻塞式 `read/write` 或 `sleep`。
    *   **机制**：基于 `libuv` 的 Reactor 模型。协议栈被设计为 **无阻塞状态机 (FSM)**，由外部 IO 事件或定时器推动状态流转。
4.  **安全底座硬化 (Hardware Security)**
    *   严禁在软件中保留明文私钥。
    *   **机制**：协议栈的加解密层直接对接 **SE (Secure Element) HAL**，实现国密/国际算法的硬件加速与密钥隔离。

---

## 2. 功能特性矩阵 (Feature Set)

DLMS 必须实现以下全量特性，以满足国网/南网及海外 AMI 需求。

### 2.1 链路层 (Data Link Layer)
*   **HDLC 协议**：
    *   支持 IEC 62056-21 Mode E 握手。
    *   支持 `SNRM/UA` 链路建立与断开。
    *   支持 `N(S)/N(R)` 滑动窗口与重传机制 (LLC)。
    *   支持 物理层分帧与重组 (Segmentation & Reassembly)。
*   **TCP/UDP Wrapper**：
    *   支持以太网/4G 场景下的报文封装（IEC 62056-47）。

### 2.2 应用层 (Application Layer)
*   **寻址模式**：同时支持 **逻辑名 (LN)** 和 **短名 (SN)** 引用。
*   **关联控制 (ACSE)**：
    *   支持 `AARQ` / `AARE` 关联建立。
    *   支持 `RLRQ` / `RLRE` 关联释放。
    *   支持 **上下文协商**（Max PDU Size, Conformance Block）。
*   **数据服务**：
    *   `GET` (Normal, Next, With-List)。
    *   `SET` (Normal, With-List)。
    *   `ACTION` (Method Invocation)。
*   **大数据传输**：
    *   **Classic Block Transfer**：基于 `GET-Response-Next` 的分块机制。
    *   **General Block Transfer (GBT)**：基于窗口流控的高速传输（用于 OTA）。
    *   **Selective Access**：支持按时间范围或列索引读取 `Profile Generic`。

### 2.3 安全套件 (Security Suites)
*   **无安全 (None)**：明文传输。
*   **低级安全 (LLS)**：密码认证。
*   **高级安全 (HLS)**：支持 4 次握手认证 (StoC, CtoS)。
*   **加密套件**：
    *   Suite 0 (AES-GCM-128).
    *   Suite 1 (ECDSA P-256 + SHA-256 + AES-GCM-128).
    *   **国密特供** (SM2 + SM3 + SM4-GCM).
*   **数据保护**：支持 `Global Ciphering` (APDU 全文加密) 和 `Dedicated Ciphering`。

---

## 3. 总体架构与分层 (Architecture Layers)

```text
[ Business Logic / APP ]
       ^
       | (TagID / DataPoint)
-------+-----------------------------------------------------
[ Service Layer ] (会话管理)
   - Connection Manager (关联状态机)
   - Block Transfer Engine (分块上下文管理)
       ^
       | (APDU Object)
-------+-----------------------------------------------------
[ Adapter Layer ] (编解码)
   - Builder (构建者模式: 正序写入+长度回填)
   - Streaming Parser (SAX模式: 流式解析)
       ^
       | (APDU Stream)
-------+-----------------------------------------------------
[ Security Layer ] (加解密)
   - Ciphering Engine (In-place Encryption)
   - SE HAL Driver (硬件卸载)
       ^
       | (Ciphered APDU)
-------+-----------------------------------------------------
[ DataLink Layer ] (传输保障)
   - HDLC FSM (分帧/重传/窗口)
   - Wrapper (TCP Header)
       ^
       | (Raw Bytes)
-------+-----------------------------------------------------
[ Physical Layer ]
   - Bus Arbiter (总线仲裁)
   - IO Executor (poll/write/read)
```

---

## 4. 关键机制设计 (Core Mechanisms)

### 4.1 双层分片机制 (Dual-Segmentation)

必须明确区分链路层分帧与应用层分块，两者独立运作。

1.  **HDLC 分帧 (Physical Segmentation)**：
    *   **触发条件**：APDU 长度 > 物理层 MTU (如 128字节)。
    *   **处理**：链路层自动切片发送，接收端自动缓存并在收到 `Final` 帧时合并。**对上层透明**。
2.  **COSEM 分块 (Logical Blocking)**：
    *   **触发条件**：业务数据长度 > 协商的 Max PDU Size (如 1024字节)。
    *   **处理**：业务层状态机介入。
        *   **Client**：发送 `Get-Request-Next` 循环索取。
        *   **Server**：维护 `Block Context`，记录 Offset，分批发送数据引用。

### 4.2 构建者模式 (The Builder)

解决 DLMS 变长长度编码（BER）与嵌套结构（Struct/Array）的构建难题。

*   **设计**：使用 `Stack` 记录容器的头部位置。
*   **操作流**：
    1.  `builder_open_array()`: 写入 Tag，预留 1 字节长度位，压栈当前指针。
    2.  `builder_put_xxx()`: 写入内容。
    3.  `builder_close_array()`: 计算内容长度 `L`。
        *   若 `L < 128`: 回填到预留位。
        *   若 `L >= 128`: 调用 `net_buf_expand` (memmove) 腾出空间，写入多字节长度。

### 4.3 资源生命周期管理 (Resource Lifecycle)

防止内存泄漏的核心机制。

*   **引用计数**：对于 Request 中的 Payload，Server 在处理期间必须 `net_buf_ref()`。
*   **会话看门狗 (Session Watchdog)**：
    *   Server 端在处理分块传输时，启动 `Inactivity Timer`。
    *   若主站超过 T 时间（如 60s）未请求下一块，**强制销毁上下文并释放资源引用**。
*   **异常清理**：
    *   收到 `RLRQ`（断开）或 链路层 `Disconnect` 信号时，立即级联销毁所有应用层上下文。

### 4.4 安全层零拷贝 (Zero-Copy Security)

*   **原地加密 (In-Place)**：
    *   Security Layer 利用 `net_buf` 的 `headroom` 预留 Security Header 空间。
    *   调用 SE HAL 时，传入 payload 指针，直接在原内存区域覆盖密文，不分配新 buffer。

---

## 5. 详细流程设计 (Workflows)

### 5.1 主站采集流程 (Master / Client)

1.  **Job 派发**：`Scheduler` 生成采集任务 (TagID)，`Arbiter` 调度给驱动。
2.  **契约查找**：驱动调用 `lookup_map(TagID)` 获取 OBIS、Class、Attr。
3.  **连接检查**：
    *   若 `State != ASSOCIATED`，暂停数据任务，插入 `Connect` 任务。
    *   执行 `SNRM -> UA -> AARQ -> AARE -> HLS` 握手。
4.  **请求构建**：使用 `Builder` 构建 `GET-Request`。
5.  **链路发送**：压入 HDLC 队列，处理重传。
6.  **响应处理**：
    *   收到数据 -> `Streaming Parser` 解析。
    *   若为 `Block` -> 循环发送 `Get-Next`，数据流式推入 Data Pipeline。
    *   若为 `Data` -> 解析为 `DataPoint`，写入 `shm_ring`。
7.  **总线释放**：通知 Arbiter 任务完成。

### 5.2 从站响应流程 (Slave / Server)

1.  **被动唤醒**：`uv_poll` 收到数据。
2.  **链路重组**：HDLC 层处理分帧，输出完整 APDU。
3.  **安全解密**：Security 层调用 SE 解密，输出明文 APDU。
4.  **路由分发**：
    *   解析 OBIS。
    *   调用 `slave_lookup(OBIS)` 获取内部数据指针（指向全局结构体或 SHM）。
5.  **响应构建**：
    *   **小数据**：直接构建响应。
    *   **大数据**：创建 `Block Context`，锁定数据源引用，发送第一块。
6.  **发送**：加密 -> HDLC 切片 -> 物理发送。

---

## 6. 接口与集成 (Integration)

### 6.1 外部依赖接口 (HAL)
协议栈不直接调用 OS API，只调用 HAL。
*   `io_send(data, len)` / `io_recv_cb` (物理层)
*   `se_encrypt(...)` / `se_sign(...)` (安全层)
*   `os_get_time()` / `os_timer_start()` (基础层)

### 6.2 与 EdgeOS 的结合点
*   **配置**：通过 `contract_compiler` 生成的 `contracts.c` 是协议栈运行的唯一数据源。
*   **并发**：协议栈是一个纯逻辑对象（Context），运行在 `IO Executor` 线程中，受 `Arbiter` 调度。

---

## 7. 验收标准 (Acceptance Criteria)

1.  **一致性**：通过官方 CTT (Conformance Test Tool) 测试。
2.  **稳定性**：在 9600bps 高误码率环境下，HDLC 重传机制能保证数据最终到达，且内存无泄漏。
3.  **性能**：处理 1KB 负荷曲线数据，内存峰值增加 < 2KB（流式处理证明）。
4.  **兼容性**：同一套代码，通过不同编译参数，既能作为网关采集电表，也能作为电表被网关采集。



## device

Excellent. Based on our discussion, here is a complete and executable plan to create the extensible, contract-aware device_simulator.

  I will structure the work into two main phases to deliver value incrementally.

  Phase 1: Build the Simulator Framework

  The goal of this phase is to create a working, multi-protocol simulator application that uses the existing, simple mocked responses already present in the drivers.

   1. Create Application Skeleton:
       * Create a new directory: cmd/device_simulator.
       * Inside, create a CMakeLists.txt to define the device_simulator executable and link it with the necessary project libraries.
       * Add this new directory to the main CMakeLists.txt so it gets built.

   2. Implement the Generic Main Loop (`cmd/device_simulator/main.c`):
       * The main function will parse command-line arguments: --protocol <name>, --port <path>.
       * It will initialize the driver_manager, register all available protocol drivers (DLT-645, Modbus, etc.), and load the one specified on the command line.
       * It will open the specified port (e.g., /tmp/vport-slave) and enter a libuv event loop.
       * The loop will read data from the port, pass it to the selected driver's existing handle_rx function, and write the hard-coded response back to the port.

  Phase 2: Implement Contract-Aware Data Simulation

  The goal of this phase is to make the simulator "intelligent" by having it generate responses based on the project's contracts, as you suggested.

   1. Create a "Simulated Device State" Module:
       * I will create new files, src/core/sim_device.c and include/sim_device.h.
       * This module will have a simple API to initialize from a product's contracts, and then get/set data values using the generic TagID.
       * At startup, it will parse the contract files and create an in-memory database of all data points with default values.

   2. Upgrade the DLT-645 Driver:
       * I will enhance the dlt645_v3_handle_rx function.
       * Instead of returning a hard-coded value, it will decode the Data ID from the request, look up the corresponding generic TagID, query the "Simulated Device State" module for the current value, and encode that real value into the response.

   3. Update the Simulator's Main function:
       * I will add a --product <name> command-line argument.
       * The main function will now call sim_device_state_init() with the specified product name before starting the main loop.

  Final Deliverable

  The final result will be a device_simulator executable that can be run like this:

  ./build/products/device_simulator/bin/device_simulator --product iems --protocol DLT645-2007 --port /tmp/vport-slave

  It will realistically simulate the iems product's DLT-645 interface, communicating over a virtual serial port. This provides a powerful, automated integration testing tool.

  contract-compiler 生成的contracts中  每个协议的lookup函数应该至少有三个,比如:
  1. edge_lookup_dlt645(uint32_t )
  2. edge_lookup_dlt645_by_buf(uint8_t id[])
  3. edge_lookup_dlt645_by_string(char name[] )
  2和3 将参数转换一下然后直接调用1
 且要注意 不同的协议  这个函数的参数是不一样的  比如DLMS有classID+OBIS+attrID