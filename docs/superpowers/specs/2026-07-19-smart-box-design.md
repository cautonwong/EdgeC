# 智能盒子 (Smart Box) 全功能软硬件与 AI 协同详细设计规格书

> **产品代号**: SmartBox-Alpha  
> **固件基线**: EdgeOS V2.1.0-AB  
> **设计基准**: 适老化、健康闭环与强实时安全监护  
> **所有者**: Mr. WKT  
> **密级**: 生产级绝密

---

## 一、 产品概述与业务闭环 (Product Overview)

智能盒子是一款集成多协议接入、边缘计算与 AI 协同能力的智能中枢设备，专注于健康管理、康复训练及适老化服务。系统打通了**用户（老人/患者）**、**家属（远程看护）**、**医护人员（临床康复）**与**管理员（批量运维）**四端权限隔离的业务闭环：

1. **用户端 (语音UI & 适老终端)**：提供大字体控制台、跌倒/心率安全报警、离线语音指令触发、AI 虚拟陪伴与金币流量兑换。
2. **家属端 (远程小程序/MQTT推送)**：接收系统主动上报的安全报警（跌倒、心率异常）、查看健康报告与日/周运动积分、远程上传照片视频共享“家庭时光”电子相册。
3. **医生/医护端 (临床工作站/REST API)**：具备较高安全权限，调取数据库脱敏后的医疗趋势分析记录，为患者开具/同步个性化康复计划。
4. **管理员端 (批量运维/安全认证)**：负责设备证书授权（NFC/蓝牙证书）、固件 A/B 静默分区升级、远程日志抓取以及紧急恢复重灌。

---

## 二、 系统架构与双轨通信平面 (System Architecture)

系统采用 **C 底座 (强实时控制) + Go 网关 (复杂业务与 AI 桥接)** 的混合多进程架构，实现物理与逻辑的安全失效隔离。

```
                     ┌───────────────────────────────────┐
                     │     Command Center Vue UI HMI     │
                     └─────────────────┬─────────────────┘
                                       │ (HTTP REST / SSE 实时流)
                                       ▼
                     ┌───────────────────────────────────┐
                     │ Go 网关 (smart-box-gateway)       │
                     │   - SQLite 加密存储 (AES-256)      │
                     │   - LLM 虚拟陪伴 & 智能音箱对接     │
                     │   - DLNA 投屏 & iptables 流量控制 │
                     └──────┬─────────────────────▲──────┘
                            │ (UDS 控制事件)       │
                            │                     │ (SHM 零拷贝读取)
                            ▼                     │
  ┌───────────────────────────────────────────────┴──────────────────────────────┐
  │ C 语言物理控制底座 (smart_box)                                                 │
  │                                                                              │
  │  ┌─────────────────┐       ┌─────────────────┐       ┌────────────────────┐  │
  │  │ 物理驱动 / GPIO │       │ 共享内存 (SHM)  │       │ Wasm 实时沙箱 (1Hz)│  │
  │  │ (LED, SOS, PMIC)│ ◄───► │  Shadow Matrix  │ ◄───► │  (rehab_companion) │  │
  │  └─────────────────┘       └─────────────────┘       └────────────────────┘  │
  └──────────────────────────────────────────────────────────────────────────────┘
```

### 2.1 物理底座与业务网关职责
1. **`smart_box` (C 语言物理控制底座)**：
   - 强实时任务，用 `libuv` 单线程主循环挂载事件，直接调用 GPIO、I2C PMIC 和 BLE GATT 接口。
   - 实例化 `WAMR (WebAssembly Micro Runtime)`，每秒（1Hz）读取 DataHub 并执行加载的 Wasm 算子（跌倒与心率判定）。
   - 通过内核 Watchdog（`/dev/watchdog`）喂狗，超时自愈。
2. **`smart-box-gateway` (Go 语言慢车道网关)**：
   - 运行在非实时平面，内置轻量级反向代理（监听 80 端口，路由转发）。
   - 托管 SQLite3 数据库并负责云端加密备份、大模型（LLM）对话与智能音箱交互等计算密集/复杂协议栈逻辑。

