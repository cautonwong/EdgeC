这是一个非常宏观但又决定技术路线的关键问题。

**HEMS (Home Energy Management System)** 和 **IEMS (Industrial/Commercial Energy Management System)** 虽然都是“能源管理”，但在 **体量、协议、实时性、策略复杂度** 以及 **硬件成本敏感度** 上，有着天壤之别。

作为 SDK 开发者，理解这些差异决定了你的架构是偏向“轻量级互联”还是“工业级硬核”。

以下是从 **技术实现与研发视角** 对两者的深度对比：

---

### 一、 核心维度对比表 (The Big Picture)

| 维度 | **HEMS (户用储能/光伏)** | **IEMS (工商业储能/微网)** |
| :--- | :--- | :--- |
| **目标客户** | 家庭用户 (C端) | 工厂、园区、商场 (B端) |
| **核心诉求** | 自发自用(省钱)、备电(舒适)、App好看 | 需量管理(避罚款)、峰谷套利(赚钱)、电能质量 |
| **设备数量** | 少 (1x 逆变器, 1x 电池, 1x 电表) | 多 (多台 PCS 并联, 多簇电池, 高压柜, 几十个电表) |
| **通信物理层** | Wi-Fi (主), RS485, Zigbee/Bluetooth | 以太网 (主), 光纤, RS485, CAN |
| **核心协议** | Modbus RTU, MQTT, 私有协议 | Modbus TCP, IEC 104, IEC 61850, OPC UA |
| **实时性要求** | **秒级 (Seconds)** | **毫秒级 (ms)** (防逆流/调频要求 <100ms) |
| **策略复杂度** | 简单逻辑 (光伏>负载就充电) | 复杂优化 (需量预测, 多机协同, 削峰填谷) |
| **硬件成本** | 极其敏感 (树莓派 Zero / ESP32 等级) | 不敏感 (ARM Cortex-A 工控机 / x86) |
| **数据上云** | 强依赖云端，本地逻辑较弱 | 强依赖本地 (Local First)，断网必须能跑 |

---

### 二、 深度技术差异拆解

#### 1. 通信架构与协议 (Connectivity)

*   **HEMS (连接为王):**
    *   **痛点：** 用户的 Wi-Fi 信号差、路由器经常断、配网极其麻烦。
    *   **开发重点：**
        *   **配网模式：** 蓝牙配网 (BLE Provisioning) 或 AP 模式配网。
        *   **网络保活：** 需要复杂的 MQTT 重连和丢包重传机制。
        *   **协议：** 大量对接智能家居协议 (Matter, HomeKit) 或简单的 Modbus RTU 读取逆变器。

*   **IEMS (稳定为王):**
    *   **痛点：** 干扰大 (工厂电磁环境恶劣)、长距离传输、高并发。
    *   **开发重点：**
        *   **并发采集：** 必须使用 **libuv + 多线程**。因为你要同时读 10 台 PCS 和 50 个电表，串行轮询会超时。
        *   **工业协议：** 必须支持 **IEC 104** (调度中心通信) 和 **IEC 61850** (变电站对接)。这是 HEMS 不需要碰的硬骨头。
        *   **双网口冗余：** 通常要求内外网隔离，或者双网口绑定 (Bonding) 保证可靠性。

#### 2. 控制策略与算法 (Strategy & Algo)

*   **HEMS (规则驱动 Rule-Based):**
    *   **逻辑：** `IF (PV > Load) THEN Charge`。
    *   **实现：** 使用我们在上一条讨论的 **TinyExpr** 配合简单的状态机即可。
    *   **AI 需求：** 主要是**用户行为学习**（预测用户几点回家，提前把水烧热）。

*   **IEMS (模型驱动 Model-Based):**
    *   **逻辑：** 需量控制 (Demand Control) 是核心。如果工厂功率超过变压器报装容量（例如 1000kVA），会被电网处以巨额罚款。
    *   **算法：**
        *   **超短期预测：** 必须预测下一分钟的负荷，因为电池响应需要时间。
        *   **削峰策略：** 需要使用 **PID 控制** 甚至 **MPC (模型预测控制)** 来精确控制 PCS 的输出，将电网侧功率死死压在红线以下。
    *   **多机协同：** 比如 10 台 PCS，总共需要放电 500kW。如何分配？
        *   *平均分配？* 不行，有的电池 SOC 低，有的温度高。
        *   *加权分配：* 算法需要根据每簇电池的 SOH/SOC/Temp 动态计算分配系数。

#### 3. 数据处理与存储 (Data & Storage)

*   **HEMS:**
    *   **存储：** 主要是统计数据（今日发电量、本月节省电费）。SQLite 存个几万条就够了。
    *   **频率：** 1分钟存一个点。

