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
	"sort"

	"gopkg.in/yaml.v3"
)

//go:embed templates
var tplFS embed.FS

type TplCtx struct {
	ActiveTags   []model.TagDef
	AllTags      []model.TagDef
	ActiveMap    map[string]bool
	PMap         map[string][]protocols.ResolvedBinding
	TagPMap      map[string][]protocols.ResolvedBinding
	Hashes       map[string]core.ProtocolHashInfo
	TagHash      core.ProtocolHashInfo
	Configs      []model.ConfigDef
	Params       []model.ParamDef
	Connectivity []model.ConnectivityDef
	Devices      []model.DeviceAttachment
	Hardware     []model.HardwareDef
	Pipelines    []model.PipelineDef
	Policies     []model.PolicyDef
	UI           model.UIDef
	Product      string
	Signature    uint32
	Router       core.RouterInfo
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
	
	// 1. 协议解析
	resolved := protocols.Resolve(contract.ActiveTags)

	// 2. 强制排序 ActiveTags 确保索引稳定
	sort.Slice(contract.ActiveTags, func(i, j int) bool {
		return contract.ActiveTags[i].ID < contract.ActiveTags[j].ID
	})

	// 3. 建立 TagName -> (GlobalID, DenseIndex) 映射
	type tagInfo struct {
		GlobalID uint32
		DenseIdx uint32
	}
	tagInfoMap := make(map[string]tagInfo)
	for i, t := range contract.ActiveTags {
		tagInfoMap[t.Name] = tagInfo{GlobalID: t.ID, DenseIdx: uint32(i)}
	}

	// 4. 建立 TagPMap (按点位索引) 供模板反查
	tagPMap := make(map[string][]protocols.ResolvedBinding)
	for _, rbs := range resolved {
		for _, rb := range rbs {
			info := tagInfoMap[rb.TagName]
			rb.LogicalIndex = info.DenseIdx
			tagPMap[rb.TagName] = append(tagPMap[rb.TagName], rb)
		}
	}

	// 5. 生成协议哈希表
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
			case "DLMS": keyStr = rb.OBIS
			}
			if keyStr == "" { continue }
			
			info := tagInfoMap[rb.TagName]
			entries = append(entries, core.HashEntry{
				RawKey:       core.ParseProtocolKey(proto, keyStr),
				StrKey:       keyStr,
				TagID:        info.GlobalID,
				LogicalIndex: info.DenseIdx,
				TagName:      rb.TagName,
				Scale:        rb.Scale,
				ByteLen:      rb.ByteLen,
				Access:       rb.Access,
				IecType:      rb.IecType, Flags: rb.Flags, DeadBand: rb.DeadBand, GroupID: rb.GroupID,
				OBIS: core.ParseOBIS(rb.OBIS), ClassID: rb.ClassID, AttrID: rb.AttrID,
			})
		}
		if len(entries) > 0 { hashes[proto] = core.GeneratePerfectHash(proto, entries) }
	}

	// 6. 生成全局 TAG 映射
	tagEntries := make([]core.HashEntry, 0, len(contract.ActiveTags))
	for i, t := range contract.ActiveTags {
		tagEntries = append(tagEntries, core.HashEntry{
			RawKey:       uint64(t.ID),
			TagName:      t.Name,
			TagID:        t.ID,
			LogicalIndex: uint32(i),
		})
	}
	tagHash := core.GeneratePerfectHash("TAG", tagEntries)

	var sig uint32
	for _, t := range contract.ActiveTags { sig = crc32.Update(sig, crc32.IEEETable, []byte(fmt.Sprintf("%d", t.ID))) }

	routerInfo := core.GenerateRouterInfo(contract)

	ctx := TplCtx{
		ActiveTags:   contract.ActiveTags,
		AllTags:      contract.AllTags,
		ActiveMap:    actMap,
		PMap:         resolved,
		TagPMap:      tagPMap,
		Hashes:       hashes,
		TagHash:      tagHash,
		Configs:      contract.Configs,
		Params:       contract.Params,
		Connectivity: contract.Connectivity,
		Devices:      contract.Devices,
		Hardware:     contract.Hardware,
		Pipelines:    contract.Pipelines,
		Policies:     contract.Policies,
		UI:           contract.UI,
		Product:      contract.Product,
		Signature:    sig,
		Router:       routerInfo,
	}

	os.MkdirAll(*out, 0755)
	fm := template.FuncMap{
		"lower": strings.ToLower, "upper": strings.ToUpper, "hex": func(v uint32) string { return fmt.Sprintf("0x%08X", v) },
		"is_digit": func(s string) bool {
			return strings.TrimSpace(s) != "" && !strings.ContainsFunc(s, func(r rune) bool { return r < '0' || r > '9' })
		},
		"dict": func(values ...interface{}) (map[string]interface{}, error) {
			if len(values)%2 != 0 { return nil, errors.New("invalid dict call") }
			dict := make(map[string]interface{}, len(values)/2)
			for i := 0; i < len(values); i+=2 {
				key, ok := values[i].(string); if !ok { return nil, errors.New("keys must be strings") }
				dict[key] = values[i+1]
			}
			return dict, nil
		},
		"default": func(d, v interface{}) interface{} {
			if v == nil { return d }
			if s, ok := v.(string); ok && s == "" { return d }
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
		"marshal": func(v interface{}) string {
			if v == nil { return "{}" }
			b, err := json.Marshal(v)
			if err != nil || len(b) == 0 { return "{}" }
			// 避免产生空白或非法 JSON
			s := string(b)
			if s == "" || s == "null" { return "{}" }
			return s
		},
		"marshal_pretty": func(v interface{}) string { b, _ := json.MarshalIndent(v, "", "  "); return string(b) },
		"c_escape": func(s string) string {
			s = strings.ReplaceAll(s, `\\`, `\\\\`)
			s = strings.ReplaceAll(s, `"`, `\\"`)
			return s
		},
		"uint32": func(v interface{}) uint32 {
			switch val := v.(type) {
			case int: return uint32(val)
			case int32: return uint32(val)
			case uint32: return val
			}
			return 0
		},
		"sub": func(a, b interface{}) int {
			av := 0; switch v := a.(type) { case int: av = v; case int32: av = int(v); case uint32: av = int(v) }
			bv := 0; switch v := b.(type) { case int: bv = v; case int32: bv = int(v); case uint32: bv = int(v) }
			return av - bv
		},
		"_contract_hash_mix": func(key, seed, mask uint32) uint32 {
			h := key ^ seed
			h ^= h >> 16
			h *= 0x85ebca6b
			h ^= h >> 13
			h *= 0xc2b2ae35
			h ^= h >> 16
			return h & mask
		},
	}

	if *target == "master" {
		render("h", filepath.Join(*out, "contracts.h"), "templates/master_h.tmpl", ctx, fm)
		render("c", filepath.Join(*out, "contracts.c"), "templates/master_c.tmpl", ctx, fm)
		render("cmake", filepath.Join(*out, "config.cmake"), "templates/config_cmake.tmpl", ctx, fm)
		render("ts", filepath.Join(*out, "tags_type.ts"), "templates/tags_ts.tmpl", ctx, fm)
		render("fuzz", filepath.Join(*out, "test_fuzz_params.c"), "templates/test_fuzz_params.tmpl", ctx, fm)
		render("router", filepath.Join(*out, "edge_router_table.c"), "templates/edge_router_table.tmpl", ctx, fm)
		render("manifest", filepath.Join(*out, "omnis_manifest.json"), "templates/omnis_manifest.tmpl", ctx, fm)
		render("sql", filepath.Join(*out, "init.sql"), "templates/sql_ddl.tmpl", ctx, fm)
		
		shadowJSON, _ := json.MarshalIndent(contract, "", "  ")
		os.WriteFile(filepath.Join(*out, "shadow_schema.json"), shadowJSON, 0644)
		y, _ := yaml.Marshal(contract)
		os.WriteFile(filepath.Join(*out, "contracts.yaml"), y, 0644)
		docExp := &exporter.DocExporter{}
		docExp.Export(contract, resolved, *out)
	}
}

func render(name, outPath, tplPath string, ctx interface{}, fm template.FuncMap) {
	t := template.New(filepath.Base(tplPath)).Funcs(fm)
	_, err := t.ParseFS(tplFS, "templates/components/*.tmpl")
	if err != nil && !strings.Contains(err.Error(), "no files") { panic(err) }
	t, err = t.ParseFS(tplFS, tplPath)
	if err != nil { panic(err) }
	f, _ := os.Create(outPath)
	defer f.Close()
	if err := t.Execute(f, ctx); err != nil { panic(err) }
}