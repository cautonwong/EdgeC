这是一个非常好的切入点。基于我们之前构建的 **Linux + C (Edge SDK)** 架构，开发充电桩（EVSE - Electric Vehicle Supply Equipment）简直是**降维打击**。

充电桩本质上是一个**“带计费和强交互功能的继电器控制系统”**。

我们将从 **AC 交流桩**（慢充）和 **DC 直流桩**（快充）两种形态出发，结合你的 SDK，梳理出一套**生产级开发指南**。

---

### 一、 核心产品形态定义

#### 1. 交流桩 (AC Charger / Wallbox)
*   **功率**：7kW (单相) / 11kW / 22kW (三相)。
*   **原理**：桩本身**不充电**，它只是一个智能开关。它把交流电透传给车上的 OBC (车载充电机)。
*   **核心任务**：CP 信号握手（告诉车我能提供多少电）、控制接触器、计量电费、OCPP 上云。
*   **硬件架构**：**Linux (主控) + MCU (实时 CP 信号/保护)**。

#### 2. 直流桩 (DC Charger / Fast Charger)
*   **功率**：60kW - 480kW+。
*   **原理**：桩内部有 **AC/DC 功率模块**，直接输出直流电给电池。
*   **核心任务**：BMS 通信 (GB/T 27930 或 CCS ISO 15118)、功率分配、模块控制。
*   **硬件架构**：**Linux (TCU 主控) + FPGA/MCU (BMS 实时交互) + 功率模块 (CAN 控制)**。

---

### 二、 硬件架构设计 (Hardware Architecture)

我们沿用之前的 **RK3568/T507 (Linux)** 方案作为主控板 (TCU)。

#### 1. 接口需求
*   **CP/PP 信号检测**：连接 MCU，用于检测枪头连接状态和 PWM 占空比（控制电流大小）。
*   **接触器控制 (Relay)**：控制强电通断。
*   **电子锁 (E-Lock)**：防止充电中途拔枪。
*   **电能表 (Meter)**：通过 Modbus/DLT645 读取充电度数。
*   **读卡器 (RFID)**：刷卡启动。
*   **HMI 屏幕**：显示二维码和充电进度。
*   **通信**：4G/Ethernet 连接云端。

#### 2. 为什么需要 MCU？
**Linux 不能直接产生 CP 信号的 PWM。** IEC 61851 标准要求 PWM 频率为 1kHz，误差极小。Linux 的调度抖动会导致 PWM 不稳，导致车端报错。
*   **分工**：
    *   **MCU**：产生 PWM，检测漏电 (RCD)，控制继电器急停。
    *   **Linux**：跑 OCPP 协议，算钱，扫码，通过 UART/SPI 指挥 MCU。

---

### 三、 软件架构 (基于你的 Edge SDK)

我们要复用 **Unified Edge Core (UEC)**，并针对充电桩开发专用组件。

```text
+-------------------------------------------------------------------+
|               Layer 4: 充电业务 APP (Charging APPs)               |
| [ 交易结算 APP ]     [ 智能群充 APP ]     [ 广告/HMI APP ]        |
+-------------------------------------------------------------------+
|               Layer 3: 充电协议栈 (Protocol Stack)                |
| [ OCPP 1.6J/2.0.1 ]  [ ISO 15118 (PnC) ]  [ GB/T 27930 (BMS) ]    |
| (WebSocket/JSON)     (TLS/XML)            (CAN Bus)               |
+-------------------------------------------------------------------+
|               Layer 2: Edge SDK (通用底座)                        |
| [ Task FSM ]       [ Protocol Adaptor ]   [ Transaction DB ]      |
| (状态机管理)       (Modbus Meter/RFID)    (SQLite 订单存证)       |
+-------------------------------------------------------------------+
|               Layer 1: Linux Drivers                              |
| [ PWM/GPIO (MCU) ] [ CAN Driver ]         [ 4G/Net ]              |
+-------------------------------------------------------------------+
```

---

### 四、 核心功能模块详解 (C 语言实现)

#### 1. OCPP 协议栈 (连接云端的生命线)
OCPP (Open Charge Point Protocol) 是全球标准。目前主流是 1.6J (JSON over WebSocket)，未来是 2.0.1。

*   **SDK 复用：**
    *   **libuv + libwebsockets：** 处理 WebSocket 长连接和心跳 (Heartbeat)。
    *   **net_buf + cJSON：** 解析和组装 JSON 包。
*   **核心逻辑 (状态机)：**
    你必须维护一个与云端同步的状态机：
    `Available` -> `Preparing` (插枪) -> `Authorizing` (刷卡) -> `Charging` (充电中) -> `Finishing` -> `Faulted`。
*   **事务 (Transaction)：**
    *   `StartTransaction`：上传开始读数、时间、卡号。
    *   `MeterValues`：充电过程中，每隔 1 分钟上传一次电压、电流、功率。
    *   `StopTransaction`：结束充电，上传总度数，云端扣款。