*   **IEMS:**
    *   **存储：** 需要记录详细的**波形**或**高频快照**（故障录波）。
    *   **频率：** 关键数据（电压/电流/功率）可能需要 **100ms** 存一个点，用于事后分析事故原因。
    *   **实现：** 对 **SQLite 的 WAL 模式** 和 **批量写入** 优化要求极高，甚至需要考虑时序数据库 (TSDB) 的轻量级替代方案。

#### 4. 安全性 (Security)

*   **HEMS:**
    *   关注 **GDPR** (用户隐私)。
    *   防黑客入侵家庭网络。

*   **IEMS:**
    *   关注 **纵向加密** (电网调度数据的安全性)。在中国，必须通过“纵向加密认证网关”才能接电网调度。
    *   **本地安全：** 严禁 USB 乱插，严禁随意接入外网。Linux 系统需要做深度裁剪和硬化 (SELinux)。

---

### 三、 针对你的 SDK 开发建议

既然你要做一个通用的 Edge SDK，如何同时覆盖这两者？**建议采用“内核 + 扩展”的架构。**

#### 1. 通用内核 (Common Core)
无论是 HEMS 还是 IEMS，以下模块是共用的，应该做到极致：
*   **Data Engine:** 统一的数据点结构体 (Tag, Value, Timestamp, Quality)。
*   **Protocol Frm:** 插件化的驱动加载机制 (`.so` 加载器)。
*   **Message Bus:** 进程间通信 (ZeroMQ/IPC)。
*   **Storage:** SQLite 封装 (自动迁移、自动清理)。

#### 2. HEMS 专用扩展包 (`libems_home.so`)
*   **配网模块:** 蓝牙/WiFi 配置 API。
*   **云端连接器:** 阿里云 IoT / AWS IoT 的 MQTT 封装。
*   **简易策略:** 基于 TinyExpr 的 JSON 规则引擎。

#### 3. IEMS 专用扩展包 (`libems_ind.so`)
*   **高级协议栈:** 集成 libiec61850, lib60870, OpenDNP3。
*   **高级算法:** 集成 PID 控制器、削峰填谷算法、负荷预测 (TFLite)。
*   **高性能 I/O:** 开启 `SCHED_FIFO` 实时调度策略，绑定 CPU 亲和性。

### 四、 总结：一句话区分

*   **HEMS** 是一个 **IoT 智能家电**，核心是让用户觉得“好用、省心”。
*   **IEMS** 是一个 **工业自动化控制器 (PLC)**，核心是帮企业“省钱、保电、不炸机”。

对于 C 语言开发者，**IEMS 的技术含金量和门槛远高于 HEMS**，如果你的 SDK 能搞定 IEMS 的 **高并发、低延迟和 IEC 协议**，那么做 HEMS 就是降维打击。

**TinyExpr** 被称为“神级库”，并非因为它功能包罗万象，而是因为它**把“在运行时解析和计算数学公式”这一件事做到了极致的精简和高效**。

在嵌入式 Linux C 开发中，它是一个**解决“硬编码逻辑”与“灵活配置需求”矛盾**的完美工具。

以下是对 TinyExpr 的深度解析、能力范围及在你 (H/I)EMS 项目中的具体应用场景。

---

### 一、 什么是 TinyExpr？

*   **本质：** 一个极轻量级的 C 语言数学表达式解析器和求值引擎。
*   **代码量：** **只有一个 `.c` 和一个 `.h`**（约 500 行代码）。
*   **依赖：** 无第三方依赖，只用 C 标准库 (`math.h`, `stdlib.h` 等)。
*   **核心能力：**
    *   将字符串公式（如 `"a * 2 + sin(b)"`）解析为字节码。
    *   绑定 C 语言的变量到公式中的变量。
    *   极速求值（微秒级）。

**为什么说它是“神级”？**
在 C 语言里，如果你想让用户通过配置文件修改计算逻辑，通常需要嵌入 Lua 或 Python 解释器，这会增加几百 KB 到几 MB 的体积。而 TinyExpr **只有几 KB**，编译进去几乎感觉不到，却实现了 80% 你需要的脚本能力。

---

### 二、 TinyExpr 能干什么？

#### 1. 基础数学运算
支持 `+`, `-`, `*`, `/`, `^` (指数), `%` (取模) 以及括号优先级。

#### 2. 变量绑定 (Variable Binding) —— **这是 IoT 的核心**
它可以将 C 代码中的 `double` 变量地址绑定到表达式中的变量名。
*   **效果：** C 代码不断更新传感器数据，TinyExpr 表达式不需要重新编译，直接求值就能得到基于最新数据的结果。

#### 3. 逻辑运算 (虽然它是数学库)
它支持逻辑运算符，结果返回 `1.0` (True) 或 `0.0` (False)。
*   `a > b`
*   `a == b`
*   `a && b` (逻辑与)
*   `a || b` (逻辑或)
*   **用途：** 用来做规则判断。