### 2.2 共享内存 (SHM) 内存布局与对齐
底座初始化时，在系统中开辟共享内存区：
- **SHM 物理文件**：`/dev/shm/edgeos_shadow_matrix`
- **内存对齐与格式**：每个 Tag 单元严格对齐 `sizeof(pipe_msg_t)` (64字节)。
  ```c
  typedef struct {
      uint8_t priority;
      uint8_t source_id;
      uint8_t _res[5];       // 占用 7 字节，确保 Header 属性部分对齐
  } PipeMsgAttr;

  typedef struct {
      uint64_t ts;          // 8字节 (MTT 纳秒时间戳)
      uint32_t tag_id;      // 4字节
      uint32_t device_id;   // 4字节
      uint16_t type;        // 2字节 (如 PIPEMSG_TYPE_SCALAR)
      uint16_t state;       // 2字节
      uint32_t drop_reason; // 4字节
      uint8_t quality;      // 1字节
      PipeMsgAttr attr;     // 7字节

      union {
          struct {
              double f;      // 8字节 浮点值
              int64_t i;     // 8字节 整型值
          } val;

          struct {
              void *ptr;
              int fd;
              size_t len;
              void (*free_cb)(void *);
          } ref;

          struct {
              void *ptr;
              int count;
          } batch;

          struct {
              uint32_t event_id;
              uint32_t param;
          } event;
      } payload;            // 起始偏移量 32 字节
  } PipeMsg;

  typedef PipeMsg pipe_msg_t;
  _Static_assert(sizeof(PipeMsg) == 64, "PipeMsg must be 64 bytes");
  ```
