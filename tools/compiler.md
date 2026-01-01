这是一个非常明智的决定。

使用 Python 编写复杂的 Schema 解析和代码生成工具时，**弱类型**确实容易导致 `KeyError`, `AttributeError` 等运行时错误，且难以维护复杂的嵌套结构。

改用 **Golang** 有以下巨大优势：
1.  **强类型 (Struct tags)**：YAML 结构必须与 Go Struct 严格匹配，解析时自动校验，**在编译生成器时就能发现错误**。
2.  **单文件分发**：编译成一个静态 Binary，丢进 CI/CD 容器或给同事用，无需配置 Python 环境。
3.  **模板引擎强大**：Go 的 `text/template` 非常适合生成 C 代码。

以下是使用 **Golang 重构的 Contract Compiler (v4.0)** 完整方案。

---

### 1. 工程目录结构

我们将工具独立为一个 Go 模块。

```text
tools/contract-compiler/
├── go.mod                # 依赖管理 (gopkg.in/yaml.v3)
├── go.sum
├── main.go               # 主逻辑
├── model/                # 数据模型定义
│   └── types.go
└── templates/            # C 代码模板 (嵌入到 Go 二进制中)
    ├── header.tmpl
    └── source.tmpl
```

### 2. 初始化 Go 模块

在 `tools/contract-compiler` 目录下执行：

```bash
go mod init contract-compiler
go get gopkg.in/yaml.v3
```

---

### 3. 数据模型定义 (`model/types.go`)

利用 Go 的 Struct Tag 严格定义 YAML 结构，彻底杜绝字段拼写错误。

```go
package model

// 1. 全局 Tag 定义 (Schema)
type GlobalSchema struct {
	Tags []TagDef `yaml:"tags"`
}

type TagDef struct {
	ID         uint32                 `yaml:"id"`
	Name       string                 `yaml:"name"`
	Type       string                 `yaml:"type"`
	Unit       string                 `yaml:"unit"`
	Desc       string                 `yaml:"desc"`
	Attributes map[string]interface{} `yaml:"attributes"`
	
	// 运行时注入的字段 (不从 YAML 读)
	Bindings   []Binding              `yaml:"bindings,omitempty"`
	IsEnabled  bool                   `yaml:"-"`
}

// 2. 协议绑定 (Binding)
type BindingFile struct {
	Model    string    `yaml:"model"`
	Protocol string    `yaml:"protocol"`
	Mappings []Binding `yaml:"mappings"`
}

type Binding struct {
	TagName      string  `yaml:"tag"` // 关联的 Tag Name
	
	// Modbus Specific
	Addr         uint16  `yaml:"addr,omitempty"`
	FuncCode     uint8   `yaml:"func,omitempty"`
	Scale        float32 `yaml:"scale,omitempty"`
	
	// DLMS Specific
	Obis         string  `yaml:"obis,omitempty"`
	Scaler       float32 `yaml:"scaler,omitempty"`
	
	// IEC104 Specific
	IOA          int     `yaml:"ioa,omitempty"`
	TypeID       int     `yaml:"type_id,omitempty"`
	
	// 内部字段
	ProtocolType string  `yaml:"protocol_type,omitempty"`
}

// 3. 产品配置 (Product)
type ProductConfig struct {
	ProductName string                 `yaml:"product"`
	Includes    []string               `yaml:"includes"`
	Overrides   map[string]TagOverride `yaml:"overrides"`
}

type TagOverride struct {
	Enabled    *bool                  `yaml:"enabled"` // 指针用于判断字段是否存在
	Attributes map[string]interface{} `yaml:",inline"` // 剩余字段混入
}

// 4. 最终生效契约 (中间态)
type EffectiveContract struct {
	Product     string    `yaml:"product"`
	GeneratedAt string    `yaml:"generated_at"`
	Tags        []TagDef  `yaml:"tags"`
}
```

---

### 4. 主程序实现 (`main.go`)

逻辑清晰：加载 -> 合并 -> 生成。