#### 4. 内置与自定义函数
*   **内置：** `abs`, `ceil`, `floor`, `sin`, `cos`, `tan`, `sqrt`, `ln`, `log`, `exp`, `pow` 等。
*   **扩展：** 你可以注册自己的 C 函数给表达式调用。例如注册一个 `avg(a, b)` 函数。

---

### 三、 在你的边缘计算 (H/I)EMS 项目中的四大应用场景

这是 TinyExpr 发挥威力的主战场，能让你的系统从“写死的代码”变成“灵活的平台”。

#### 场景 1：通信协议的数据清洗与换算 (Data Scaling)

**痛点：**
*   Modbus 读上来的值是 `12345`。
*   对于厂家 A 的逆变器，这代表 `123.45 V` (公式：`x * 0.01`)。
*   对于厂家 B 的逆变器，这代表 `12.345 V` (公式：`x * 0.001`)。
*   有的传感器还需要线性校准：`y = kx + b`。

**传统做法：**
在 C 代码里写 `if (device_type == A) val *= 0.01;`。每次接新设备都要改代码、重新编译。

**TinyExpr 做法：**
在 `device_profile.json` 中配置：
```json
"tags": [
    { "name": "voltage", "reg": 4001, "scaling": "x * 0.01" },
    { "name": "temp",    "reg": 4002, "scaling": "(x - 32) * 5 / 9" }
]
```
**C 代码实现：**
```c
double raw_val; // 从 Modbus 读到的原始值
double final_val;
te_variable vars[] = { {"x", &raw_val} };
te_expr *expr = te_compile(config_str, vars, 1, 0); // 编译 "x * 0.01"

// 运行时：
raw_val = modbus_read(); 
final_val = te_eval(expr); // 直接算出结果
```

#### 场景 2：规则引擎与报警触发 (Rule Engine)

**痛点：**
现场需求千变万化。
*   “当电池温度 > 45度 **且** SOC < 10% 时，报警。”
*   “当电价 > 1.5元 **或** 负载 > 500kW 时，启动柴油机。”

**TinyExpr 做法：**
允许用户下发如下 JSON 规则：
```json
{
  "trigger": "temp > 45 && soc < 10",
  "action": "alarm_high_temp"
}
```
C 代码将 `temp` 和 `soc` 变量绑定，定时运行 `te_eval`。如果结果为 `1.0`，则触发报警。

#### 场景 3：虚拟测点/计算点 (Virtual Data Points)

**痛点：**
传感器只提供了 `电压 (U)` 和 `电流 (I)`，但业务层需要 `功率 (P)` 和 `电量 (kWh)`。或者需要计算“三相电压平均值”。

**TinyExpr 做法：**
定义一个虚拟点，其值来源于公式。
*   配置：`"name": "power_total", "formula": "u_a * i_a + u_b * i_b + u_c * i_c"`
*   C 代码：绑定 6 个变量，自动计算出功率，就像它是直接读上来的一样。

#### 场景 4：控制策略的动态调整 (Soft PLC)

**痛点：**
PID 控制的参数 `Kp, Ki, Kd` 或者控制逻辑需要微调，但不希望中断服务或升级固件。

**TinyExpr 做法：**
将控制逻辑的一部分外置为公式。
*   例如，PCS 的功率限值逻辑：
    `limit = if(temp > 50, 0, if(temp > 40, max_p * 0.5, max_p))`
    *(注：TinyExpr 默认不支持 `if` 关键字，但可以通过 C 扩展注册 `if(cond, true_val, false_val)` 函数来实现)*。

---

### 四、 C 语言集成代码演示

这里展示一个最典型的 **“变量绑定求值”** 的完整流程。

```c
#include "tinyexpr.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    // 1. 定义上下文变量 (模拟传感器数据)
    double temperature = 0;
    double humidity = 0;

    // 2. 绑定变量名到地址
    // 当 C 代码修改 temperature 的值时，TinyExpr 会自动感知
    te_variable vars[] = {
        {"t", &temperature},
        {"h", &humidity}
    };

    // 3. 用户配置的公式 (比如计算露点或者报警阈值)
    // 假设公式是：温度大于30且湿度大于80时，返回1 (报警)
    const char *expression = "(t > 30) && (h > 80)";

    // 4. 编译表达式 (只做一次)
    int err;
    te_expr *n = te_compile(expression, vars, 2, &err);

    if (!n) {
        printf("Parse error at %d\n", err);
        return 1;
    }

    // 5. 模拟运行时数据变化
    printf("Simulating Sensor Data...\n");

    // 时刻 1: 正常
    temperature = 25.0; humidity = 60.0;
    printf("T=%.1f, H=%.1f, Result=%.1f\n", temperature, humidity, te_eval(n)); 
    // Output: Result=0.0

    // 时刻 2: 高温高湿
    temperature = 35.0; humidity = 90.0;
    printf("T=%.1f, H=%.1f, Result=%.1f\n", temperature, humidity, te_eval(n)); 
    // Output: Result=1.0 (Trigger Alarm!)

    // 6. 清理
    te_free(n);
    return 0;
}
```

