package main

import (
	"contract-compiler/core"
	"contract-compiler/exporter"
	"contract-compiler/model"
	"contract-compiler/protocols"
	_ "embed"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"text/template"
	"hash/crc32"
	"encoding/json"

	"gopkg.in/yaml.v3"
)

//go:embed templates/master_h.tmpl
var masterHTpl string

//go:embed templates/master_c.tmpl
var masterCTpl string

//go:embed templates/config_cmake.tmpl
var configCmakeTpl string

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

	// 5. 计算 TagID 到连续索引的完美哈希 (解决稀疏数组空间占用问题)
	tagEntries := make([]core.HashEntry, 0, len(contract.ActiveTags))
	for i, t := range contract.ActiveTags {
		tagEntries = append(tagEntries, core.HashEntry{
			RawKey:  uint64(t.ID),
			TagName: t.Name,
			TagID:   uint32(i), // 存储该 Tag 在 Dense Table 中的连续索引
		})
	}
	tagHash := core.GeneratePerfectHash("TAG", tagEntries)

	ctx := TplCtx{
		ActiveTags: contract.ActiveTags,
		AllTags:    contract.AllTags,
		ActiveMap:  actMap,
		PMap:       resolved,
		Hashes:     hashes,
		TagHash:    tagHash, // [新增] TagID 完美哈希元数据
		Configs:    contract.Configs,
				Params:      contract.Params,
				Hardware:    contract.Hardware,
				Pipelines:   contract.Pipelines,
				Policies:    contract.Policies,
				UI:          contract.UI,
				Product:     contract.Product,
				Signature:   sig,
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
							"c_escape": func(s string) string {
								s = strings.ReplaceAll(s, "\\", "\\\\")
								s = strings.ReplaceAll(s, "\"", "\\\"")
								return s
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
	}

	if *target == "master" {
		render("h", filepath.Join(*out, "contracts.h"), masterHTpl, ctx, fm)
		render("c", filepath.Join(*out, "contracts.c"), masterCTpl, ctx, fm)
		render("cmake", filepath.Join(*out, "config.cmake"), configCmakeTpl, ctx, fm)
		y, _ := yaml.Marshal(contract)
		os.WriteFile(filepath.Join(*out, "contracts.yaml"), y, 0644)

		docExp := &exporter.DocExporter{}
		docExp.Export(contract, resolved, *out)
	}
}

func render(name, path, tpl string, ctx interface{}, fm template.FuncMap) {
	t, err := template.New(name).Funcs(fm).Parse(tpl)
	if err != nil { panic(err) }
	f, _ := os.Create(path)
	defer f.Close()
	if err := t.Execute(f, ctx); err != nil { panic(err) }
}
