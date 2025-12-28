# EdgeOS Developer Guide

欢迎使用 EdgeOS SDK 进行二次开发。本指南将帮助您构建自己的边缘计算应用。

## 1. 安装 SDK

首先，您需要在开发机上编译并安装 EdgeOS 核心库。

```bash
# 在 EdgeOS 源码根目录
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

这将安装：
*   库文件：`/usr/local/lib/libedge_core_lib.a`
*   头文件：`/usr/local/include/edgeos/*.h` （注意：包含时无需 edgeos/ 前缀）

## 2. 创建您的第一个 APP
...
### 2.2 核心 API 速查

#### 数据读取 (DataHub)
```c
#include "shmring.h"
// 初始化读取消息
...
```

#### 指令下发 (Arbiter)
```c
#include "arbiter.h"
// 构建指令
...
```


## 3. 最佳实践

1.  **非阻塞 I/O**：您的所有业务逻辑运行在 `libuv` 回调中，严禁执行 `sleep()` 或长时间的 `while` 循环。
2.  **内存管理**：SDK 内部大量使用内存池 (`net_buf`)，请尽量复用该机制，避免频繁 `malloc/free`。
3.  **日志规范**：请使用 SDK 提供的日志接口（待开放），以便统一收集上报。

## 4. 调试

运行您的应用时，可以通过设置环境变量开启调试日志：
```bash
export EDGE_LOG_LEVEL=DEBUG
./my_app
```
