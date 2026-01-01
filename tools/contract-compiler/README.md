# EdgeOS Contract Compiler (Golang Version v4.4+)

EdgeOS 契约编译器是整个分布式系统的数据枢纽。它负责将多层级的 YAML 契约定义（Tags & Bindings）编译为高度优化的 C 语言查表代码（Master 模式）或紧凑的 C 结构体（Slave 模式）。

## 1. 设计原理 (Architecture)

本编译器采用 Golang 重构，旨在解决原始 Python 版本在处理大规模工业点表时的弱类型截断和环境依赖问题。

*   **强类型模型 (Type Safety)**：利用 Go 的 `struct tags` 严格校验 YAML 语义，在编译阶段即可发现 ID 冲突或地址非法。
*   **纯 Map 驱动合并 (Pure-Map Merger)**：采用原始 `map[string]interface{}` 进行多级合并后再转结构体。这彻底解决了库文件“零值”覆盖现场“有效值”的顽疾。
*   **物理隔离模板 (De-hardcoded Templates)**：通过 `//go:embed` 将 `.tmpl` 文件嵌入二进制，实现逻辑与展现的分离，同时保持单文件分发的便利性。
*   **接口化协议插件 (Plugin Registry)**：每个工业协议都是一个独立的插件，通过 `init()` 自动注册，核心引擎无需修改即可支持新协议。

---

## 2. YAML 覆盖优先级 (Override Hierarchy)

编译器遵循 **“现场真相优先”** 的权力链条，确保标准定义能够被灵活地根据现场物理环境进行修正。

| 等级 | 文件类型 | 职责 | 优先级 |
| :--- | :--- | :--- | :--- |
| **L1** | `tags.yaml` | **全局字典**：定义测点语义、ID 和标准单位 | 最低 |
| **L2** | `includes/*.yaml` | **协议库**：定义设备厂商的标准寄存器映射 | 中 |
| **L3** | `product.yaml` | **产品模版**：定义产品的默认行为、属性 Override | 高 |
| **L4** | `extension.yaml` | **现场真相**：修正物理地址、缩放系数、本地测点扩展 | **最高** |

**合并逻辑说明**：
*   **后发制人**：优先级高的文件定义的属性将物理覆盖（而非追加）低优先级文件的同名属性。
*   **现场解释权**：如果 L4 定义了与 L2/L3 相同的协议绑定（如 MODBUS 寄存器），L4 的配置将被视为“最终真相”。

---

## 3. 安全红线校验 (Safety Guardrails)

编译器内置了工业级的自检机制，任何不符合逻辑的配置都会导致 **Build Panic**，拒绝产生带毒代码：

1.  **ID 唯一性校验**：严禁两个不同的测点名共用同一个逻辑 ID。
2.  **物理地址冲突检测**：严禁在同一协议内将两个测点映射到同一个物理寄存器地址（目前支持 Modbus）。
3.  **类型闭环检测**：确证生成的浮点数常量符合 C99 规范（防止产生 `0f` 这种语法错误）。

---

## 4. 扩展新协议 (Adding New Protocols)

编译器采用 **“接口注册制”**，扩展新协议（如 Zigbee 或私有 RF 协议）无需触动核心引擎。

### 步骤 1：实现接口
在 `protocols/` 目录下新建 `zigbee.go`，实现 `ProtocolHandler` 接口：

```go
type ZigbeeHandler struct{}

func (h *ZigbeeHandler) Name() string { return "ZIGBEE" }
func (h *ZigbeeHandler) Resolve(t model.TagDef, b model.Binding) ResolvedBinding {
    return ResolvedBinding{
        TagName: t.Name,
        ProtocolType: "ZIGBEE",
        Addr: getUint16Case(b.Attributes, "endpoint"), // 自定义提取
        // ... 其他逻辑
    }
}
```

### 步骤 2：自动注册
在文件末尾添加 `init` 函数：

```go
func init() {
    Register(&ZigbeeHandler{})
}
```

### 步骤 3：更新模板 (可选)
如果新协议需要特殊的 C 结构体或查表函数，修改 `templates/master_c.tmpl` 即可。

---

## 5. 构建与使用

### 编译编译器
```bash
go build -o contract-compiler .
```

### 生产 C 代码
```bash
./contract-compiler \
    --schema ../../contracts/schema/tags.yaml \
    --product ../../contracts/products/iems.yaml \
    --local ./extension.yaml \
    --out ./generated
```

---
**EdgeOS SDK Toolchain - 定力驱动工业未来**