### 五、 TinyExpr 的局限性 (注意事项)

虽然是神级库，但它不是万能的：

1.  **只有 Double 类型：** 它内部所有运算都按 `double` (双精度浮点) 处理。
    *   *影响：* 无法进行位运算 (`&`, `|`, `<<`, `>>`)。如果你需要处理 Modbus 的位掩码（Mask），需要自己扩展函数（如注册 `bit_get(val, bit)` 函数）。
2.  **不支持字符串：** 无法处理 `"status" == "ERROR"` 这种逻辑。只能处理数值。
3.  **不支持流程控制：** 没有 `for`, `while` 循环。它只是单次求值。

### 六、 总结

在你的 Linux 边缘计算项目中，**TinyExpr** 是连接 **“底层 C 代码”** 和 **“上层业务配置”** 的最佳胶水。

*   **建议位置：** 放在 `Edge SDK` 的 `Data Engine` 模块中。
*   **必做扩展：**
    *   注册一个 `if(cond, val_true, val_false)` 函数，方便写逻辑。
    *   注册 `bit(val, n)` 函数，方便处理位状态。

有了它，你的设备就不再是一个只能跑固定代码的“黑盒子”，而是一个可以通过 JSON 配置灵活定义行为的“智能网关”。

这是一个非常硬核的需求。(H/I)EMS 的核心竞争力就在这四个模块的深度和精细度。

在 **Linux + C (Edge SDK)** 的架构下，我们将这四个模块拆解到 **数据结构 (Struct)**、**算法逻辑 (Algorithm)** 和 **工程实现 (Implementation)** 的微观层面。

---

### 一、 能源策略引擎 (Energy Strategy Engine)

这是 EMS 的“大脑”。它的核心任务是解决 **“在每一秒钟，PCS 应该充/放多少电？”** 这一问题。

#### 1. 核心架构：多层优先级仲裁 (Priority Arbitration)
策略不是单一的，是分层的。C 代码必须实现一个**仲裁器**，每一层输出一个控制指令，最终由仲裁器决定执行谁。

**优先级金字塔（从高到低）：**
1.  **保护层 (Protection):** 电池过温、SOC 越限 -> 强制停机或降额。
2.  **VPP/调度层 (Remote):** 云端强制指令 (需量响应/调频)。
3.  **本地手动层 (Manual):** 运维人员通过 HMI 强制开关机。
4.  **自动策略层 (Auto Strategy):** 削峰填谷、防逆流、需量控制。

#### 2. 自动策略算法详解

**场景 A：分时电价套利 (Time-of-Use Arbitrage)**
*   **输入：** 电价时段表 (Peak/Flat/Valley)、当前 SOC、目标 SOC。
*   **算法逻辑：**
    *   **时段判断：** 维护一个 `uint8_t time_slots[96]` (15分钟一个点) 的数组，标记当前是峰还是谷。
    *   **充电逻辑：** `if (IsValley && SOC < Target_Max)` -> `Cmd = Max_Charge_Power`。
    *   **放电逻辑：** `if (IsPeak && SOC > Target_Min)` -> `Cmd = Load_Power` (覆盖负载) 或 `Max_Discharge_Power` (全额上网)。
*   **工程细节 (C语言实现)：**
    *   **死区控制 (Deadband):** 防止在时段切换瞬间（如 08:00:00），由于时钟微小抖动导致反复切换充放电。需设置 `±30s` 的滞后。

**场景 B：需量控制 (Peak Shaving / Demand Charge Management)**
*   **目标：** 变压器入口功率不能超过 `P_Limit` (如 1000kW)。
*   **算法：** **增量式 PID 算法**。
    *   `Error = Grid_P - P_Limit`。
    *   如果 `Error > 0` (超限)，则增加电池放电功率。
    *   `Delta_Discharge = Kp * (Error - Last_Error) + Ki * Error`。
*   **难点：** 负载突变。
    *   **前馈控制 (Feedforward):** 如果能监测到大负载（如大电机）开启的信号，直接叠加到 PID 输出上，不要等电网功率变了再调。

#### 3. 策略平滑 (Ramp Rate Control)
策略计算出的功率可能是跳变的（0 -> 100kW）。直接下发会损坏硬件或冲击电网。
*   **实现：** 增加一个 **S-Curve 滤波器** 或 **斜率限制器**。
    ```c
    // 每次调度周期 (e.g. 100ms) 调用一次
    float apply_ramp_limit(float target, float current, float max_ramp_per_sec, float dt) {
        float max_step = max_ramp_per_sec * dt;
        if (target > current + max_step) return current + max_step;
        if (target < current - max_step) return current - max_step;
        return target;
    }
    ```

