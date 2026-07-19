# 智能盒子 (Smart Box) 工业级软硬件接口与 AI 协同详细设计说明书

> **产品代号**: SmartBox-Alpha  
> **固件基线**: EdgeOS V2.1.0-AB  
> **文档版本**: V2.0 (Detailed Engineering Design Spec)  
> **文档所有者**: Mr. WKT  
> **密级**: 生产级绝密  

---

## 一、 硬件芯片选型与物理引脚接口设计 (Hardware Interface Specifications)

智能盒子采用核心主板（运行 Linux）与低功耗传感器节点（运行 Zephyr）协同的架构。主板核心 SoC 采用 NXP i.MX8M Nano 工业级应用处理器（4x Cortex-A53 @1.5GHz + 1x Cortex-M7 @600MHz），配备 1GB DDR4 内存与 8GB eMMC 闪存。

### 1.1 物理引脚分配 (Pinout Definition)

主机板 40-Pin 物理引脚中，针对传感器、指示灯及调试按钮的分配如下表所示：

| 引脚编号 | 引脚名称 | 信号类型 | 电平标准 | 物理接口/挂载外设 | 内核驱动/映射路径 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Pin 1** | `VCC_3V3` | Power | 3.3V | 主板外设供电轨 | N/A |
| **Pin 3** | `I2C1_SDA` | Digital I/O | 3.3V | LIS3DH 3轴加速度计 SDA | `/dev/i2c-1` (地址 `0x1D`) |
| **Pin 5** | `I2C1_SCL` | Output | 3.3V | LIS3DH 3轴加速度计 SCL | `/dev/i2c-1` (地址 `0x1D`) |
| **Pin 8** | `UART2_TX` | Output | 3.3V | RS485 物理串口 TXD | `/dev/ttyYS1` |
| **Pin 10** | `UART2_RX` | Input | 3.3V | RS485 物理串口 RXD | `/dev/ttyYS1` |
| **Pin 11** | `GPIO_23` | Output | 3.3V | 状态 LED (绿灯) | `/sys/class/gpio/gpio23` |
| **Pin 12** | `GPIO_24` | Output | 3.3V | 状态 LED (红灯) | `/sys/class/gpio/gpio24` |
| **Pin 13** | `GPIO_25` | Output | 3.3V | 状态 LED (黄灯) | `/sys/class/gpio/gpio25` |
| **Pin 15** | `GPIO_26` | Input | 3.3V | 物理大按钮 (按键中断) | `/sys/class/gpio/gpio26` (检测下降沿) |
| **Pin 16** | `RS485_DE` | Output | 3.3V | SP3485 芯片收发使能脚 | 由串口驱动内核态自动控制 (RTS 自动流控) |
| **Pin 39** | `GND` | Ground | 0V | 公共地线 | N/A |

### 1.2 物理外设通讯电气规范

1. **三轴加速度计 (LIS3DH)**：
   - 接口：I2C 模式，SCL 时钟频率最高支持 400kHz。
   - 满量程可选：$\pm 2g/\pm 4g/\pm 8g/\pm 16g$。系统统一配置为 $\pm 8g$，以防止剧烈冲击时信号饱满失真。
   - 分辨率：12-bit。
2. **RS485 隔离电路**：
   - 采用 ADM2483 磁隔离型收发器，隔离电压 2.5kV RMS。
   - 带短路保护和热关断功能，120 欧姆终端电阻通过物理拨码开关选择是否接入。

---

## 二、 内存预算与算力限制 (Memory & CPU Resource Budgets)

为了防止进程内存溢出（OOM）导致设备崩溃，对系统 1GB 总内存和双核 CPU 算力进行硬性切分：

```
                    【智能盒子 1GB 总物理内存分配】
┌─────────────────────────────────────────────────────────────┐
│ 运行内核与根文件系统 (Linux Core)                     : 512MB  │
├─────────────────────────────────────────────────────────────┤
│ 慢车道网关守护进程 (Go Gateway & API Server)          : 128MB  │
├─────────────────────────────────────────────────────────────┤
│ 缓存数据库 (本地 SQLite3 iems.db 进程缓存)            :  64MB  │
├─────────────────────────────────────────────────────────────┤
│ DataHub 零拷贝共享内存 (dev/shm)                       :  16MB  │
├─────────────────────────────────────────────────────────────┤
│ WAMR WebAssembly 虚拟机实例池 (8 个 App 额度)         :  64MB  │
├─────────────────────────────────────────────────────────────┤
│ 预留系统空闲缓冲 (System Emergency Buffer)            : 220MB  │
└─────────────────────────────────────────────────────────────┘
```

