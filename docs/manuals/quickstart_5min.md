# EdgeOS 5分钟极简上手指南 (5-Minute Quickstart)

> 目标：跟随本指南，您将在 5 分钟内从零创建、配置并运行一个基于 EdgeOS 工业微电网网关（`microgrid_box`）产品！

---

## ⏱️ 第 1 分钟：创建产品脚手架
在 EdgeOS 工程根目录下，运行脚手架生成命令：

```bash
.local/linux-x86_64/bin/edge-cli create-product microgrid_box
```

终端将输出成功提示，并自动创建：
- `contracts/products/microgrid_box.yaml` (测点契约)
- `cmd/microgrid_box/main.c` (产品入口)
- `cmd/microgrid_box/CMakeLists.txt` (CMake 配置)
- `cmd/microgrid_box/prj.conf` (Kconfig 配置)

---

## ⏱️ 第 2 分钟：定义微电网测点契约
编辑 `contracts/products/microgrid_box.yaml`，声明电压、电流与告警测点：

```yaml
product: "microgrid_box"
version: "1.0.0"

includes:
  - "contracts/schema/domains/power.yaml"

tags:
  - id: 1001
    name: "MICROGRID_GRID_VOLTAGE"
    type: "float32"
    unit: "V"
    desc: "Main grid bus voltage"
  - id: 1002
    name: "MICROGRID_BATTERY_SOC"
    type: "float32"
    unit: "%"
    desc: "Energy storage state of charge"
```

---

## ⏱️ 第 3 分钟：语法校验与智能诊断
运行 Linter 工具校验契约 YAML：

```bash
.local/linux-x86_64/bin/edge-cli lint-contract contracts/products/microgrid_box.yaml --strict
```
输出 `✅ Lint PASSED: 0 errors, 0 warnings` 即代表契约合法无冲突！

---

## ⏱️ 第 4 分钟：编译产品可执行文件
使用 Ninja 编译新产品：

```bash
ninja -C build microgrid_box
```

构建系统将自动调用 `contract-compiler` 编译测点查表库，并完成链接。

---

## ⏱️ 第 5 分钟：启动并运行产品
以单次自检模式运行新产品：

```bash
EDGEOS_RUN_ONCE=1 ./build/products/microgrid_box/bin/microgrid_box
```

终端将打印：
```text
=========================================================
🚀 Starting EdgeOS Product: microgrid_box (SDK v2.0.0)
=========================================================
[+] Generated driver registry initialized (result: 0, drivers: 0)
[+] Product microgrid_box is ready and operational.
[+] EDGEOS_RUN_ONCE mode: exiting cleanly.
[-] Product microgrid_box stopped cleanly.
```

🎉 **祝贺您！您已成功构建并运行了首个 EdgeOS 工业微电网产品！**
更多进阶开发（自定义驱动、边缘算法、DDS 网状通信），请参阅 [《EdgeOS 嵌入式二次开发工程师手册》](secondary_developer_guide.md)。
