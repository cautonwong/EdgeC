# EdgeOS Contract Compiler (Golang v4.5-PerfectHash-Secure)

EdgeOS 契约编译器是全系统的“数据宪法”执行引擎。它负责将多层级 YAML 定义（Tags & Bindings）编译为具有 **O(1) 性能**和**工业级安全性**的 C 语言查表代码。

## 1. 核心技术突破 (Technical Breakthroughs)

### 🚀 O(1) 完美哈希查找 (Perfect Hashing)
编译器不再生成低效的二分查找或线性扫描代码，而是利用 **MurmurHash3 Mixer** 算法为每个产品生成定制的完美哈希表。
*   **性能**：无论点表规模（10个还是10,000个），测点检索耗时均为恒定纳秒级。
*   **抗碰撞**：针对工业地址（如 DLT645 的 DI）高位集中、低位多零的特性，引入位混合扰动，确保哈希分布均匀。

### 🛡️ 工业安全加固 (Safety Hardening)
*   **物理键校验**：生成的 `edge_lookup_xxx_num` 接口包含物理地址（Addr/DI/IOA）二次比对，彻底杜绝哈希伪命中（False Hit）导致的工业误报。
*   **编译期冲突检测**：如果在 YAML 中误将两个 Tag 映射到同一个物理地址，编译器将触发 `panic` 报错，拒绝产生带病固件。
*   **自适应特性门控**：自动为所有 Tag 生成 `TAG_XXX_ENABLED` 宏和枚举，确保全产品线代码编译一致性。

### 🩺 逻辑闭环自检 (Self-Consistency Test)
编译器会自动生成 `edge_contract_self_test()` 函数。该函数在产品启动时：
1.  遍历 YAML 定义的所有原始映射。
2.  调用 C 语言查找接口进行反查。
3.  验证 ID 与 **Scaler (缩放系数)** 是否完全匹配。
**这实现了“配置即真相”的最后 1 厘米验证。**

---

## 2. 语义规范 (Semantic Standards)

为保持工业规约的一致性，系统已完成语义归一化：
*   **缩放系数**：统一使用 **`scaler`** 关键字（替代旧版的 `scale`）。
*   **物理地址**：统一使用 **`addr`**（Modbus）、**`di`**（DLT645）、**`ioa`**（IEC104）。

---

## 3. YAML 覆盖优先级 (Override Hierarchy)

| 优先级 | 文件类型 | 核心职责 |
| :--- | :--- | :--- |
| **1 (最高)** | `extension.yaml` | **现场真相**：修正缩放系数、物理地址偏移、本地测点扩展 |
| **2** | `product.yaml` | **产品定义**：选择开启哪些点位 (`enabled_tags`)、UI 品牌化配置 |
| **3** | `includes/*.yaml` | **协议库**：设备厂商的标准寄存器/DI 映射模板 |
| **4 (最低)** | `tags.yaml` | **全局字典**：定义测点标准 ID、单位和语义 |

---

## 4. 扩展新协议 (Adding New Protocols)

编译器采用插件化设计，扩展新协议仅需三步：
1.  在 `protocols/` 实现 `ProtocolHandler` 接口。
2.  在 `core/hash_gen.go` 的 `ParseProtocolKey` 中定义该协议的数字解析逻辑。
3.  在 `templates/master_c.tmpl` 中添加对应的 C 代码生成逻辑。

---

## 5. 构建与集成

### 编译编译器
```bash
go build -o contract-compiler .
```

### 集成到 CMake
通过 `EdgeContract.cmake` 自动调用，开发者只需关注 YAML 维护，构建系统会自动处理 Go 编译与 C 代码生成。

---
**EdgeOS SDK Toolchain - 以契约之名，定工业之律。**