### 2.1 DataHub 共享内存物理结构设计 (DataHub Memory Layout)

DataHub 的共享内存（SHM）映射路径为 `/dev/shm/edgeos_shadow_matrix`。总大小预分配为 **16,777,216 字节 (16MB)**。内存空间排布字节如下：

| 起始偏移量 (Hex) | 长度 (Bytes) | 区域名称 | 数据结构/字节对齐方式 | 读写权限 |
| :--- | :--- | :--- | :--- | :--- |
| `0x000000` | `1,024` | 内存头控制区 (Header) | `shm_header_t` (包含魔数、Tag总数、读写互斥锁) | 宿主进程 (RW) |
| `0x000400` | `4,194,304` | 实时寄存器影子表 (Shadow) | `pipe_msg_t` 结构体数组（共 4096 个坑位，每个 1024 字节对齐） | 宿主 (RW), 慢车道 (RO), Wasm (RO) |
| `0x400400` | `8,388,608` | 物理高频波形环形队列 (RingBuf)| `shm_ring_t`（双缓冲环形队列，负责 ADC_RAW 100Hz 传输） | 驱动线程 (W), 算子线程 (R) |
| `0xC00400` | `4,194,304` | 用户自定义配置缓存区 (User) | 扁平字节流数据空间 | Go 网关 (RW) |

### 2.2 Wasm 运行时限制 (WAMR Sandbox Limitations)

在宿主进程中，每个实例化加载的 `.wasm` 字节码均运行在独立的 WAMR 隔离沙箱中，限制如下：
*   **Wasm 内存限制**：
    - 最大堆空间 (Heap Size)：`262,144` 字节 (256KB)。
    - 最大栈空间 (Stack Size)：`65,536` 字节 (64KB)。
    - 内存上限超出时，WAMR 抛出 `trap-out-of-bounds` 异常，沙箱被宿主线程强制终止。
*   **运行时 CPU 监控 (Watchdog)**：
    - Wasm 算子的 `on_tick` 执行时长上限为 **10ms**。
    - 宿主中，`wasm_watchdog_t` 定时监控线程每 1ms 轮询一次运行耗时。若耗时超过 10ms，立即执行：
      ```c
      wasm_runtime_set_exception(inst, "execution-timeout-killed");
      wasm_runtime_terminate(inst);
      ```

---

## 三、 跌倒检测与心率报警 AI 算法实操设计 (Local AI Specifications)

### 3.1 跌倒检测算法：SVM 与 SMA 双轨判定数学模型

本地 Wasm 算子读取加速度原始数据 $a_x, a_y, a_z$（单位：G），计算**信号矢量幅值 (SVM - Signal Vector Magnitude)** 与 **信号幅值面积 (SMA - Signal Magnitude Area)**：

1. **信号矢量幅值 (SVM)** 的计算公式为：
   $$SVM[i] = \sqrt{a_x[i]^2 + a_y[i]^2 + a_z[i]^2}$$
2. **信号幅值面积 (SMA)** 的计算公式为：
   $$SMA = \frac{1}{N} \sum_{j=0}^{N-1} \left( |a_x[j]| + |a_y[j]| + |a_z[j]| \right)$$
   其中 $N = 200$ （在 100Hz 采样率下，对应 2 秒时间窗口）。

```
  SVM (G)
   ▲
4.0│                                      ◆ (Impact Phase > 3.2G)
   │                                     / \
1.0│─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ (基准 1.0G)
   │                       _ _          /   \
0.2│                      /   \        /     \
   │                     /     \______/       \______________________
   │                    /  (Free Fall < 0.2G)   (Post-Fall Inactivity)
  0└────────────────────┴─────────────────────────────────────────────► 时间 (ms)
```

#### 算法判定判定逻辑步骤：
1. **自由落体检测 (Free Fall Phase)**：
   若连续 10 个采样点（100ms）满足 $SVM < 0.2 G$，记录失重起始时间戳 $T_{fall}$，标记为“失重状态”。
2. **冲击碰撞检测 (Impact Phase)**：
   在 $T_{fall}$ 发生后的 500ms 内，如果检测到某一点的 $SVM > 3.2 G$，判定发生了物理碰撞。