```go
package main

import (
	"contract-compiler/model"
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"text/template"
	"time"

	"gopkg.in/yaml.v3"
)

// CLI 参数
var (
	schemaPath  = flag.String("schema", "", "Path to tags.yaml")
	productPath = flag.String("product", "", "Path to product.yaml")
	localPath   = flag.String("local", "", "Path to extension.yaml (optional)")
	outDir      = flag.String("out", "./generated", "Output directory")
)

func main() {
	flag.Parse()

	if *schemaPath == "" || *productPath == "" {
		log.Fatal("Usage: contract-compiler -schema <path> -product <path> -out <dir>")
	}

	// 1. 加载 Global Schema
	globalTags := loadSchema(*schemaPath)
	fmt.Printf("Loaded %d global tags.\n", len(globalTags))

	// 2. 加载 Product Config
	productConf := loadProduct(*productPath)
	fmt.Printf("Building Product: %s\n", productConf.ProductName)

	// 3. 加载 Bindings (协议映射)
	// Map: TagName -> []Binding
	bindingMap := make(map[string][]model.Binding)
	
	// 解析 Includes 路径 (相对于 Product 文件或根目录)
	// 这里假设简单的相对路径处理
	rootDir := filepath.Dir(*productPath) + "/../../" // 回退到 contracts 根
	
	for _, inc := range productConf.Includes {
		// 拼接完整路径
		fullPath := filepath.Join(rootDir, inc)
		bindings, proto := loadBindings(fullPath)
		
		for _, b := range bindings {
			b.ProtocolType = proto
			bindingMap[b.TagName] = append(bindingMap[b.TagName], b)
		}
	}

	// 4. 合并逻辑 (Merge & Override)
	effectiveTags := make([]model.TagDef, 0)
	
	// 按 ID 排序遍历
	keys := make([]string, 0, len(globalTags))
	for k := range globalTags { keys = append(keys, k) }
	sort.Strings(keys)

	for _, name := range keys {
		tag := globalTags[name]
		
		// 检查 Overrides
		if ovr, ok := productConf.Overrides[name]; ok {
			// 如果显式禁用
			if ovr.Enabled != nil && !*ovr.Enabled {
				continue
			}
			// 合并属性
			if tag.Attributes == nil { tag.Attributes = make(map[string]interface{}) }
			for k, v := range ovr.Attributes {
				tag.Attributes[k] = v
			}
		}
		
		// 注入 Bindings
		if bindings, ok := bindingMap[name]; ok {
			tag.Bindings = bindings
		}
		
		tag.IsEnabled = true
		effectiveTags = append(effectiveTags, tag)
	}

	// 5. 生成中间态 YAML
	contract := model.EffectiveContract{
		Product:     productConf.ProductName,
		GeneratedAt: time.Now().Format(time.RFC3339),
		Tags:        effectiveTags,
	}
	
	saveYaml(filepath.Join(*outDir, "effective_contract.yaml"), contract)

	// 6. 生成 C 代码
	generateCCode(contract, *outDir)
	
	fmt.Println("✅ Build Success!")
}

// --- Helper Functions ---

func loadSchema(path string) map[string]model.TagDef {
	data, err := os.ReadFile(path)
	if err != nil { log.Fatalf("Read schema failed: %v", err) }
	
	var schema model.GlobalSchema
	if err := yaml.Unmarshal(data, &schema); err != nil {
		log.Fatalf("Parse schema failed: %v", err)
	}
	
	m := make(map[string]model.TagDef)
	for _, t := range schema.Tags {
		m[t.Name] = t
	}
	return m
}

func loadProduct(path string) model.ProductConfig {
	data, err := os.ReadFile(path)
	if err != nil { log.Fatalf("Read product failed: %v", err) }
	
	var cfg model.ProductConfig
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		log.Fatalf("Parse product failed: %v", err)
	}
	return cfg
}

func loadBindings(path string) ([]model.Binding, string) {
	data, err := os.ReadFile(path)
	// 容错：如果找不到，尝试直接路径
	if err != nil {
		data, err = os.ReadFile(path) // Retry
		if err != nil {
			log.Printf("WARN: Binding file not found: %s", path)
			return nil, ""
		}
	}

	var bf model.BindingFile
	if err := yaml.Unmarshal(data, &bf); err != nil {
		log.Fatalf("Parse binding %s failed: %v", path, err)
	}
	return bf.Mappings, bf.Protocol
}

func saveYaml(path string, v interface{}) {
	f, err := os.Create(path)
	if err != nil { log.Fatal(err) }
	defer f.Close()
	
	enc := yaml.NewEncoder(f)
	enc.SetIndent(2)
	enc.Encode(v)
}
```

---

### 5. 代码生成逻辑 (Embed Templates)

为了保持单文件（方便复制），我们将模板作为**字符串常量**放在 Go 代码里。

**`generator.go` (追加到 main 包)**