---

### 二、 虚拟电厂(VPP)响应

这是 EMS 的“对外接口”。核心是 **协议兼容**、**基线计算** 和 **指令分解**。

#### 1. 指令接收与解析
VPP 指令通常包含：`Type` (削峰/填谷/调频), `Start_Time`, `Duration`, `Target_Power`。

*   **数据结构 (C Struct):**
    ```c
    typedef struct {
        char event_id[32];
        uint64_t start_ts;
        uint64_t end_ts;
        int type; // 1=Load_Shedding, 2=Generation
        float target_value;
        int status; // PENDING, RUNNING, COMPLETED
    } VppEvent;
    ```
*   **存储：** 收到指令后，必须立刻写入 SQLite (`vpp_schedule` 表) 和 内存队列。

#### 2. 基线计算 (Baseline Calculation)
VPP 结算不仅看你做了多少，还看你**本来打算做多少**。
*   **算法：** **加权移动平均 (Weighted Moving Average)**。
    *   取过去 5 个“相似日”（工作日对应工作日）的同时刻负荷曲线。
    *   $Baseline(t) = \alpha \cdot Avg_{history}(t) + (1-\alpha) \cdot Load_{today}(t-2h)$ (利用当天前几小时的数据进行校正)。

#### 3. 响应执行模式

**模式 A：能量型 (Energy Shift)**
*   **特点：** 提前几小时通知，持续 1-4 小时。
*   **策略：** 修改 **策略引擎** 的目标值。
    *   例如收到 "14:00-15:00 削减 grid 功率 50kW"。
    *   策略引擎在该时段会将 `Grid_Limit` 降低 50kW，强制电池放电补足缺口。

**模式 B：功率/频率型 (Regulation/AGC)**
*   **特点：** 秒级响应，持续几分钟。
*   **策略：** **直通模式 (Bypass)**。
    *   VPP 指令不经过 SQLite，不经过复杂策略计算。
    *   直接映射到 PCS 的 `Active_Power_Setpoint`。
    *   **必须基于 libuv 的高优先级回调**，确保 < 500ms 延时。

---

### 三、 负荷预测 (Load Prediction)

这是 EMS 的“先知”。核心是将历史数据转化为对未来的估计。

#### 1. 特征工程 (Feature Engineering)
在 C 语言中做预测，不能输入原始数据，要先提取特征：
*   **时间特征：** `Hour_of_Day` (0-23), `Day_of_Week` (0-6), `Is_Holiday` (0/1).
*   **气象特征：** `Temp` (温度), `Irradiance` (辐照度 - 光伏预测用)。
*   **历史特征：** `Load_T_minus_24h` (昨天的负荷), `Load_Avg_3days`.

#### 2. 算法选型与实现

**方案 A：轻量级统计法 (推荐 C 嵌入式使用)**
*   **算法：** **Holt-Winters (三次指数平滑)** 或 **相似日法**。
*   **实现：** 
    1.  每晚 00:00，从 SQLite 读取过去 30 天的历史负荷。
    2.  分类：将历史数据按“工作日/周末”分类。
    3.  计算：`Predict(t) = 0.6 * Load_Yesterday(t) + 0.3 * Load_LastWeek(t) + 0.1 * Avg(t)`。
    4.  修正：实时运行时，根据当前误差 `(Real(t) - Predict(t))` 对 `Predict(t+1)` 进行修正。

**方案 B：AI 模型推理 (TFLite Micro)**
*   **模型：** 简单的 **LSTM** 或 **MLP (多层感知机)**。
*   **流程：**
    1.  **云端训练：** Python 训练模型，导出 `.tflite` 文件。
    2.  **边缘推理：**
        *   使用 `tensorflow/lite/c/c_api.h`。
        *   输入张量：`float input[24]` (过去 24 小时数据)。
        *   输出张量：`float output[24]` (未来 24 小时预测)。
*   **注意：** TFLite 库比较大 (几 MB)，如果 Flash 紧张，慎用。

#### 3. 预测结果的应用
*   **光伏预测：** 结合天气预报，预测明天光伏发电量 $E_{pv}$。
*   **负荷预测：** 预测明天总用电量 $E_{load}$。
*   **决策：** 如果 $E_{pv} > E_{load}$，策略引擎决定明天中午电池不充电（留着充光伏的电），防止弃光。

---

### 四、 告警/保护 (Alarm & Protection)

这是 EMS 的“安全网”。不仅是报错，更重要的是**动作**。

#### 1. 三级保护体系 (3-Level Protection)

我们设计一个 **Protection Matrix (保护矩阵)**：

