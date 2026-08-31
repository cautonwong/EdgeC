# EdgeOS 二开驱动开发指南 (Custom Driver Example)

本范例演示如何在不修改 EdgeOS SDK 核心源码的前提下，编写一个独立的工业南向驱动（例如太阳能逆变器 `custom_inverter_driver`），并通过 DataHub 影子总线完成遥测写回。

---

## 1. 目录结构
```text
examples/custom_driver/
├── include/
│   └── custom_inverter_driver.h  # 驱动头文件
├── src/
│   └── custom_inverter_driver.c  # 驱动生命周期与轮询实现
├── binding/
│   └── custom_inverter.yaml      # Modbus 测点映射表
├── CMakeLists.txt                # 独立构建规则
└── README.md                     # 本指南
```

---

## 2. 驱动生命周期接口

二开驱动遵循 `edgeos_driver_t` VTable 标准：
```c
#include <edgeos/edgeos.h>

const edgeos_driver_t custom_inverter_driver = {
    .name = "custom_inverter_driver",
    .protocol = "modbus",
    .version = "1.0.0",
    .priv_size = sizeof(custom_inverter_priv_t),
    .on_init = custom_inverter_on_init,
    .on_cleanup = custom_inverter_on_cleanup,
    .on_open = custom_inverter_on_open,
    .on_close = custom_inverter_on_close,
    .is_ready = custom_inverter_is_ready,
};
```

---

## 3. 遥测写回与 DataHub 集成

在驱动轮询回调中，通过标准 SDK 消息构造函数将物理值写入影子总线：
```c
edgeos_pipe_msg_t msg;
pipe_msg_init_scalar(&msg, 1001, voltage, timestamp_ms);
datahub_write_shadow(hub, &msg);
```