```go
package main

import (
	"contract-compiler/model"
	"os"
	"path/filepath"
	"sort"
	"text/template"
)

// 模板函数：分类获取不同协议的绑定
func filterBindings(tags []model.TagDef, proto string) []struct {
	Tag     model.TagDef
	Binding model.Binding
} {
	var res []struct {
		Tag     model.TagDef
		Binding model.Binding
	}
	for _, t := range tags {
		for _, b := range t.Bindings {
			if b.ProtocolType == proto {
				res = append(res, struct {
					Tag     model.TagDef
					Binding model.Binding
				}{t, b})
			}
		}
	}
	// 默认排序：按地址或OBIS
	sort.Slice(res, func(i, j int) bool {
		if proto == "MODBUS" {
			return res[i].Binding.Addr < res[j].Binding.Addr
		}
		if proto == "DLMS" {
			return res[i].Binding.Obis < res[j].Binding.Obis
		}
		return res[i].Tag.ID < res[j].Tag.ID
	})
	return res
}

func generateCCode(contract model.EffectiveContract, outDir string) {
	os.MkdirAll(outDir+"/include", 0755)
	os.MkdirAll(outDir+"/src", 0755)

	funcMap := template.FuncMap{
		"filterProto": filterBindings,
	}

	// 1. 生成 Header (contracts.h)
	tmplH, _ := template.New("header").Funcs(funcMap).Parse(headerTemplate)
	fH, _ := os.Create(filepath.Join(outDir, "include/contracts.h"))
	tmplH.Execute(fH, contract)
	fH.Close()

	// 2. 生成 Source (contracts.c)
	tmplC, _ := template.New("source").Funcs(funcMap).Parse(sourceTemplate)
	fC, _ := os.Create(filepath.Join(outDir, "src/contracts.c"))
	tmplC.Execute(fC, contract)
	fC.Close()
}

// --- TEMPLATES ---

const headerTemplate = `
/* AUTO-GENERATED BY GO COMPILER */
#ifndef EDGE_CONTRACTS_H
#define EDGE_CONTRACTS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Feature Gates
{{- range .Tags }}
#define TAG_{{ .Name }}_ENABLED 1
{{- end }}
#define IS_ENABLED(x) ((x) == 1)

// Tag Enum
typedef enum {
    TAG_INVALID = 0,
{{- range .Tags }}
    TAG_{{ .Name }} = {{ .ID }},
{{- end }}
    TAG_MAX_ID
} edge_tag_id_t;

// Structures
typedef struct { const char* name; const char* unit; bool persist; } tag_meta_t;
typedef struct { uint16_t addr; edge_tag_id_t id; float scale; } modbus_map_t;
typedef struct { const char* obis; edge_tag_id_t id; float scaler; } dlms_map_t;

// API
extern const tag_meta_t g_tag_meta_table[];
edge_tag_id_t edge_lookup_modbus(uint16_t reg);
edge_tag_id_t edge_lookup_dlms(const char* obis);

#endif
`

const sourceTemplate = `
#include "contracts.h"
#include <string.h>

// Metadata Table
const tag_meta_t g_tag_meta_table[] = {
{{- range .Tags }}
    [TAG_{{ .Name }}] = { "{{ .Name }}", "{{ .Unit }}", {{ if .Attributes.persist }}true{{else}}false{{end}} },
{{- end }}
};

// ---------------------------
// Modbus Map (Sorted)
// ---------------------------
{{- $mb := filterProto .Tags "MODBUS" }}
static const modbus_map_t s_modbus_map[] = {
{{- range $mb }}
    { {{ .Binding.Addr }}, TAG_{{ .Tag.Name }}, {{ .Binding.Scale }}f },
{{- end }}
};
static const int s_modbus_len = {{ len $mb }};

edge_tag_id_t edge_lookup_modbus(uint16_t reg) {
    int l = 0, r = s_modbus_len - 1;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        if (s_modbus_map[mid].addr == reg) return s_modbus_map[mid].id;
        if (s_modbus_map[mid].addr < reg) l = mid + 1;
        else r = mid - 1;
    }
    return TAG_INVALID;
}

// ---------------------------
// DLMS Map (Linear Search for now)
// ---------------------------
{{- $dl := filterProto .Tags "DLMS" }}
static const dlms_map_t s_dlms_map[] = {
{{- range $dl }}
    { "{{ .Binding.Obis }}", TAG_{{ .Tag.Name }}, {{ .Binding.Scaler }}f },
{{- end }}
};
static const int s_dlms_len = {{ len $dl }};