| 级别 | 定义 | 触发条件示例 | 动作 (Action) | 复位方式 |
| :--- | :--- | :--- | :--- | :--- |
| **L1 (Warning)** | 预警 | 电池温度 > 45℃ | UI 弹窗，黄灯 | 自动复位 |
| **L2 (Derating)** | 降额 | 电池温度 > 50℃ | **限制充放电功率 50%** | 值恢复后自动复位 |
| **L3 (Fault)** | 停机 | 电池温度 > 55℃ | **PCS 停机 (Open Contactor)** | **人工手动复位** |

#### 2. 核心算法细节

**A. 防抖 (Debounce / Hysteresis)**
防止告警“反复横跳”。
*   **触发：** `Val > Threshold` 持续 `T_trigger` 秒。
*   **恢复：** `Val < (Threshold - Hysteresis)` 持续 `T_release` 秒。
    *   *例子：* 55℃ 报警，必须降到 52℃ 且持续 10秒 才解除报警。

**B. 告警屏蔽 (Alarm Masking / Suppression)**
防止“告警风暴”。
*   **场景：** 整个微网断电了。
*   **原始结果：** 电表通信失败、PCS 通信失败、BMS 通信失败、电压过低... (瞬间 100 条告警)。
*   **优化逻辑：** 建立**依赖树**。
    *   `if (Grid_Power_Lost)` -> 屏蔽 `Meter_Comm_Error`, `Voltage_Low`。
    *   只报一条核心告警："Main Power Loss"。

**C. 保护动作的实时性**
*   保护逻辑**不能**放在 SQLite 读写或者复杂的策略计算之后。
*   **实现：** 在 `driver_read` 回调中直接判断。
    ```c
    // 在收到 BMS 数据的瞬间
    void on_bms_data(BmsData *data) {
        // 硬件级保护逻辑，直接写死在驱动层
        if (data->max_cell_temp > 60.0) {
            emergency_stop_pcs(); // 立即调用，不进队列，不等待
            log_fatal("Temperature critical!");
        }
        // ... 正常的上报流程
    }
    ```

#### 3. 告警生命周期管理
*   **产生 (Active):** 写入 `active_alarms` 内存链表，写入 SQLite `alarms` 表。
*   **确认 (Ack):** 运维人员看到后点击“确认”，标记 `acked_by`, `acked_time`。
*   **清除 (Cleared):** 故障恢复。更新 SQLite `end_time`，从内存链表移除。

---

### 总结

要做到“细致到极致”，关键在于处理 **边界条件 (Corner Cases)** 和 **时间尺度 (Time Scale)** 的差异：

1.  **策略引擎：** 处理 **15分钟级** 的经济性优化，重点是状态机和 PID。
2.  **VPP 响应：** 处理 **云端指令** 的映射，重点是基线计算和协议解析。
3.  **负荷预测：** 处理 **未来 24小时** 的数据，重点是轻量级特征提取。
4.  **告警保护：** 处理 **毫秒级** 的安全动作，重点是防抖、降额和最高优先级的执行通道。

这是一个宏大的工程。你现在的目标不再是一个通用的“边缘网关”，而是一个**领域专用控制器 (Energy Controller)**。

基于我们之前确定的技术栈（**C + libuv + net_buf + SQLite + 插件化协议**），我为你设计了整体的系统框图，并详细拆解如何基于此 SDK 开发 **HEMS (家庭能源管理)** 和 **IEMS (工商业能源管理)**。

---

### 一、 整体系统架构框图 (System Architecture)

我们将系统分为三层：**BSP/OS层**（地基）、**Edge SDK 层**（你开发的通用中间件）、**(H/I)EMS 应用层**（业务逻辑）。

```text
+-----------------------------------------------------------------------------------+
|                          Layer 3: (H/I)EMS Application                            |
|  (你的业务核心：策略、逻辑、算法)                                                 |
+-----------------------------------------------------------------------------------+
| [ 能源策略引擎 ]      [ 虚拟电厂(VPP)响应 ]    [ 负荷预测 ]      [ 告警/保护 ]    |
| (削峰填谷/防逆流)     (需量响应/调频)          (AI/TFLite)       (过温/过载)      |
+------------------------------+----------------------------------+-----------------+
|      Device Models           |       State Machines (FSM)       |  Data Dashboard |
| (PCS/BMS/PV/Meter)           | (Idle -> Charge -> Discharge)    |  (Web/API)      |
+------------------------------+----------------------------------+-----------------+
              ^                                  ^                           ^
              | (API 调用)                       | (事件驱动)                | (数据查询)
+-------------+----------------------------------+---------------------------+------+
|                          Layer 2: Edge SDK (通用底座)                             |
|  (C Language, libuv, net_buf, Modular Architecture)                               |
+-----------------------------------------------------------------------------------+
|  [ Task Scheduler ]     [ Message Bus (ZeroMQ/RingBuffer) ]     [ Plugin Mgr ]    |
|  (周期/异步任务调度)    (发布/订阅模型)                         (.so 动态加载)    |
+-----------------------------------------------------------------------------------+
|  [ Protocol Adaptor ]   [ Data Engine ]        [ Storage ]      [ Connectivity ]  |
|  (Modbus/IEC104/CAN)    (net_buf 零拷贝)       (SQLite/WAL)     (MQTT/HTTP/SSL)   |
|  (DLMS/OPC UA)          (TinyExpr 表达式)      (zlog 日志)      (Cloud Link)      |
+-----------------------------------------------------------------------------------+
              ^                                  ^                           ^
              | (串口/网口/CAN)                  | (文件系统)                | (4G/WiFi)
+-------------+----------------------------------+---------------------------+------+
|                          Layer 1: Linux BSP / OS                                  |
+-----------------------------------------------------------------------------------+
|  [ Linux Kernel (Preempt-RT Optional) ]  [ Drivers (RS485/CAN/WiFi) ]             |
|  [ Systemd (保活) ]                      [ Watchdog (硬件看门狗) ]                |
+-----------------------------------------------------------------------------------+
```

