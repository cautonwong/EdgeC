package main

import (
	"contract-compiler/core"
	"contract-compiler/exporter"
	"contract-compiler/model"
	"contract-compiler/protocols"
	"embed"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"text/template"
	"hash/crc32"
	"encoding/json"
	"errors"

	"gopkg.in/yaml.v3"
)

//go:embed templates
var tplFS embed.FS

type TplCtx struct {
	ActiveTags []model.TagDef
	AllTags    []model.TagDef
	ActiveMap  map[string]bool
	PMap       map[string][]protocols.ResolvedBinding
	Hashes     map[string]core.ProtocolHashInfo
	TagHash    core.ProtocolHashInfo
	Configs    []model.ConfigDef
	Params     []model.ParamDef
	Hardware   []model.HardwareDef
	Pipelines  []model.PipelineDef
	Policies   []model.PolicyDef
	UI         model.UIDef
	Product    string
	Signature  uint32
	Router     core.RouterInfo
}

func (c TplCtx) IsActive(name string) bool { return c.ActiveMap[name] }

func main() {
	schema := flag.String("schema", "", "path")
	product := flag.String("product", "", "path")
	local := flag.String("local", "", "path")
	out := flag.String("out", "", "path")
	base := flag.String("base", "", "path")
	target := flag.String("target", "master", "target")
	flag.Parse()

	contract := core.BuildContract(*schema, *product, *local, *base)
	actMap := make(map[string]bool)
	for _, t := range contract.ActiveTags { actMap[t.Name] = true }
	resolved := protocols.Resolve(contract.ActiveTags)

	// --- Phase 3: 生成路由掩码 ---
	routerInfo := core.GenerateRouterInfo(contract)

	hashes := make(map[string]core.ProtocolHashInfo)
	for proto, rbs := range resolved {
		entries := make([]core.HashEntry, 0, len(rbs))
		for _, rb := range rbs {
			keyStr := ""
			switch proto {
			case "MODBUS": keyStr = fmt.Sprintf("%d", rb.Addr)
			case "DLT645": keyStr = rb.DI
			case "DLT698": keyStr = rb.OAD
			case "IEC104": keyStr = fmt.Sprintf("%d", rb.IOA)
			}
			if keyStr == "" { continue }
			entries = append(entries, core.HashEntry{
				RawKey: core.ParseProtocolKey(proto, keyStr),
				StrKey: keyStr, TagID: rb.TagID, TagName: rb.TagName, Scale: rb.Scale, ByteLen: rb.ByteLen, Access: rb.Access,
			})
		}
		if len(entries) > 0 { hashes[proto] = core.GeneratePerfectHash(proto, entries) }
	}

	var sig uint32
	for _, t := range contract.ActiveTags { sig = crc32.Update(sig, crc32.IEEETable, []byte(fmt.Sprintf("%d", t.ID))) }

	tagEntries := make([]core.HashEntry, 0, len(contract.ActiveTags))
	for i, t := range contract.ActiveTags {
		tagEntries = append(tagEntries, core.HashEntry{
			RawKey:  uint64(t.ID),
			TagName: t.Name,
			TagID:   uint32(i),
		})
	}
	tagHash := core.GeneratePerfectHash("TAG", tagEntries)

	ctx := TplCtx{
		ActiveTags: contract.ActiveTags,
		AllTags:    contract.AllTags,
		ActiveMap:  actMap,
		PMap:       resolved,
		Hashes:     hashes,
		TagHash:    tagHash,
		Configs:    contract.Configs,
		Params:     contract.Params,
		Hardware:   contract.Hardware,
		Pipelines:  contract.Pipelines,
		Policies:   contract.Policies,
		UI:         contract.UI,
		Product:    contract.Product,
		Signature:  sig,
		Router:     routerInfo,
	}

	// [硬件资源冲突检测]
	hwUsage := make(map[int]string)
	for _, hw := range ctx.Hardware {
		if hw.ID == 0 { continue }
		if owner, exists := hwUsage[hw.ID]; exists {
			fmt.Fprintf(os.Stderr, "[ERROR] Hardware Conflict: ID %d used by both '%s' and '%s'\n", hw.ID, owner, hw.Label)
			os.Exit(1)
		}
		hwUsage[hw.ID] = hw.Label
	}

	os.MkdirAll(*out, 0755)
	
	fm := template.FuncMap{
		"lower": strings.ToLower,
		"equalFold": strings.EqualFold,
		"hex": func(v uint32) string { return fmt.Sprintf("0x%08X", v) },
		"marshal": func(v interface{}) string {
			b, _ := json.Marshal(v)
			return string(b)
		},
		"marshal_pretty": func(v interface{}) string {
			b, _ := json.MarshalIndent(v, "", "  ")
			return string(b)
		},
		"c_escape": func(s string) string {
			// 使用 raw string 避免转义地狱
			bs := `\`
			escapedBs := `\\`
			qs := `"`
			escapedQs := `\"`
			s = strings.ReplaceAll(s, bs, escapedBs)
			s = strings.ReplaceAll(s, qs, escapedQs)
			return s
		},
		"len": func(v interface{}) int {
			if v == nil { return 0 }
			switch val := v.(type) {
			case []uint8: return len(val)
			case []int: return len(val)
			case []interface{}: return len(val)
			case []model.TagDef: return len(val)
			case []model.ParamDef: return len(val)
			case []model.PipelineDef: return len(val)
			case []model.PolicyDef: return len(val)
			case []model.HardwareDef: return len(val)
			}
			return 0
		},
		"dict": func(values ...interface{}) (map[string]interface{}, error) {
			if len(values)%2 != 0 { return nil, errors.New("invalid dict call") }
			dict := make(map[string]interface{}, len(values)/2)
			for i := 0; i < len(values); i+=2 {
				key, ok := values[i].(string)
				if !ok { return nil, errors.New("dict keys must be strings") }
				dict[key] = values[i+1]
			}
			return dict, nil
		},
		"default": func(d, v interface{}) interface{} {
			if v == nil { return d }
			switch val := v.(type) {
			case string: if val == "" { return d }
			case int: if val == 0 { return d }
			}
			return v
		},
		"isTrue": func(v interface{}) bool {
			if v == nil { return false }
			switch val := v.(type) {
			case bool: return val
			case int: return val != 0
			case string: return val == "1" || strings.ToLower(val) == "true"
			}
			return false
		},
		"upper": strings.ToUpper,
	}

	if *target == "master" {
		render("h", filepath.Join(*out, "contracts.h"), "templates/master_h.tmpl", ctx, fm)
		render("c", filepath.Join(*out, "contracts.c"), "templates/master_c.tmpl", ctx, fm)
		render("cmake", filepath.Join(*out, "config.cmake"), "templates/config_cmake.tmpl", ctx, fm)
		
		// [新特性渲染]
		render("ts", filepath.Join(*out, "tags_type.ts"), "templates/tags_ts.tmpl", ctx, fm)
		render("fuzz", filepath.Join(*out, "test_fuzz_params.c"), "templates/test_fuzz_params.tmpl", ctx, fm)
		render("router", filepath.Join(*out, "edge_router_table.c"), "templates/edge_router_table.tmpl", ctx, fm)
		
		// [影子元数据/JSON Schema]
		shadowFile := filepath.Join(*out, "shadow_schema.json")
		shadowJSON, _ := json.MarshalIndent(contract, "", "  ")
		os.WriteFile(shadowFile, shadowJSON, 0644)
		fmt.Printf("[Compiler] Generated shadow metadata: %s\n", shadowFile)

		y, _ := yaml.Marshal(contract)
		os.WriteFile(filepath.Join(*out, "contracts.yaml"), y, 0644)

		docExp := &exporter.DocExporter{}
		docExp.Export(contract, resolved, *out)
	}
}

func render(name, outPath, tplPath string, ctx interface{}, fm template.FuncMap) {
	t := template.New(filepath.Base(tplPath)).Funcs(fm)
	
	// 加载 components
	// 使用 glob pattern 匹配 templates/components 下的所有 .tmpl 文件
	_, err := t.ParseFS(tplFS, "templates/components/*.tmpl")
	if err != nil && !strings.Contains(err.Error(), "no files") {
		panic(fmt.Errorf("ParseFS components: %v", err))
	}

	// 加载主入口
	t, err = t.ParseFS(tplFS, tplPath)
	if err != nil { panic(fmt.Errorf("ParseFS %s: %v", tplPath, err)) }

	f, _ := os.Create(outPath)
	defer f.Close()
	if err := t.Execute(f, ctx); err != nil { panic(fmt.Errorf("Execute %s: %v", tplPath, err)) }
}