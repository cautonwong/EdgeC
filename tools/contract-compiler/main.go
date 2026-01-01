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
	Modbus     []protocols.ResolvedBinding
	Can        []protocols.ResolvedBinding
	PMap       map[string][]protocols.ResolvedBinding
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

func lower(s string) string { return strings.ToLower(s) }
func list(ss ...string) []string { return ss }

func main() {
	schema := flag.String("schema", "", "path to schema file")
	product := flag.String("product", "", "path to product file")
	local := flag.String("local", "", "path to local extension")
	out := flag.String("out", "", "output directory")
	base := flag.String("base", "", "project base directory")
	target := flag.String("target", "master", "build target (master or slave)")
	flag.Parse()

	contract := core.BuildContract(*schema, *product, *local, *base)
	activeMap := make(map[string]bool)
	for _, t := range contract.ActiveTags {
		activeMap[t.Name] = true
	}
	resolved := protocols.Resolve(contract.ActiveTags)

	ctx := TplCtx{
		ActiveTags: contract.ActiveTags,
		AllTags:    contract.AllTags,
		ActiveMap:  activeMap,
		Modbus:     resolved["MODBUS"],
		Can:        resolved["CAN"],
		PMap:       resolved,
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
	}

	if *target == "master" {
		render("gen", filepath.Join(*out, "contracts.h"), masterHTpl, ctx, funcMap)
		render("gen", filepath.Join(*out, "contracts.c"), masterCTpl, ctx, funcMap)
		yData, _ := yaml.Marshal(contract)
		os.WriteFile(filepath.Join(*out, "contracts.yaml"), yData, 0644)
	} else {
		render("gen", filepath.Join(*out, "slave_data.h"), slaveHTpl, ctx, funcMap)
	}
	fmt.Printf("Go Build Success [%s]: %s\n", *target, contract.Product)
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