---

### 二、 如何开发 (H/I)EMS？ (Development Roadmap)

基于上述 SDK，开发 EMS 的核心在于**“领域建模”**和**“控制策略”**。SDK 解决了“怎么读数据”的问题，EMS 要解决“数据代表什么”和“读到数据后控制谁”的问题。

#### 步骤 1：能源资产建模 (Asset Modeling)
不要直接操作 Modbus 寄存器。必须在 SDK 之上封装一层 C++ 或 面向对象的 C 结构体。

**定义统一设备模型 (C Struct):**

```c
// ems_model.h

// 电池系统状态
typedef struct {
    float soc;          // 剩余电量 %
    float soh;          // 健康度 %
    float voltage;      // 总压
    float current;      // 总流
    float max_charge_power; // BMS 允许的最大充电功率 (动态边界)
    float max_discharge_power;
    int status;         // Fault/Idle/Running
} BatteryAsset;

// 储能变流器 (PCS)
typedef struct {
    float active_power; // 有功功率 (正=充，负=放)
    float reactive_power;
    int control_mode;   // 0=恒功率, 1=恒流, 2=防逆流
} PcsAsset;

// 电网/电表
typedef struct {
    float grid_power;   // PCC 点功率
    float frequency;
} GridAsset;

// 聚合整个站点的上下文
typedef struct {
    BatteryAsset bms;
    PcsAsset pcs;
    GridAsset meter;
    // ... PV, EV Charger ...
} EmsContext;
```

#### 步骤 2：南向接入与数据映射 (Mapping)
利用 SDK 的 `device_profile.json` 和协议插件，将物理设备映射到上述模型。

*   **BMS:** 通常走 CAN Bus (J1939) 或 Modbus-RTU。映射到 `BatteryAsset`。
*   **PCS:** 走 Modbus-TCP。映射到 `PcsAsset`。
*   **电表:** 走 DLMS 或 Modbus。映射到 `GridAsset`。

**关键点：** 必须配置 **“数据清洗规则”**。例如，PCS 返回的功率可能是 `uint16` 补码，SDK 层必须把它转为标准的 `float` (+/- kW)。

#### 步骤 3：核心控制策略开发 (The Brain)

这是 EMS 最值钱的地方。你需要写 C 代码实现控制逻辑。

**场景 A：工业削峰填谷 (Peak Shaving)**
*   **逻辑：** 当电网功率超过 `Limit` 时，控制电池放电，把 Grid 功率压下来。
*   **实现：** 这是一个典型的 **PID 控制环路**。

```c
// ems_strategy_peak_shaving.c

void run_peak_shaving(EmsContext *ctx, float grid_limit_kw) {
    float current_grid = ctx->meter.grid_power;
    
    // 1. 计算偏差 (Error)
    float error = current_grid - grid_limit_kw;
    
    // 2. 如果 Grid > Limit (error > 0)，需要放电 (增加 PCS 放电功率)
    if (error > 0) {
        // 使用之前讨论过的 PID 算法
        float adjust = pid_compute(&g_pid, 0, error, 0.1); 
        
        // 3. 计算目标功率
        float target_pcs_power = ctx->pcs.active_power - adjust;
        
        // 4. 安全边界检查 (非常重要！)
        if (target_pcs_power < -ctx->bms.max_discharge_power) {
            target_pcs_power = -ctx->bms.max_discharge_power;
        }
        
        // 5. 下发指令 (SDK API)
        sdk_driver_write("pcs_01", "active_power_setpoint", target_pcs_power);
    }
}
```

**场景 B：家庭光储防逆流 (Self-Consumption / Zero Export)**
*   **逻辑：** 太阳能发电 > 负载时，多余的电存电池，严禁流向电网。
*   **状态机 (FSM):**