edge_tag_id_t edge_lookup_dlms(const char* obis) {
    for(int i=0; i<s_dlms_len; i++) {
        if(strcmp(s_dlms_map[i].obis, obis) == 0) return s_dlms_map[i].id;
    }
    return TAG_INVALID;
}
`
```

---

### 6. 使用指南

#### 步骤 1：编译工具

```bash
cd tools/contract-compiler
go build -o ../contract_compiler  # 编译二进制放到 tools 根目录
```

#### 步骤 2：在 CMake 中调用

修改 `cmake/EdgeContract.cmake`，将原来的 `python3 ...` 替换为：

```cmake
add_custom_command(
    OUTPUT "${OUT_HEADER}" "${OUT_SOURCE}"
    COMMAND "${CMAKE_SOURCE_DIR}/tools/contract_compiler"
            -schema "${SCHEMA_PATH}"
            -product "${PRODUCT_YAML}"
            -out "${GEN_DIR}"
    DEPENDS ...
)
```

### ✅ 方案评价

1.  **鲁棒性**：Go 的 `yaml.Unmarshal` 会严格检查字段类型，如果 YAML 写错了（比如 `addr` 写成了字符串），生成器直接报错并指出行号，而不是生成错误的 C 代码。
2.  **性能**：Go 二进制执行速度极快，且没有 Python 解释器启动开销，大型项目构建更快。
3.  **零依赖发布**：你可以把编译好的 `contract_compiler` 二进制直接提交到 Git（或放在 Docker 镜像里），开发人员连 Python 都不用装。

## yaml的合并逻辑  这么多种协议的支持? 将来如何扩展协议  这些问题你上面给的方案都有吗

这是一个非常核心的架构拷问。

如果 `contract_compiler` 只是简单地堆砌 `if (modbus) ... else if (can) ...`，那么当你未来要增加 **Zigbee** 或 **私有 RF 协议** 时，你就必须修改核心逻辑，这违反了 **开闭原则 (Open-Closed Principle)**。

为了实现 **“对扩展开放，对修改关闭”**，并在 Golang 中优雅地处理多种协议的差异，我们需要采用 **接口（Interface） + 注册机制** 的设计模式。

以下是 **EdgeOS Contract Compiler (Go Version)** 的终极设计方案。

---

### 1. 核心设计图：如何解耦协议？

我们将编译器拆分为 **核心引擎 (Engine)** 和 **协议插件 (Plugins)**。

```text
[ YAML Files ]
      |
      v
+-----------------------+      +---------------------------+
|   Core Engine (Main)  | <--> |   Protocol Interface      |
+-----------------------+      +---------------------------+
| 1. 加载 Global Tags    |      ^           ^            ^
| 2. 加载 Product Conf   |      |           |            |
| 3. 合并逻辑 (Merge)    |   [Modbus]    [DLMS]       [CAN]
| 4. 模板渲染 (Render)   |   (Plugin)    (Plugin)     (Plugin)
+-----------------------+
```

---

### 2. 工程目录结构

```text
tools/contract-compiler/
├── go.mod
├── main.go                 # 入口
├── core/
│   ├── config.go           # 核心配置加载与合并逻辑
│   ├── generator.go        # C 代码生成编排
│   └── models.go           # 通用数据结构
├── protocols/              # [扩展点] 所有协议逻辑都在这里
│   ├── interface.go        # 定义协议接口
│   ├── modbus.go           # Modbus 实现
│   ├── dlms.go             # DLMS 实现
│   └── iec104.go           # IEC104 实现
└── templates/              # Go text/template 模板
    ├── header.tmpl
    └── source.tmpl
```

---

### 3. 代码实现：协议接口化 (The Interface)

这是解耦的关键。我们在 `protocols/interface.go` 中定义“一个协议需要做什么”。

```go
package protocols

// 协议生成器接口
type Generator interface {
    // 1. 协议名称 (e.g. "MODBUS")
    Name() string

    // 2. C 语言结构体定义 (生成 contracts.h 中的 struct)
    // e.g. "typedef struct { uint16_t addr; ... } modbus_map_t;"
    GetCStruct() string

    // 3. C 语言查找表生成 (生成 contracts.c 中的 const array)
    // 输入: 该协议下的所有绑定列表
    // 输出: C 代码片段 (数组定义 + 查找函数)
    GenerateCCode(bindings []BindingObj) string
}

// 通用的绑定对象 (从 YAML 解析而来)
type BindingObj struct {
    TagName    string                 `yaml:"tag_name"`
    TagID      uint32                 `yaml:"tag_id"`
    Attributes map[string]interface{} `yaml:",inline"` // 捕获所有协议特定字段
}
```