- Go 语言端利用 `sys/unix` 的 `Mmap` 零拷贝映射此文件，直接按偏移量读取原始值与类型，完全避免进程间频繁系统调用的性能开销。
- smartbox.yaml 点表定义中的 Tag ID，将经由 `edge_tag_get_compact_index` 映射为 compact index，并在 `on_tick` 的 Wasm 读取中进行标量判定与处理。对于多维原始数据（如三轴加速度），采用 `PIPEMSG_TYPE_REF` 或 `PIPEMSG_TYPE_BATCH` 指针进行缓冲区交互。
```

### 2.3 Unix 域套接字 (UDS) 控制层协议
对于非周期的控制指令（如：开始 BLE 扫描、触发投屏、修改隐私模式状态），C 与 Go 通过 `/tmp/smart_box_ipc.sock` 传输自定义二进制报文：

| 字节偏移 | 字段名 | 类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **0 - 1** | `Magic` | `uint16_t` | 帧头魔数，固定为 `0x5342` ("SB") |
| **2** | `MsgType` | `uint8_t` | 指令类型 (1=设置隐私模式, 2=启动设备扫描, 3=SOS报警事件, 4=OTA触发) |
| **3** | `Length` | `uint8_t` | 数据负载长度 $N$（物理上限严格限制为 `MaxPayload = 255` 字节。接收解析时必须进行长度合法性截断校验，丢弃任何非法的越界帧） |
| **4 - (4+N-1)**| `Payload` | `uint8_t[N]` | 指令参数 payload |
| **4+N** | `Checksum` | `uint8_t` | 异或校验和 (XOR Checksum) |

---

## 三、 南向多协议接入与物理接口设计 (Southbound Protocols & Interfaces)

### 3.1 GPIO 物理引脚分配

| 引脚编号 | 引脚名称 | 信号类型 | 电平标准 | 挂载物理设备 | 驱动路径 / 注册机制 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Pin 3** | `I2C1_SDA` | Digital I/O | 3.3V | LIS3DH 3轴加速度计 SDA | `/dev/i2c-1` (地址 `0x1D`) |
| **Pin 5** | `I2C1_SCL` | Output | 3.3V | LIS3DH 3轴加速度计 SCL | `/dev/i2c-1` (地址 `0x1D`) |
| **Pin 8** | `UART2_TX` | Output | 3.3V | RS485 物理串口 TXD | `/dev/ttyYS1` |
| **Pin 10** | `UART2_RX` | Input | 3.3V | RS485 物理串口 RXD | `/dev/ttyYS1` |
| **Pin 11** | `GPIO_23` | Output | 3.3V | 绿灯 LED | `/sys/class/gpio/gpio23` |
| **Pin 12** | `GPIO_24` | Output | 3.3V | 红灯 LED | `/sys/class/gpio/gpio24` |
| **Pin 13** | `GPIO_25` | Output | 3.3V | 黄灯 LED | `/sys/class/gpio/gpio25` |
| **Pin 15** | `GPIO_26` | Input | 3.3V | 物理 SOS 大按钮 | `/sys/class/gpio/gpio26` (下降沿中断) |
| **Pin 19** | `SPI1_MOSI`| Output | 3.3V | PN532 NFC 读卡芯片 | `/dev/spidev0.0` (SPI 通信) |
| **Pin 21** | `SPI1_MISO`| Input | 3.3V | PN532 NFC 读卡芯片 | `/dev/spidev0.0` |
| **Pin 23** | `SPI1_CLK` | Output | 3.3V | PN532 NFC 读卡芯片 | `/dev/spidev0.0` |
| **Pin 24** | `SPI1_CS`  | Output | 3.3V | PN532 NFC 读卡芯片 | `/dev/spidev0.0` |
| **Pin 29** | `GPIO_29` | Output | 3.3V | 摄像头电源使能 (VCC_CAM) | `/sys/class/gpio/gpio29` (低电平切断) |
| **Pin 31** | `GPIO_31` | Output | 3.3V | 麦克风电源使能 (VCC_MIC) | `/sys/class/gpio/gpio31` (低电平切断) |

### 3.2 物理引脚控制与物理切断机制
1. **可视化指示灯控制 (Tag 12002 - `BOX_INDICATOR_LED`)**：
   - 当值为 `1.0` (系统正常) -> `/sys/class/gpio/gpio23/value` 写入 `1`，其余写入 `0`。
   - 当值为 `2.0` (系统报警) -> 启动 `libuv` 定时器，以 500ms 周期交替对 `GPIO_24` 写入 `1` 和 `0`，呈现红灯闪烁效果。
   - 当值为 `3.0` (升级中) -> `GPIO_25` 写入 `1`，呈现黄灯常亮。
2. **物理 SOS 大按钮边沿中断**：
   - 硬件中断判定：C 底座在初始化时，使用 epoll 监视 `/sys/class/gpio/gpio26/edge` 的变化，触发极性设为 `falling` (下降沿触发)。
   - 触发动作：当老人按下 SOS 键，引脚拉低，C 底座立刻捕获此中断，向 DataHub 写入 `BOX_VOICE_COMMAND = 2` (紧急求助指令字)，并向 Go 后端发送 UDS 二进制报警帧。
3. **隐私沙盒物理电源切断 (GDPR 物理隔离保护)**：
   - 当用户触发隐私模式 (`PRIVACY_MODE_ACTIVE = 1`) -> C 底座立即向 `gpio29/value` 和 `gpio31/value` 写入 `0`（低电平截止），物理断开摄像头与麦克风的供电，从硬件上防堵解密与窃听。

### 3.3 PN532 NFC 一碰绑定与设备发现 (SPI1)
- C 底座主循环定期向 PN532 发送 `InListPassiveTarget` 指令，扫描 NFC 标签。
- 当患者将贴有 NFC 标签的外骨骼机器人靠近盒子时，PN532 读取 NDEF 格式 JSON 报文：
  `{"dev_type":"exoskeleton","slave_id":1,"protocol":"ModbusRTU","baud":115200}`
- C 底座将此数据包通过 UDS 发送给 Go 网关，Go 网关自动在本地 SQLite 中注册该设备并下发 Modbus 轮询任务。

### 3.4 穿戴式心率手环 BLE GATT 对接
- C 底座后台调用 BlueZ，维持与心率手环 MAC 地址的 ACL 链接，订阅标准 GATT 特征值 UUID `0x2A37` (通知模式)。
- 报文解析规则：
  - 首字节为 Flag 标志位。
  - 若 `Flag & 0x01 == 0`，则第 2 字节为 8 位无符号整数表示的心率值。
  - 若 `Flag & 0x01 == 1`，则第 2-3 字节为 16 位无符号整数表示的心率值。
- 成功解析心率值后，将其更新写入 DataHub 的 `RAW_HEART_RATE` (Tag 12005) 寄存器。

### 3.5 康复外骨骼机器人 Modbus RTU (RS485)
- 通信接口：`/dev/ttyYS1` 串口设备，参数为 `115200, 8N1`。
- C 底座每 200ms 下发读取保持寄存器请求（读取 0x0100 开始的 2 个字）。
- 解析应答帧，提取当前训练进度，折算后写入 DataHub 的 `REHAB_COMPLETION_RATE` (Tag 12007)。

### 3.6 PMIC AXP209 电池监测 (I2C)
- 物理接口：`/dev/i2c-1`，PMIC 芯片地址为 `0x34`。
- C 底座以 2s 周期读取 AXP209 寄存器 `0x01` (Power Status Register)。
- 检查 Bit 4 (AC IN Presence) 与 Bit 3 (USB IN Presence)：
  - 若均为 0，表示外部交流电源断开，系统运行在移动后备电池上，DataHub 写入 `SYS_POWER_SOURCE = 1`。
  - 若任一为 1，表示电源接入正常，写入 `SYS_POWER_SOURCE = 0`。

---

## 四、 数据库、去标识化与区块链存证 (Database & Data Security)

### 4.1 SQLite 数据库表结构设计 (Schema)
智能盒子拥有专用的本地数据库 `/workspaces/edgeos/EdgeC/smart_box.db`，表结构如下：

1. **`ui_configs` 表** (UI 布局与用户配置)：
   ```sql
   CREATE TABLE IF NOT EXISTS ui_configs (
       key TEXT PRIMARY KEY,
       val TEXT
   );
   ```
2. **`devices` 表** (通过 NFC 一碰绑定或 BLE 发现的设备列表)：
   ```sql
   CREATE TABLE IF NOT EXISTS devices (
       id INTEGER PRIMARY KEY AUTOINCREMENT,
       name TEXT NOT NULL,
       dev_type TEXT NOT NULL,
       slave_id INTEGER DEFAULT 0,
       protocol TEXT NOT NULL,
       address TEXT NOT NULL,
       baud INTEGER DEFAULT 0,
       bound_time DATETIME DEFAULT CURRENT_TIMESTAMP
   );
   ```
3. **`tag_history` 表** (常规传感器历史物理量，保留 30 天)：
   ```sql
   CREATE TABLE IF NOT EXISTS tag_history (
       timestamp INTEGER NOT NULL,
       tag_id INTEGER NOT NULL,
       value REAL NOT NULL,
       status INTEGER DEFAULT 0,
       PRIMARY KEY (timestamp, tag_id)
   );
   CREATE INDEX IF NOT EXISTS idx_tag_history_timestamp ON tag_history(timestamp);
   ```
4. **`sensitive_records` 表** (加密存储的隐私数据，如心率数值、步态明细)：
   ```sql
   CREATE TABLE IF NOT EXISTS sensitive_records (
       timestamp INTEGER NOT NULL,
       tag_id INTEGER NOT NULL,
       iv TEXT NOT NULL,
       ciphertext TEXT NOT NULL,
       auth_tag TEXT NOT NULL,
       PRIMARY KEY (timestamp, tag_id)
   );
   ```
5. **`blockchain_receipts` 表** (区块链存证收据记录)：
   ```sql
   CREATE TABLE IF NOT EXISTS blockchain_receipts (
       timestamp INTEGER PRIMARY KEY,
       tx_hash TEXT NOT NULL,
       record_hash TEXT NOT NULL,
       signature TEXT NOT NULL
   );
   ```

### 4.2 本地敏感数据加密存储 (AES-256-GCM)
为避免盒子丢失导致患者隐私泄露，Go 网关在写入 `sensitive_records` 表前，必须在应用层执行 **AES-256-GCM** 加密：
1. **密钥派生 (KDF)**：
   - 读取 CPU 唯一序列号（从 `/proc/cpuinfo` 提取 `Serial` 字段），与硬编码的物理盐值做 **PBKDF2-HMAC-SHA256**（迭代 10000 次），生成 256 位密钥。
2. **加密算法逻辑**：
   - 每次加密产生一个随机的 12-byte 初始向量 (IV)。
   - 使用 AES-GCM 算法对浮点数/字符串形式的健康指标进行加密，输出密文 `ciphertext` 和由 `aesgcm.Overhead()`（默认为 16 字节）确定的认证标签 `auth_tag`，严禁在提取时硬编码魔数 16。

### 4.3 GDPR 被遗忘权与物理零擦除接口 (`/api/v1/privacy/purge`)
当用户在 Vue UI 上点击“被遗忘权”注销时，Go 网关执行以下擦除：
1. **指令分发**：下发 UDS 帧通知 C 底座，C 底座立即暂停 Wasm 对 DataHub 数据的轮询写入。
2. **物理擦除**：Go 网关在数据库事务中执行：
   ```sql
   BEGIN TRANSACTION;
   DELETE FROM sensitive_records;
   DELETE FROM tag_history;
   DELETE FROM blockchain_receipts;
   COMMIT;
   ```
3. **物理碎片重整**：在事务提交之后，在全局 `g.mu.Lock()` 互斥锁的保护下，在主连接上独立执行 **`VACUUM;`** 命令（不得处于任何活跃事务内）。由于 `VACUUM` 会请求独占数据库排他锁，互斥锁保护可防止任何并发的读取或写入操作导致 SQLite 锁冲突失败。SQLite 会物理重写数据库文件，将所有已删除页数据清零重构。

### C.4 区块链存证机制
1. **设备密钥对**：在 `/workspaces/edgeos/EdgeC/keys/` 目录下生成独一无二的私钥 `smartbox_private.pem` 和公钥 `smartbox_public.pem`。
2. **存证逻辑与防重放**：
   - 为彻底规避重放攻击风险，计算哈希时除打包 JSON 记录外，必须强行拼装毫秒级高精度时间戳（或随机 Nonce）：
     $$Hash = SHA256(JSON\_Record \parallel "-" \parallel Timestamp)$$
   - 用本地 ECDSA 私钥对该 Hash 进行签名：
     $$r, s = ECDSA\_Sign(Hash, PrivateKey)$$
   - **签名固定编码规则**：将大整数 $r$ 和 $s$ 各自通过 `FillBytes` 或零填充形式转换为固定 **32 字节** 长度的大端字节数组，拼接为固定 **64 字节** 的 `r || s` 格式（如：`sigBytes := append(rBytes, sBytes...)`），随后进行 Base64 编码，避免首位为零时长度缩水导致的解析越界。
   - 将签名和哈希推送到云端存证节点，并将 `tx_hash`、`record_hash` 和 `signature` 写入本地 `blockchain_receipts` 表。

---

## 五、 边缘计算、AI 算法与 Wasm 沙箱设计 (AI Algorithms & Wasm Sandbox)

### 5.1 Wasm 沙箱资源限额与安全机制
底座主循环动态加载并周期调度 `/data/apps/rehab_companion.wasm`：
- **堆栈物理分配**：堆内存空间上限为 **256KB**，运行栈为 **64KB**。
- **执行时间强熔断 (Execution Watchdog)**：
  - C 宿主在调度 Wasm 算子的 `on_tick` 前，启动一个高精度独立看门狗线程。
  - 如果 `on_tick` 执行耗时超过 **10ms**，看门狗线程立即向 WAMR 虚拟机抛出 `timeout-interrupted` 异常，强制掐断 Wasm 实例运行。
- **Trap 崩溃防连坐 (Isolation)**：
  Wasm 发生除零、非法的内存越界读写等指令级崩溃 (Trap)，WAMR 捕获后仅释放当前 Wasm 算子实例上下文，宿主进程正常运行。

### 5.2 跌倒判定逻辑与双指标算法 (SVM & SMA)
Wasm 算子订阅三轴加速度数据 `RAW_ACCELERATION` (Tag 12006)，以 100Hz 物理频率输入滑动缓冲区 $A[100]$（记录最新 1 秒波形）：

1. **信号矢量幅值 (SVM) 瞬时检测**：
   $$SVM[i] = \sqrt{a_x[i]^2 + a_y[i]^2 + a_z[i]^2}$$
   - **失重相判定**：若连续 10 个采样点（即 $100\text{ms}$ 时长）其 $SVM[i] < 0.2 G$，判定进入空中坠落状态。
   - **冲击碰撞判定**：在失重信号触发后的 500ms 时间窗内，如果检索到任意采样点 $SVM[j] > 3.2 G$，则标记进入强物理碰撞状态。
2. **信号幅值面积 (SMA) 姿态恢复监测**：
   在强碰撞后第 1.0 秒开始，计算后续 1.0 秒内的信号幅值面积：
   $$SMA = \frac{1}{N} \sum_{k=0}^{N-1} \left( |a_x[k]| + |a_y[k]| + |a_z[k]| \right)$$
   - 若 $SMA < 0.4 G$，代表碰撞后人体处于极低动力学活动状态（静卧/失去知觉）。
   - 同时满足 **失重 + 碰撞 + 低 SMA** 三重条件，判定为真实跌倒。
   - 触发逻辑：写入 `BOX_FALL_ALERT = 1`，`BOX_INDICATOR_LED = 2` (红灯闪烁)，`BOX_SYSTEM_STATUS = 1`。

### 5.3 步频 (Cadence) 与步态非对称性分析 (Gait Analysis)
当外骨骼机器人在 Modbus/GATT 链路激活时，盒子接收实时双腿关节的角度与运动相位：
1. **步频 (Cadence) 计算**：
   - 提取步态加速度信号的波峰（足底着地 Heel Strike）。
   - 计算 10 秒时间窗内的波峰总数 $P$，估算每分钟步数：
     $$Cadence = P \times 6 \text{ (Steps/Min)}$$
2. **步态非对称性评分 (Asymmetry Index)**：
   - 记录左腿单次跨步支撑相时长 $T_L$，与右腿单次支撑相时长 $T_R$：
     $$Asymmetry = 100 \times \frac{|T_L - T_R|}{\frac{1}{2}(T_L + T_R)} (\%)$$
   - 若 $Asymmetry > 15\%$，提示步态严重失衡或偏瘫风险，前端自动推送健康警告。

### 5.4 适老化界面自适应学习得分 (Habit Score)
Go 慢车道网关自动统计用户每日 24 小时的功能模块点击频次。每项功能（例如：电子相册、AI聊天、康复训练、电视投屏）在时段 $H$ 的推荐得分计算公式为：
$$Score(Feature, H) = \alpha \cdot Freq(Feature, H) + (1-\alpha) \cdot Recency(Feature)$$
- $\alpha = 0.7$（时间粘性权重占 70%）。
- 前端 Vue UI 主动拉取排名前三的 Feature 并置顶，自动为老人凸显最需要的功能。

### 5.5 跌倒概率与慢病恶化趋势预测 (Logistic Regression)
1. **跌倒概率预测模型**：
   使用逻辑回归模型预测患者接下来的跌倒几率 $P(Fall)$：
   $$P(Fall) = \frac{1}{1 + e^{-z}}$$
   $$z = \beta_0 + \beta_1 \cdot Asymmetry + \beta_2 \cdot HistoricalFalls - \beta_3 \cdot DailySteps$$
   - 经验常数定义：$\beta_0 = -2.5$ (基准偏置), $\beta_1 = 0.08$ (非对称指数权重), $\beta_2 = 1.2$ (历史跌倒史加重权重), $\beta_3 = 0.00015$ (平均每日步数衰减因子)。
   - 得出的概率由 Go 网关分为三档推送到家属/医生端：低风险 (<20%)、中风险 (20%-50%)、高风险 (>50%)。

---

## 六、 慢车道网关、AI 集成与网络流量控制 (Integrations & Network Control)

### 6.1 AI 情感陪伴与用药提醒接口 (Go 后端)
Go 后端服务通过 HTTP 客户端连接云端大模型，实现流式自然语言处理与健康指标融合。
- **对话接口**：`POST /api/v1/companion/chat`
- **动态 Prompt 注入**：网关在将用户消息发送给 LLM 之前，零拷贝读取 SHM 中的最新 Tag 指标：
  - 心率 (`RAW_HEART_RATE`)；
  - 训练进度 (`REHAB_COMPLETION_RATE`)；
  - 是否有未读活动通知。
- **Prompt 模板封装**：
  ```
  [系统设定]
  你是一个部署在智能居家健康盒子里的 AI 虚拟伴侣，名叫“小康”。当前对话的老人 72 岁。
  当前生理数据：实时心率 = {{RAW_HEART_RATE}} BPM，今日运动达成度 = {{REHAB_COMPLETION_RATE}}%。
  [限制条件]
  1. 说话口吻要温和、慢条斯理，使用短句。
  2. 若当前心率超过 140 或低于 50，必须在对话开头强制提示：“检测到您心率有波动，请坐下静息，我已通知家属”。
  3. 若运动达成度 < 50% 且当前是傍晚，委婉提醒其散步或使用外骨骼进行训练。
  4. 结合常识回答老人的聊天，字数控制在 100 字内。
  ```

### 6.2 UPNP/DLNA 大字版电视投屏
- **技术原理**：Go 后端引入 SSDP 模块，定期多播扫描局域网内的智能电视及机顶盒（服务标识为 `urn:schemas-upnp-org:service:AVTransport:1`）。
- **投屏接口**：`POST /api/v1/ui/cast`
- **执行流程**：向电视的 `AVTransport` 控制端口发送 SOAP XML 投屏请求，调用 `SetAVTransportURI`，参数 `CurrentURI` 传入当前盒子的大字版 HMI URL，实现把 100Hz 示波器波形与健康卡片投影到大屏电视。

### 6.3 金币兑换 WiFi 流量网络限速控制 (Traffic Controller)
智能盒子可通过 4G/5G 模组开启 WiFi 热点，并利用 `iptables` 实现基于积分的流量配额管理：
1. **默认封锁**：在启动时，Go 网络初始化脚本封锁热点端口的外网路由：
   ```bash
   iptables -t filter -A FORWARD -i wlan0 -o wwan0 -j DROP
   ```
2. **兑换流量处理**：用户在 Vue UI 点击兑换后，Go 网关从 SQLite 扣减金币，并向 `iptables` 插入一条免检放行规则：
   ```bash
   iptables -t filter -I FORWARD 1 -i wlan0 -o wwan0 -m mac --mac-source <CLIENT_MAC> -j ACCEPT
   ```
3. **流量熔断监控**：Go 内部运行 `TrafficWatcher` 线程，每隔 5 秒执行 `iptables -L FORWARD -v -n -x` 读取累计字节数。当累计字节数超过兑换额度时，立刻执行删除规则：
   ```bash
   iptables -t filter -D FORWARD -m mac --mac-source <CLIENT_MAC> -j ACCEPT
   ```

### 6.4 智能音箱对接与场景自适应
- **服务接口**：Go 后端提供外部 Webhook：`POST /api/v1/voice/intent`，供小米/百度智能音箱 Skill 意图回调，收到控制意图后，通过 SHM 写入 DataHub 的 `BOX_VOICE_COMMAND = 1`。
- **场景自适应 (`BOX_CURRENT_SCENE`)**：
  - **居家模式 (0)**：启用本地 Wasm 100Hz 跌倒/心率分析；使能家庭电子相册屏保。
  - **养老院模式 (1)**：开启网络集中管理，自动接收管理端群发的用药广播（Activity Broadcaster）。
  - **社区模式 (2)**：关闭摄像头与麦克风供电（`PRIVACY_MODE_ACTIVE = 1`），突出社区团体活动与金币贡献度排行。

---

## 七、 物理部署与安全自愈机制 (Deployment & Self-Healing)

### 7.1 eMMC 闪存空间分区表映射
主板配备 8GB eMMC 闪存，分区布局如下：

| 分区节点 | 挂载点 | 起始物理地址 | 结束物理地址 | 空间大小 | 文件系统与承载内容 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `/dev/mtd0` | N/A (只读) | `0x00000000` | `0x00100000` | 1MB | 原始 Bootloader (U-Boot 二进制) |
| `/dev/mtd1` | N/A (只读) | `0x00100000` | `0x01000000` | 15MB | U-Boot 引导环境变量与安全证书密钥签名 |
| `/dev/mtd2` | `/` (活动 A) | `0x01000000` | `0xE1000000` | 3.5GB | Ext4 (只读挂载，系统 A 分区，存放底层 Linux + C/Go/Wasm 程序) |
| `/dev/mtd3` | `/` (备用 B) | `0xE1000000` | `0x1C1000000`| 3.5GB | Ext4 (只读挂载，系统 B 分区，作为 OTA 静默下载与热升级写入区) |
| `/dev/mtd4` | `/data` (读写) | `0x1C1000000`| 闪存物理边界 | 约 984MB | Ext4 (可读写挂载，存放 `smart_box.db`、家庭照片视频、系统运行 Log) |

### 7.2 A/B 双分区无感升级与回滚流程
1. **静默写入**：Go 网关校验升级包完整性后，将固件静默写入备份分区 B (`/dev/mtd3`)。
2. **切换引导指针**：Go 调用系统底座，修改 U-Boot 引导参数：
   `fw_setenv boot_switch 1` (1指向B，0指向A)，`fw_setenv boot_count 0`，并触发重启。
3. **启动自检与回滚**：系统从 B 分区引导启动，开启 30 秒倒计时自检。自检失败或系统卡死时，由于未喂狗，看门狗超时触发设备复位。U-Boot 检查到 `boot_count` 自增，若 `boot_count >= 3`，U-Boot 自动将 `boot_switch` 复位清零，系统强制回滚启动至原先安全的分区 A。

### 7.3 内核级硬件看门狗喂狗逻辑 (Watchdog API)
在 C 底座 `smart_box` 中，开辟独立线程绑定硬件看门狗 `/dev/watchdog`，设置超时门限为 5s，在主循环定时器中每 2 秒触发一次 ioctl 喂狗操作，实现运行期进程死锁自愈。

### 7.4 物理大按钮 (GPIO 26) 触发 U 盘救砖自愈
在上电瞬间，U-Boot 检测到 `GPIO_26`（物理 SOS 大按钮）被持续拉低 5 秒以上，进入 `USB Recovery Mode`，提示黄灯常亮。挂载插入盒子的 FAT32 格式 U 盘第一分区 `/dev/sda1`，执行 `/recovery.sh` 脚本，格式化并重新提取原厂固件 `golden_firmware.tar.gz` 写入系统 A 分区，并重置 U-Boot 引导环境变量，实现脱机救砖自愈。

---

## 八、 未来扩展与 PRD 遗漏补充 (PRD Requirements Coverage & Future Extensions)

为响应产品需求文档 (PRD) 对物联网边缘及慢病看护的规划，设计对以下 PRD 未覆盖模块进行预架构拓展：

### 8.1 Zigbee / LoRa 双物联网中枢物理接入 (PRD §2.1)
1. **Zigbee 模块**：
   - 接入引脚：USB /dev/ttyUSB0 (内部集成 EZSP 协议转换器，波特率 115200)。
   - 驱动机制：C 底座加载内置 `zigbee_driver`，在独立的事件轮询中读取串口数据帧，解析出环境传感器（如养老院温湿度、漏水探头）状态，并映射写入 DataHub。
2. **LoRa 模块 (USB接入)**：
   - 物理通道：`/dev/ttyUSB1` (波特率 9600)，采用标准 AT 指令控制射频模块。
   - 轮询策略：每 5 秒发起一次接收状态检索，处理家庭周界围栏告警信号，保障断网环境下的超长距离传感器应急接入。

### 8.2 慢病管理模块与健康数据采集 (PRD §6.2)
1. **血糖仪与血压计**：
   - 采用标准 BLE GATT 规范协议进行绑定与数据同步：
     - 血糖监测服务：GATT UUID `0x1808` (Glucose Service)。
     - 血压监测服务：GATT UUID `0x1810` (Blood Pressure Service)。
   - 数据捕获后，写入 DataHub 专用 Tag，由 Go 网关读取后通过 AES-256-GCM 密文插入 `sensitive_records` 表，记录为本地历史档案。

### 8.3 云端双向双备份 mTLS 传输与增量同步机制 (PRD §3.1.1)
1. **端到端加密传输**：
   - Go 网关与云端服务器之间的所有 REST/MQTT 通信强制开启双向 TLS 认证 (mTLS)，客户端证书 `client_cert.pem` 由设备出厂阶段写入只读分区 `/dev/mtd1` 并由硬件加密卡保护。
2. **云端增量同步与备份**：
   - Go 网关在每天凌晨 2:00 生成增量 SQLite 数据备份。
   - 使用设备 PBKDF2 派生密钥对备份数据库进行对称加密打包（`tar.gz.aes`），并通过 HTTPS mTLS 安全通道上传至远程云端备份服务器，确保“本地+云端”双备份安全性。

### 8.4 本地语音识别引擎与视音频采集管线 (PRD §5.3.1, §4.5)
1. **本地语音识别 (ASR/TTS)**：
   - 基于轻量级 ASR 引擎（如 Sherpa-onnx），以独立守护进程常驻，通过 `/proc/asound` ALSA 音频直读。
   - 识别结果经分词转换为意图 JSON，通过 UDS 通道发送给 Go 网关。
2. **视频与音频物理开关逻辑**：
   - 当 `PRIVACY_MODE_ACTIVE = 1` 时，GPIO 29/31 强制切断摄像头与麦克风 VCC 物理供电线，从源头上杜绝黑客通过驱动溢出越权开启视音频监听的物理安全红线。
   - 在隐私模式关闭时，使用 standard V4L2 与 ALSA 驱动捕获多媒体流，支持紧急救援视频通话。