3. **静止与姿态检查 (Post-Fall Inactivity)**：
   碰撞发生后，检测 1000ms 内的 $SMA$。如果 $SMA < 0.4 G$ 且合加速度稳定在 $1.0 G$ 左右，表明人体在碰撞后处于“静止不动”状态，排除日常剧烈运动的假阳性。
4. **触发警报**：
   三步同时满足，Wasm 执行 Host API `host_set_tag(12003, 1.0)` 将跌倒报警置 1，并点亮红灯。

---

## 四、 南向与北向多协议十六进制报文设计 (Protocol Hex Frames)

### 4.1 Modbus RTU 南向外骨骼机器人指令规约

智能盒子通过 RS485 读写下肢外骨骼机器人的当前训练角度、拉力值并下发控制指令。

#### 1. 主机发送：轮询外骨骼状态帧 (读取从站 01 地址的第 0000 至 0003 寄存器)
*   **十六进制报文流**：`01 03 00 00 00 04 44 09`

| 字节偏移 | 字段意义 | 预设数值 (Hex) | 物理说明 |
| :--- | :--- | :--- | :--- |
| `Byte 0` | 设备地址 (Slave ID) | `01` | 外骨骼机器人默认地址 |
| `Byte 1` | 功能码 (Function Code) | `03` | 读保持寄存器 |
| `Byte 2-3` | 寄存器起始地址 | `00 00` | 从第 0 号寄存器开始 |
| `Byte 4-5` | 寄存器数量 | `00 04` | 共读取 4 个寄存器 |
| `Byte 6-7` | 校验码 (CRC-16 Modbus) | `44 09` | 包含起始至长度的 CRC 校验值 |

#### 2. 从站回执：状态数据响应帧 (带符号整数数据对齐)
*   **十六进制报文流**：`01 03 08 00 02 01 F4 00 12 00 00 A2 F2`

| 字节范围 | 字段意义 | 数值 (Hex) | 物理换算解析 |
| :--- | :--- | :--- | :--- |
| `Byte 0-2` | 帧头与字节长度 | `01 03 08` | 从站 01，功能码 03，后续数据共 8 字节 |
| `Byte 3-4` | 运行状态 (Status Word) | `00 02` | 代表“正处于步行训练模式” |
| `Byte 5-6` | 关节活动角度 (Angle) | `01 F4` | $0x01F4 = 500$，换算比例 0.1，即为 $50.0^\circ$ |
| `Byte 7-8` | 关节阻尼拉力 (Torque) | `00 12` | $0x0012 = 18$，即当前输出拉力 $18 N\cdot m$ |
| `Byte 9-10` | 故障代码 (Error Code) | `00 00` | `0` 代表当前物理设备运行无故障 |
| `Byte 11-12` | 校验码 (CRC-16) | `A2 F2` | 针对前 11 字节的整帧校验值 |

### 4.2 蓝牙心率带 GATT 解析规约

连接建立后，通过 `hcitool` 或 BlueZ 通信链路向 UUID 为 `00002a37-0000-1000-8000-00805f9b34fb` 的特征值写入使能控制字：
*   **使能订阅命令包**：`01 00` （写入 Client Characteristic Configuration Descriptor - CCCD `0x2902`，以开启心率 Notification）。
*   **心率通知包结构**：
    手环每秒吐出一包数据，例如：`16 4E E5 03`
    - `Byte 0` (Flags): `0x16` (二进制 `00010110`)。代表：心率数据占用 8-bit；传感器接触状态 OK；带 RR-Interval 间隔数据。
    - `Byte 1` (Heart Rate Value): `0x4E` (十进制 78 BPM)。此值立即被写入 DataHub `RAW_HEART_RATE` (Tag 12005)。
    - `Byte 2-3` (RR-Interval): `0x03E5` ($0x03E5 = 997ms$)。代表两次心跳的物理间隔。

---

## 五、 系统分区、Watchdog 自愈与防砖升级机制 (Deployment & Self-Healing)

智能中枢盒子的 eMMC 闪存大小为 8GB，规划的物理分区及地址空间映射如下：

```
eMMC Flash (8GB) 地址空间排布
┌───────────────────────────┬──────────────┬──────────────┬─────────────┐
│ Bootloader & Env (16MB)   │ System_A     │ System_B     │ User Data   │
│ 0x00000000 - 0x01000000   │ (Active OS)  │ (Backup OS)  │ (SQLite/log)│
└───────────────────────────┴──────────────┴──────────────┴─────────────┘
```

