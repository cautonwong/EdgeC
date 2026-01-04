package main

import (
	"contract-compiler/core"
	"contract-compiler/model"
	"contract-compiler/protocols"
	_ "embed"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"text/template"

	"gopkg.in/yaml.v3"
)

//go:embed templates/master_h.tmpl
var masterHTpl string

//go:embed templates/master_c.tmpl
var masterCTpl string

//go:embed templates/slave_h.tmpl
var slaveHTpl string

type TplCtx struct {
	ActiveTags []model.TagDef
	AllTags    []model.TagDef
	ActiveMap  map[string]bool
	PMap       map[string][]protocols.ResolvedBinding
	Hashes     map[string]core.ProtocolHashInfo
	Product    string
}

func (c TplCtx) IsActive(name string) bool { return c.ActiveMap[name] }
func (c TplCtx) GetUIAttr(t model.TagDef, key string) interface{} {
	if ui, ok := t.Attributes["ui"].(map[string]interface{}); ok {
		val := ui[key]
		switch v := val.(type) {
		case int: return float64(v)
		case float64: return v
		}
		return val
	}
	return nil
}
func (c TplCtx) CType(t string) string {
	switch t {
	case "float": return "float"
	case "double": return "double"
	case "int": return "int32_t"
	default: return "uint16_t"
	}
}

func main() {
	schema := flag.String("schema", "", "path to schema file")
	product := flag.String("product", "", "path to product file")
	local := flag.String("local", "", "path to local extension")
	out := flag.String("out", "", "output directory")
	base := flag.String("base", "", "project base directory")
	target := flag.String("target", "master", "build target (master or slave)")
	flag.Parse()

	contract := core.BuildContract(*schema, *product, *local, *base)
	actMap := make(map[string]bool)
	for _, t := range contract.ActiveTags {
		actMap[t.Name] = true
	}
	resolved := protocols.Resolve(contract.ActiveTags)

	// 计算各协议的完美哈希
	hashes := make(map[string]core.ProtocolHashInfo)
	for proto, rbs := range resolved {
		entries := make([]core.HashEntry, 0, len(rbs))
		fmt.Printf("[Compiler] Protocol %s: Found %d bindings\n", proto, len(rbs))
		for _, rb := range rbs {
			keyStr := ""
			switch proto {
			case "MODBUS": keyStr = fmt.Sprintf("%d", rb.Addr)
			case "IEC104": keyStr = fmt.Sprintf("%d", rb.IOA)
			case "DLT645": keyStr = rb.DI
			case "DLT698": keyStr = rb.OAD
			case "DLMS": keyStr = rb.OBIS
			case "CAN": continue // CAN 暂不使用完美哈希
			}
		
			rawKey := core.ParseProtocolKey(proto, keyStr)
			// 修正：只要 keyStr 不为空，即便 rawKey 是 0 (Modbus 合法地址)，也应包含
			if keyStr == "" {
				continue 
			}

			entries = append(entries, core.HashEntry{
				RawKey:  rawKey,
				StrKey:  keyStr,
				TagID:   rb.TagID,
				TagName: rb.TagName,
				Scale:   rb.Scale,
				ByteLen: rb.ByteLen,
				Access:  rb.Access,
			})
		}
		if len(entries) > 0 {
			hashes[proto] = core.GeneratePerfectHash(proto, entries)
			fmt.Printf("[Compiler] Protocol %s: Generated hash for %d entries (IsPerfect: %v)\n", 
				proto, len(entries), hashes[proto].IsPerfect)
		}
	}

	ctx := TplCtx{
		ActiveTags: contract.ActiveTags,
		AllTags:    contract.AllTags,
		ActiveMap:  actMap,
		PMap:       resolved,
		Hashes:     hashes,
		Product:    contract.Product,
	}

	os.MkdirAll(*out, 0755)
	funcMap := template.FuncMap{
		"lower": strings.ToLower,
		"list": func(ss ...string) []string { return ss },
		"default": func(d interface{}, v interface{}) interface{} {
			if v == nil { return d }
			return v
		},
		"hex": func(v uint64) string { return fmt.Sprintf("0x%X", v) },
		"strings": func() interface{} { return strings.ToUpper }, // 简单包装以支持 strings.EqualFold 逻辑
		"equalFold": strings.EqualFold,
		"parseKey": core.ParseProtocolKey,
	}

	if *target == "master" {
		render("gen", filepath.Join(*out, "contracts.h"), masterHTpl, ctx, funcMap)
		render("gen", filepath.Join(*out, "contracts.c"), masterCTpl, ctx, funcMap)
		render("gen", filepath.Join(*out, "contracts_slave.h"), slaveHTpl, ctx, funcMap)
		yData, _ := yaml.Marshal(contract)
		os.WriteFile(filepath.Join(*out, "contracts.yaml"), yData, 0644)
	} else {
		render("gen", filepath.Join(*out, "contracts_slave.h"), slaveHTpl, ctx, funcMap)
	}
	fmt.Printf("Go Build Success [%s]: %s (v4.5-PerfectHash-Secure)\n", *target, contract.Product)
}

func render(name, path, tpl string, ctx interface{}, fm template.FuncMap) {
	t := template.Must(template.New(name).Funcs(fm).Parse(tpl))
	f, err := os.Create(path)
	if err != nil {
		fmt.Printf("Error creating file %s: %v\n", path, err)
		return
	}
	defer f.Close()
	t.Execute(f, ctx)
}