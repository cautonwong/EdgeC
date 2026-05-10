package backend

import (
	"contract-compiler-v2/internal/ir"
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"gopkg.in/yaml.v3"
)

type MetadataBackend struct {
	OutDir string
}

func (b *MetadataBackend) Emit(contract *ir.ContractIR) error {
	// [Nexus Fix] 严格对齐 V1 时代的 JSON 扁平化结构，确保 Python Actor 兼容性
	type tagExport struct {
		ID         uint32                 `json:"id"`
		Name       string                 `json:"name"`
		Attributes map[string]interface{} `json:"attributes"`
		Bindings   []interface{}          `json:"bindings"`
	}

	tags := make([]tagExport, 0, len(contract.Tags))
	for _, t := range contract.Tags {
		// 构造符合 Actor 预期的 bindings 结构
		var bindings []interface{}
		for _, b := range t.Bindings {
			binding := map[string]interface{}{
				"protocol":   strings.ToUpper(b.Protocol),
				"attributes": b.Attributes,
			}
			bindings = append(bindings, binding)
		}

		tags = append(tags, tagExport{
			ID:         t.ID,
			Name:       t.Name,
			Attributes: t.Attributes,
			Bindings:   bindings,
		})
	}

	m := map[string]interface{}{
		"product":      contract.ProductName,
		"signature":    contract.Signature,
		"connectivity": contract.Connectivity,
		"devices":      contract.Devices,
		"tags":         tags,
		"pipelines":    contract.Pipelines,
		"policies":     contract.Policies,
	}
	
	// 确保空数组不为 nil
	if m["pipelines"] == nil { m["pipelines"] = []string{} }
	if m["policies"] == nil { m["policies"] = []string{} }

	// 1. Omnis Manifest JSON
	mPath := filepath.Join(b.OutDir, "omnis_manifest.json")
	mFile, _ := os.Create(mPath)
	enc := json.NewEncoder(mFile)
	enc.SetIndent("", "  ")
	enc.Encode(m)
	mFile.Close()

	// 2. Emit YAML (供应给 C 核心初始化使用)
	yPath := filepath.Join(b.OutDir, "contracts.yaml")
	yFile, _ := os.Create(yPath)
	yEnc := yaml.NewEncoder(yFile)
	// 在 YAML 中我们保持原有的 ContractIR 结构
	yEnc.Encode(contract)
	yFile.Close()

	return nil
}