### 5.1 Flash 分区表映射 (Partition Table)

| 分区标号 | 设备挂载路径 | 分区起始地址 | 分区结束地址 | 物理空间大小 | 文件系统/存放内容 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `mtd0` | `/dev/mtd0` | `0x00000000` | `0x00100000` | 1MB | Raw Boot (U-Boot 二进制) |
| `mtd1` | `/dev/mtd1` | `0x00100000` | `0x01000000` | 15MB | U-Boot 环境变量与引导配置 |
| `mtd2` | `/dev/mtd2` | `0x01000000` | `0xE1000000` | 3.5GB | Ext4 (系统活动 A 分区，主 OS) |
| `mtd3` | `/dev/mtd3` | `0xE1000000` | `0x1C1000000`| 3.5GB | Ext4 (系统备用 B 分区，OTA 缓冲) |
| `mtd4` | `/dev/mtd4` | `0x1C1000000`| 8GB 物理边界 | 约 984MB | Ext4 (用户持久化配置及本地 SQLite db) |

### 5.2 A/B 分区无感 OTA 与回滚机制设计

```
       [开始 OTA 升级]
             │
             ▼
   [将固件写入非活动分区 System_B]
             │
             ▼
      [校验固件签名与 MD5]
             │
      ┌──────┴──────┐
  [验证失败]     [验证成功]
      │             │
      ▼             ▼
  [终止升级]     [写引导环境变量 boot_switch=1]
                    │
                    ▼
               [触发重启]
                    │
                    ▼
     [引导活动分区切换至 System_B]
                    │
                    ▼
      [运行 30s 系统健康度自检]
             ┌──────┴──────┐
         [健康自检失败]   [自检正常]
             │             │
             ▼             ▼
      [自动写回引导指针] [锁定 System_B 为活动分区]
      [复位重启回 System_A]
```

1. **升级触发**：
   云端下发升级指令，Go API 接收固件包后，使用 `dd` 命令将固件以 block 写入当前的**非活动分区**（假设当前运行 A，则写入 `/dev/mtd3`）。
2. **校验与引导转换**：
   - 验证签名。如果无误，调用 Linux 工具 `fw_setenv`，修改 U-Boot 变量 `boot_switch=1`。
   - 重启设备。U-Boot 识别到 `boot_switch=1` 后，切换启动分区指向备用分区 B。
3. **健康检查检测 (Sanity Check)**：
   - 新分区引导启动后，主守护进程初始化。
   - 启动 30 秒倒计时，期间判定 `datahub` 与 `wasm_scheduler` 是否全部启动正常。
   - 若自检崩溃（或主进程无法在 10s 内喂狗导致重启），U-Boot 触发计数器归零，自动将环境变量 `boot_switch` 擦除复位，引导重置切回安全的原 A 分区，实现 OTA 失败防砖。

### 5.3 硬件看门狗自愈接口实现 (Watchdog Interface)

在 Linux 环境下，底座程序 `edge_service.c` 内部封装了针对内核级 Watchdog 驱动的控制接口。

*   **API 结构定义**：
    ```c
    #include <sys/ioctl.h>
    #include <linux/watchdog.h>
    
    static int g_wd_fd = -1;
    
    // 打开并初始化看门狗驱动
    int edge_watchdog_init(const char *dev_path, int timeout_s) {
        g_wd_fd = open(dev_path, O_WRONLY);
        if (g_wd_fd < 0) {
            LOG_ERROR(LOG_CAT_CORE, "Failed to open watchdog device: %s", dev_path);
            return -1;
        }
        // 设置超时时限为 5 秒
        ioctl(g_wd_fd, WDIOC_SETTIMEOUT, &timeout_s);
        LOG_INFO(LOG_CAT_CORE, "Hardware Watchdog enabled. Timeout: %d seconds", timeout_s);
        return 0;
    }
    
    // 周期喂狗逻辑 (由 IEMS 主定时器周期触发)
    void edge_watchdog_feed(void) {
        if (g_wd_fd >= 0) {
            int dummy;
            ioctl(g_wd_fd, WDIOC_KEEPALIVE, &dummy);
        }
    }
    
    // 关闭看门狗 (安全退出时调用，写入字符 'V' 告知内核非异常崩溃)
    void edge_watchdog_close(void) {
        if (g_wd_fd >= 0) {
            write(g_wd_fd, "V", 1);
            close(g_wd_fd);
            g_wd_fd = -1;
        }
    }
    ```
