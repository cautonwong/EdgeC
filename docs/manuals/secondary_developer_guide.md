# EdgeOS 嵌入式二次开发工程师手册 (Secondary Developer Guide)

> **版本**：v2.0 (Clean Architecture & Contract-Driven Edition)  
> **适用对象**：基于 EdgeOS / EdgeC 进行工业产品（储能 IEMS、配电融合终端 DCU、微电网网关等）定制、外设驱动编写与边缘算法接入的二次开发工程师。

---

## 目录
1. [架构哲学与核心法则](#1-架构哲学与核心法则)
2. [开发环境与工具链准备](#2-开发环境与工具链准备)
3. [5分钟快速创建新产品脚手架](#3-5分钟快速创建新产品脚手架)
4. [契约驱动开发 (CDD) 规范](#4-契约驱动开发-cdd-规范)
5. [自定义南向驱动开发规范](#5-自定义南向驱动开发规范)
6. [自定义流计算与控制算法接入](#6-自定义流计算与控制算法接入)
7. [跨 SDK 大版本自动迁移指南](#7-跨-sdk-大版本自动迁移指南)
8. [Out-of-Tree 独立编译与 SDK 导出](#8-out-of-tree-独立编译与-sdk-导出)
9. [反熵架构红线与自检清单](#9-反熵架构红线与自检清单)

---

## 1. 架构哲学与核心法则

EdgeOS 采用**整洁架构 (Clean Architecture)** 与 **DDD 领域驱动设计**，严格推行**四平面隔离**与**依赖倒置 (DIP)** 铁律：

```
┌────────────────────────────────────────────────────────────────────────┐
│                        EdgeOS 四平面架构模型                           │
├───────────────────┬────────────────────────────────────────────────────┤
│ 1. Foundation     │ DataHub 内存影子总线 (POSIX SHM) + 契约数据宪法    │
│ 2. Control Plane  │ 状态机仲裁、任务调度、挂钟 MTT 确定性心跳          │
│ 3. Compute Plane  │ 无锁流计算 DAG 管道、DSP/FFT 谐波分析、AI 推理保护 │
│ 4. Execution      │ 南向驱动池 (Modbus/DLT645/CAN/104/DDS)、网络缓冲池 │
└───────────────────┴────────────────────────────────────────────────────┘
```

### 🚨 二开开发者三大铁律：
1. **统一门面引用**：仅允许包含 `<edgeos/edgeos.h>`、`<edgeos/datahub.h>`、`<edgeos/driver.h>` 公共 SDK 头文件，**严禁跨层直接引用内部实现头文件**；
2. **契约驱动 (CDD)**：严禁在 C 源码中硬编码测点 TagID、协议寄存器地址或缩放系数，所有定义必须集中在 YAML 契约中；
3. **零手工 Extern**：驱动注册完全由编译器依据 YAML 自动生成（`edge_driver_registry.c`），无需在产品 `main.c` 手写 `extern ...`。

---

## 2. 开发环境与工具链准备

### 依赖环境
- **操作系统**：Linux (Ubuntu 20.04+, Debian 11+, 统信 UOS, OpenHarmony)
- **编译工具链**：CMake $\ge 3.16$, Ninja $\ge 1.10$, GCC $\ge 9.0$ / Clang $\ge 10.0$
- **二开辅助工具**：`edge-cli`（位于 `.local/linux-x86_64/bin/edge-cli`）与 `contract-compiler`

### 环境变量配置
```bash
export PATH="/path/to/edgeos/EdgeC/.local/linux-x86_64/bin:$PATH"
export EDGEOS_ROOT="/path/to/edgeos/EdgeC"
```

---

## 3. 5分钟快速创建新产品脚手架

使用 `edge-cli create-product` 一键生成符合标准整洁架构的产品应用脚手架：

```bash
edge-cli create-product microgrid_gateway
```

### 生成的标准化工程目录结构：
```
cmd/microgrid_gateway/
├── CMakeLists.txt              # 规范 CMake 构建脚本，通过 edgeos 统一门面链接
├── manifest.yaml               # 模块功能清单
├── prj.conf                    # Kconfig 配置文件
├── main.c                      # 产品入口，仅包含公共 SDK 头文件与优雅退出信号处理
└── README.md                   # 产品说明文档
contracts/products/
└── microgrid_gateway.yaml      # 产品测点契约文件
```

### 编译与运行产品：
```bash
ninja -C build microgrid_gateway
./build/products/microgrid_gateway/bin/microgrid_gateway
```

---

## 4. 契约驱动开发 (CDD) 规范

### 4.1 契约 YAML 结构定义
在 `contracts/products/<product_name>.yaml` 中声明测点与外设绑定：

```yaml
product: "microgrid_gateway"
version: "1.0.0"

# 1. 继承标准领域模型
includes:
  - "contracts/schema/domains/power.yaml"
  - "contracts/schema/domains/storage.yaml"

# 2. 绑定南向物理驱动映射
bindings:
  - driver: "custom_inverter"
    protocol: "modbus"
    device_addr: 1
    registers:
      - reg: 0x0100
        tag: "GRID_VOLTAGE_A"
        scaler: 0.1
        type: "float32"
        unit: "V"
```

### 4.2 契约静态语法校验
提交或编译前，使用 `edge-cli` 进行静态诊断纠错：
```bash
edge-cli lint-contract contracts/products/microgrid_gateway.yaml --strict
```
编译器具备智能 Levenshtein 纠错功能（例如误写 `flot` 自动提示 `Did you mean 'float'?`，误写 `Volts` 自动提示规范化单位 `V`）。

---

## 5. 自定义南向驱动开发规范

### 5.1 快速生成驱动脚手架
```bash
edge-cli create-driver custom_meter --type=serial --protocol=modbus
```

### 5.2 实现 `edge_driver_ops_t` 接口
在 `drivers/` 目录下实现设备驱动 VTable：

```c
#include <edgeos/driver.h>
#include <edgeos/datahub.h>

static int custom_meter_init(edge_driver_t *drv, const void *config) {
    // 1. 初始化串口 / TCP 通信上下文
    return 0;
}

static int custom_meter_read_telemetry(edge_driver_t *drv, edgeos_datahub_t *dh) {
    // 2. 发送查询帧，接收报文并解析
    // 3. 写入 DataHub 影子表 (纳秒级零拷贝写入)
    pipe_msg_t msg = {
        .tag_id = 1001,
        .data_type = DATA_TYPE_FLOAT32,
        .val.f32 = 220.5f,
        .ts = edgeos_get_wall_time_ms(),
    };
    return datahub_write_shadow(dh, &msg);
}

const edge_driver_ops_t g_custom_meter_ops = {
    .init = custom_meter_init,
    .read_telemetry = custom_meter_read_telemetry,
    .destroy = NULL,
};
```

---

## 6. 自定义流计算与控制算法接入

在微电网与储能控制中，算法需要实时感知 DataHub 变化并执行闭环计算：

```c
#include <edgeos/edgeos.h>
#include <edgeos/datahub.h>

void soc_protection_algorithm(edgeos_datahub_t *dh) {
    pipe_msg_t soc_msg = {0};
    // 1. 从 DataHub 影子表 $O(1)$ 快速读取最新 SOC
    if (datahub_read_shadow(dh, TAG_BATTERY_SOC, &soc_msg) == 0) {
        float soc = soc_msg.val.f32;
        if (soc < 10.0f) {
            // 2. 触发过放安全限功率保护
            edge_log_warn("ALGO", "Battery SOC critically low (%.1f%%)! Throttling discharge.", soc);
        }
    }
}
```

---

## 7. 跨 SDK 大版本自动迁移指南

当升级至新版本 EdgeOS SDK 时，使用 `edge-cli migrate` 自动改写废弃 API 与老旧契约语法：

```bash
# 1. 扫描诊断 (只读预览)
edge-cli migrate --dir=cmd/my_custom_product

# 2. 自动安全改写修复
edge-cli migrate --dir=cmd/my_custom_product --fix
```

### 自动处理的迁移规则：
- 自动将旧单数 `include:` 升级为标准 `includes:` 列表；
- 自动将旧头文件 `#include "edgeos.h"` 替换为 `<edgeos/edgeos.h>`；
- 自动将非标工程单位与数据类型别名归一化。

---

## 8. Out-of-Tree 独立编译与 SDK 导出

二次开发工程完全支持脱离 EdgeOS 源码树独立构建：

```cmake
# 二开独立工程 CMakeLists.txt 示例
cmake_minimum_required(VERSION 3.16)
project(my_independent_app C)

find_package(EdgeOS REQUIRED)

add_executable(my_independent_app main.c)
target_link_libraries(my_independent_app PRIVATE EdgeOS::edgeos)
```

---

## 9. 反熵架构红线与自检清单

在提交代码至评审或合并前，请对照以下红线清单自检：

| 检查项 | 约束标准 | 违规后果 |
|---|---|---|
| **物理真实性** | 严禁在入库代码中使用 Hardcoded / Mock 静态占位数据 | 物理穿帮即视为任务彻底失败 |
| **手术级修改** | 严禁随意格式化无关源码或扩大改动行数 | CI Diffstat 门禁拦截 |
| **头文件污染** | 严禁产品和驱动直接包含 `modules/` 内部私有头文件 | 架构符号门禁拦截 |
| **契约一致性** | 测点定义必须在 YAML 声明，严禁在 C 代码出现魔法测点 ID | CDD 编译器校验拦截 |