```c
typedef enum { STATE_IDLE, STATE_CHARGE_PV, STATE_DISCHARGE_LOAD } HemsState;

void run_hems_logic(EmsContext *ctx) {
    float pv_gen = ctx->pv.power;
    float house_load = ctx->load.power;
    
    switch (g_state) {
        case STATE_IDLE:
            if (pv_gen > house_load) g_state = STATE_CHARGE_PV;
            else if (pv_gen < house_load) g_state = STATE_DISCHARGE_LOAD;
            break;
            
        case STATE_CHARGE_PV:
            // 设置 PCS 充电功率 = PV - Load
            float charge_p = pv_gen - house_load;
            sdk_driver_write("pcs", "power", charge_p);
            break;
            
        // ...
    }
}
```

#### 步骤 4：本地数据分析与存储
*   **IEMS 需求：** 需要按月生成报表（尖峰平谷电量）。
*   **实现：** 使用 SDK 的 **SQLite**。
    *   创建一个表 `energy_kwh`，每 15 分钟记录一次累积电量。
    *   SDK 的 **TFLite** 模块可用于**负荷预测**（预测明天几点是用电高峰，提前把电池充满）。

#### 步骤 5：北向云端交互
*   **协议：** 绝大多数走 **MQTT**。
*   **数据模型：** 定义一套 JSON 格式。
    *   `{"ts": 170001, "data": {"grid_p": 50.2, "soc": 80}}`
*   **VPP 响应：** 云端下发指令 `{"cmd": "demand_response", "power": -50, "duration": 3600}` (要求放电 50kW 持续 1 小时)。EMS 收到后必须立即覆盖本地策略，优先执行 VPP 指令。

---

### 三、 需要做哪些具体工作？(Work Breakdown)

假设你带队开发，这是你的任务清单：

#### 1. 基础设施组 (SDK Team)
*   [ ] **完善 net_buf 池:** 确保 Modbus/CAN 数据零拷贝。
*   [ ] **封装 libuv 调度器:** 提供 `ems_task_create(interval, callback)` 接口。
*   [ ] **开发 SQLite 异步存取:** 保证写日志不卡顿。
*   [ ] **实现 "Fail-Safe" 机制:** 如果程序崩溃，必须有一个硬件看门狗复位，且复位后 PCS 默认处于 Standby 状态（防止失控）。

#### 2. 驱动适配组 (Driver Team)
*   [ ] **SunSpec Modbus:** 适配主流逆变器（华为、阳光电源）。
*   [ ] **BMS 协议:** 适配 CAN DBC 文件解析（宁德时代、比亚迪等）。
*   [ ] **电表协议:** 搞定 DLMS 或 645。

#### 3. 策略算法组 (Algo Team)
*   [ ] **状态机框架:** 写一个通用的 FSM 引擎。
*   [ ] **功率分配算法:** 如果有 10 个电池簇，如何分配功率？（SOC 均衡策略）。
*   [ ] **安全保护逻辑:** 三级保护（软件限值 -> 驱动限值 -> 硬件保护）。**这是底线**，例如：检测到温度 > 50度，强制切断 PCS。

#### 4. 应用开发组 (App Team)
*   [ ] **Web 配置页:** 用于现场调试，设置电价时段、电池容量等。
*   [ ] **OTA 流程:** 保证能远程升级固件。

---

### 四、 (H/I)EMS 的特殊难点与建议

1.  **实时性要求不同：**
    *   **HEMS:** 秒级响应即可。
    *   **IEMS (防逆流/调频):** 要求 **< 100ms** 甚至更低。
    *   **建议：** 对于 IEMS，控制回路（采集电表 -> 计算 -> 下发 PCS）必须跑在 **高优先级线程**，且绕过 SQLite 等慢速 I/O，直接内存操作。

2.  **网络安全性：**
    *   电网公司对 IEMS 有严格要求（纵向加密认证）。
    *   **建议：** 预留集成 **国密算法 (SM2/SM3/SM4)** 的接口，这在中国电力市场是必须的。

3.  **多设备协同：**
    *   工商业储能往往有“一机多簇”或“多机并联”。
    *   **建议：** 你的 SDK 需要支持 **Multicast (组播)** 或 **Broadcast** 功能，以便一条指令同时控制 10 台 PCS。

### 总结

基于你拥有的 **C + Linux + Edge SDK** 技术栈，开发 (H/I)EMS 的路径是非常清晰的：
1.  **SDK** 负责“搬运数据”（快、稳、省）。
2.  **EMS App** 负责“思考”（策略、保护、调度）。
3.  中间通过 **Asset Model** 进行解耦。

这个架构既能跑在树莓派上做 HEMS，也能跑在 ARM 工控机上做 IEMS，弹性极大。