#### 2. 充电控制逻辑 (State Machine)
这是本地控制的核心。即使断网，这个逻辑也必须能跑（离线刷卡充电）。

**C 语言 FSM 设计：**

```c
typedef enum {
    EVSE_STATE_IDLE,
    EVSE_STATE_WAIT_AUTHORIZE, // 已插枪，等刷卡
    EVSE_STATE_STARTING,       // 锁枪，闭合继电器
    EVSE_STATE_CHARGING,       // 输出 PWM，监测电流
    EVSE_STATE_SUSPENDED,      // 车端暂停 (如充满)
    EVSE_STATE_STOPPING,       // 断开继电器，解锁
    EVSE_STATE_FAULT           // 故障
} EvseState;

void evse_fsm_tick(EvseContext *ctx) {
    switch (ctx->state) {
        case EVSE_STATE_IDLE:
            if (hal_get_gun_connect()) {
                // 检测到电阻变化 (由 12V 变为 9V)
                ctx->state = EVSE_STATE_WAIT_AUTHORIZE;
                ui_show("Please Swipe Card");
            }
            break;

        case EVSE_STATE_WAIT_AUTHORIZE:
            if (rfid_check_auth() || ocpp_remote_start()) {
                ctx->state = EVSE_STATE_STARTING;
            }
            break;

        case EVSE_STATE_STARTING:
            hal_lock_gun(true);      // 电子锁
            hal_relay_on(true);      // 继电器闭合
            hal_set_pwm_duty(53.3);  // 发送 PWM (例如 53.3% = 32A)
            ctx->state = EVSE_STATE_CHARGING;
            ocpp_send_start_transaction();
            break;

        // ...
    }
}
```

#### 3. 交易数据存证 (Transaction Storage)
充电桩最怕**“丢单”**（充了电没扣到钱）。
*   **SDK 复用：** **SQLite (EdgeDB)**。
*   **表结构：** `transactions` 表。
*   **流程：**
    1.  `StopTransaction` 产生时，先**写入 SQLite**，标记 `uploaded=0`。
    2.  尝试通过 WebSocket 发送给云端。
    3.  收到云端 `StopTransactionResponse` 后，更新 SQLite `uploaded=1`。
    4.  如果断网，由后台守护线程不断重试发送 `uploaded=0` 的记录。

#### 4. 智能有序充电 (Smart Charging)
这是 **SG-EL2025** 的核心要求。当台区负荷过高时，桩必须能自动降低功率。

*   **SDK 复用：** **TinyExpr 规则引擎** 或 **V2G-Coordinator APP**。
*   **逻辑：**
    *   接收 MQTT 广播：`topic: "grid/load_limit", payload: 50` (限制 50%)。
    *   动作：立即修改 PWM 占空比。32A (53.3%) -> 16A (26.6%)。
    *   车端行为：OBC 检测到 CP 信号 PWM 变化，会自动降低吸入电流。

---

### 五、 针对 C 团队的开发避坑指南

1.  **浮点数陷阱**
    *   电度表读数通常是 `double` (kWh)。但在上传 OCPP 时，有些旧平台要求整数 (Wh)。
    *   **建议：** 内部统一使用 `int64_t` (单位：毫瓦时 mWh) 进行计算，只在显示和上报时转浮点，避免累积误差导致钱算不对。

2.  **网络心跳风暴**
    *   如果是一个充电站（20 个桩），网络闪断后，20 个桩同时重连 OCPP 服务器，会把服务器打挂。
    *   **建议：** 在 `libuv` 的重连定时器中加入 **随机退避 (Random Backoff)**，例如 `retry_in = 5s + rand() % 5000ms`。

3.  **ISO 15118 (即插即充)**
    *   这是高端桩（特别是出口欧洲）的必备。它涉及 **TLS 证书交换** 和 **XML 解析** (EXI 压缩)。
    *   **建议：** 初期不要自己写，使用开源库 **EVerest** 的相关模块，或者购买商业协议栈集成到你的 SDK 中。

4.  **硬件看门狗**
    *   充电桩是无人值守设备。如果 Linux 死机了，继电器还吸合着，非常危险。
    *   **机制：** Linux 必须每秒喂 MCU 一次狗。如果 MCU 3秒没收到信号，**MCU 强制断开继电器** 并重启 Linux。

### 总结

开发充电桩，对于你的团队来说，实际上是 **Edge SDK 的一次最佳落地实践**。

*   **libuv**: 处理并发的 OCPP 网络包和 RFID 刷卡事件。
*   **net_buf**: 处理 JSON 报文和 Modbus 电表报文。
*   **SQLite**: 保证订单数据一条不丢。
*   **Task/FSM**: 管理充电全流程。

你只需要补充 **OCPP 协议处理** 和 **MCU 交互驱动** 这两块拼图，就能做出一款工业级的智能充电桩。