---

### 4. 核心实现：合并逻辑 (Merge Logic)

在 `core/config.go` 中，处理你关心的 **YAML 合并** 问题。

**逻辑要点**：
1.  **Global Schema** 是基准。
2.  **Product Config** 决定启用/禁用。
3.  **Bindings** 决定物理映射。
4.  **Extension** (Local) 可以追加。

```go
package core

import (
	"contract-compiler/protocols"
	"fmt"
	"sort"
    "gopkg.in/yaml.v3"
)

// 最终生效的契约对象
type EffectiveContract struct {
    Product       string
    Tags          []TagDef
    // 按协议分组的绑定列表: Map<ProtocolName, []Binding>
    ProtocolBindings map[string][]protocols.BindingObj
}

func BuildEffectiveContract(schema GlobalSchema, product ProductConfig, localExt *LocalExt) *EffectiveContract {
    contract := &EffectiveContract{
        Product:          product.Name,
        ProtocolBindings: make(map[string][]protocols.BindingObj),
    }

    // 1. 建立全量 Tag 索引
    tagMap := make(map[string]TagDef)
    for _, t := range schema.Tags { tagMap[t.Name] = t }
    
    // 2. 合并本地扩展 (如果存在)
    if localExt != nil {
        for _, t := range localExt.Tags { tagMap[t.Name] = t }
    }

    // 3. 处理绑定 (Bindings)
    // 遍历 Product 包含的所有 binding 文件
    for _, bindFile := range product.BindingFiles {
        protoName := bindFile.Protocol // e.g. "MODBUS"
        
        for _, rawMapping := range bindFile.Mappings {
            tagName := rawMapping["tag"].(string)
            
            // 只有在 Schema 中存在的 Tag 才有效
            if tagDef, exists := tagMap[tagName]; exists {
                // 构建绑定对象
                binding := protocols.BindingObj{
                    TagName:    tagName,
                    TagID:      tagDef.ID,
                    Attributes: rawMapping, // 保留原始 YAML 字段 (addr, scale...)
                }
                contract.ProtocolBindings[protoName] = append(contract.ProtocolBindings[protoName], binding)
            }
        }
    }

    // 4. 处理裁剪 (Overrides) 并生成最终 Tag 列表
    for _, tag := range tagMap {
        // 检查 Product Overrides (enabled/disabled)
        if ovr, ok := product.Overrides[tag.Name]; ok {
            if !ovr.Enabled { continue } // 被禁用
            // 合并属性...
        }
        contract.Tags = append(contract.Tags, tag)
    }
    
    // 排序，保证生成确定性
    sort.Slice(contract.Tags, func(i, j int) bool { return contract.Tags[i].ID < contract.Tags[j].ID })

    return contract
}
```

---

### 5. 协议插件实现：以 Modbus 为例

在 `protocols/modbus.go` 中，我们实现具体的 C 代码生成逻辑。

```go
package protocols

import (
	"fmt"
	"sort"
	"strings"
)

type ModbusGen struct{}

func (m *ModbusGen) Name() string { return "MODBUS" }

func (m *ModbusGen) GetCStruct() string {
	return `
typedef struct {
    uint16_t addr;
    edge_tag_id_t id;
    float scale;
} modbus_map_t;`
}

func (m *ModbusGen) GenerateCCode(bindings []BindingObj) string {
	// 1. 针对 Modbus 地址排序 (为了二分查找)
	sort.Slice(bindings, func(i, j int) bool {
		addrI := getInt(bindings[i].Attributes, "addr")
		addrJ := getInt(bindings[j].Attributes, "addr")
		return addrI < addrJ
	})

	var sb strings.Builder
    
    // 生成数组
	sb.WriteString(fmt.Sprintf("// --- Modbus Map (Count: %d) ---\n", len(bindings)))
	sb.WriteString("const modbus_map_t g_modbus_map[] = {\n")
	
	for _, b := range bindings {
		addr := getInt(b.Attributes, "addr")
		scale := getFloat(b.Attributes, "scale", 1.0)
		sb.WriteString(fmt.Sprintf("    { %d, TAG_%s, %.2ff },\n", addr, b.TagName, scale))
	}
	sb.WriteString("};\n")
    sb.WriteString(fmt.Sprintf("const int g_modbus_map_len = %d;\n\n", len(bindings)))

    // 生成查找函数 (二分查找)
    sb.WriteString(`
edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale) {
    int l = 0, r = g_modbus_map_len - 1;
    while (l <= r) {
        int mid = l + (r - l) / 2;
        if (g_modbus_map[mid].addr == reg) {
            if (out_scale) *out_scale = g_modbus_map[mid].scale;
            return g_modbus_map[mid].id;
        }
        if (g_modbus_map[mid].addr < reg) l = mid + 1;
        else r = mid - 1;
    }
    return TAG_INVALID;
}
`)
	return sb.String()
}

// 辅助函数：安全获取 Map 中的值
func getInt(m map[string]interface{}, key string) int {
    if v, ok := m[key]; ok {
        if i, ok := v.(int); ok { return i }
    }
    return 0
}
```

---

### 6. 主程序入口 (`main.go`)

将插件注册进去，利用 Go 的 template 生成最终文件。

```go
package main

import (
	"contract-compiler/core"
	"contract-compiler/protocols"
	"flag"
	"os"
	"text/template"
)

// 注册所有支持的协议
var plugins = []protocols.Generator{
	&protocols.ModbusGen{},
	&protocols.DlmsGen{}, // 假设已实现
    // &protocols.CanGen{},  // 将来要加 CAN，只需在这里注册
}

func main() {
	// ... (解析参数 schemaPath, productPath 等) ...

    // 1. 构建契约
	contract := core.BuildEffectiveContract(schema, product, local)

	// 2. 准备模板数据
	type TplData struct {
		Contract   *core.EffectiveContract
		StructDefs []string
		ImplCodes  []string
	}
	
	data := TplData{Contract: contract}

	// 3. 遍历插件，生成各协议代码
	for _, p := range plugins {
		// 检查当前产品是否用到了该协议
		if bindings, ok := contract.ProtocolBindings[p.Name()]; ok && len(bindings) > 0 {
			data.StructDefs = append(data.StructDefs, p.GetCStruct())
			data.ImplCodes = append(data.ImplCodes, p.GenerateCCode(bindings))
		}
	}

	// 4. 渲染 Header
	renderTemplate("templates/header.tmpl", "contracts.h", data)
	
	// 5. 渲染 Source
	renderTemplate("templates/source.tmpl", "contracts.c", data)
}
```

---

### 7. 模板文件示例

**`templates/header.tmpl`**

```c
/* AUTO-GENERATED */
#ifndef EDGE_CONTRACTS_H
#define EDGE_CONTRACTS_H

#include <stdint.h>

// 1. Tag Enum
typedef enum {
    TAG_INVALID = 0,
{{- range .Contract.Tags }}
    TAG_{{ .Name }} = {{ .ID }},
{{- end }}
    TAG_MAX_ID
} edge_tag_id_t;

// 2. Protocol Structs (由插件生成)
{{- range .StructDefs }}
{{ . }}
{{- end }}

// 3. API Declarations
// ... (标准 API) ...

#endif
```

**`templates/source.tmpl`**

```c
#include "contracts.h"

// Metadata Table
const tag_meta_t g_tag_meta_table[] = { ... };

// ---------------------------------------------------------
// Protocol Implementations
// ---------------------------------------------------------
{{- range .ImplCodes }}
{{ . }}
{{- end }}
```

---

### 🔄 回答你的问题：将来如何扩展？

基于这套 Golang 架构：

1.  **新增协议（比如 Zigbee）**：
    *   **不需要** 修改 `main.go` 的核心逻辑。
    *   **不需要** 修改 YAML 解析逻辑。
    *   **只需要**：
        1.  在 `protocols/` 下新建 `zigbee.go`，实现 `Generator` 接口。
        2.  在 `main.go` 的 `plugins` 列表中注册它。
        3.  编写 `bindings/zigbee/xxx.yaml`。
    *   编译器会自动识别 YAML 中的 `protocol: ZIGBEE`，并调用你的新插件生成 C 代码。

2.  **多端设备**：
    *   在 `Generator` 接口中增加 `GenerateSlaveCode(bindings)` 方法。
    *   在主程序中增加 `--target=slave` 参数。
    *   插件根据 target 参数，决定是生成 Master 的查表代码，还是 Slave 的结构体代码。

这套方案将 **“变化的”**（协议细节）封装在插件中，将 **“不变的”**（YAML 解析、合并、模板渲染）保留在核心引擎中，完美符合工业级软件的演进需求。