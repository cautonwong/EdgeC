# EdgeOS 二开流式计算与调控算法开发指南 (Custom Algorithm Example)

本范例演示如何在不修改 EdgeOS SDK 核心源码的前提下，编写一个独立的边缘算法库（例如电池电量 SOC 平滑滤波算法 `custom_soc_filter`），并通过 DataHub 影子总线完成零拷贝输入读取与计算结果写回。

---

## 1. 目录结构
```text
examples/custom_algo/
├── include/
│   └── custom_soc_filter.h      # 算法头文件
├── src/
│   └── custom_soc_filter.c      # 滤波计算与 DataHub 读写实现
├── CMakeLists.txt               # 独立构建规则
└── README.md                    # 本指南
```

---

## 2. 算法核心逻辑

算法模块实现输入信号的指数滑动平均平滑（EMA）：
$$S_t = \alpha \cdot Y_t + (1 - \alpha) \cdot S_{t-1}$$

并通过公共 SDK 抽象接口直接与 DataHub 进行读写交互：
```c
#include <edgeos/edgeos.h>

int custom_soc_filter_process_datahub(custom_soc_filter_t *filter, edgeos_datahub_t *hub, uint32_t raw_tag_id, uint32_t out_tag_id) {
    edgeos_pipe_msg_t raw_msg;
    if (datahub_read_shadow(hub, raw_tag_id, &raw_msg) != 0) return -1;

    float filtered_val = custom_soc_filter_step(filter, raw_msg.payload.val.f);

    edgeos_pipe_msg_t out_msg;
    pipe_msg_init_scalar(&out_msg, out_tag_id, filtered_val, raw_msg.header.timestamp_ms);
    return datahub_write_shadow(hub, &out_msg);
}
```
