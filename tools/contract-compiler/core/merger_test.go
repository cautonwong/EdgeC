package core

import (
	"os"
	"path/filepath"
	"testing"
)

func TestBuildContract_HardwareParsing(t *testing.T) {
	// 1. Setup temporary test directory
	tmpDir, err := os.MkdirTemp("", "contract_test_*")
	if err != nil {
		t.Fatal(err)
	}
	defer os.RemoveAll(tmpDir)

	// 2. Create mock schema (tags.yaml)
	schemaPath := filepath.Join(tmpDir, "tags.yaml")
	tagsData := []byte(`
tags:
  - name: VOLTAGE_PHASE_A
    id: 1001
    type: float
`)
	if err := os.WriteFile(schemaPath, tagsData, 0644); err != nil {
		t.Fatal(err)
	}

	// 3. Create mock product config (product.yaml) with Hardware definition
	prodPath := filepath.Join(tmpDir, "product.yaml")
	prodData := []byte(`
product: "Test_Product"
hardware:
  - type: bus
    id: 0
    label: "RS485_Main"
    transport: uds
    path: "/tmp/test.sock"
    devices:
      - protocol: ModbusRTU
        start_address: 1
        count: 5
enabled_tags:
  - VOLTAGE_PHASE_A
`)
	if err := os.WriteFile(prodPath, prodData, 0644); err != nil {
		t.Fatal(err)
	}

	// 4. Run BuildContract
	contract := BuildContract(schemaPath, prodPath, "", tmpDir)

	// 5. Verify Hardware structure
	if len(contract.Hardware) != 1 {
		t.Fatalf("Expected 1 hardware entry, got %d", len(contract.Hardware))
	}

	hw := contract.Hardware[0]
	if hw.ID != 0 {
		t.Errorf("Expected Bus ID 0, got %d", hw.ID)
	}
	if hw.Label != "RS485_Main" {
		t.Errorf("Expected Label 'RS485_Main', got '%s'", hw.Label)
	}

	// Verify attributes map
	transport, ok := hw.Attributes["transport"].(string)
	if !ok || transport != "uds" {
		t.Errorf("Expected transport 'uds', got %v", hw.Attributes["transport"])
	}

	// Verify nested devices list in attributes
	devsRaw, ok := hw.Attributes["devices"].([]interface{})
	if !ok {
		t.Fatalf("Hardware devices attribute is missing or not a list")
	}
	if len(devsRaw) != 1 {
		t.Fatalf("Expected 1 device config, got %d", len(devsRaw))
	}

	// Check device details
	dev1, ok := devsRaw[0].(map[string]interface{})
	if !ok {
		t.Fatalf("Device entry is not a map")
	}
	if dev1["protocol"] != "ModbusRTU" {
		t.Errorf("Expected protocol ModbusRTU, got %v", dev1["protocol"])
	}
}

func TestBuildContract_IncludesAndBindings(t *testing.T) {
	tmpDir, err := os.MkdirTemp("", "contract_inc_test_*")
	if err != nil { t.Fatal(err) }
	defer os.RemoveAll(tmpDir)

	// schema
	schemaPath := filepath.Join(tmpDir, "tags.yaml")
	os.WriteFile(schemaPath, []byte(`
tags:
  - name: TAG_A
    id: 100
    type: int
`), 0644)

	// binding file to include
	bindingPath := filepath.Join(tmpDir, "binding.yaml")
	os.WriteFile(bindingPath, []byte(`
protocol: MODBUS
mappings:
  - tag: TAG_A
    addr: 10
`), 0644)

	// product
	prodPath := filepath.Join(tmpDir, "product.yaml")
	os.WriteFile(prodPath, []byte(`
product: "Inc_Test"
includes:
  - binding.yaml
enabled_tags:
  - TAG_A
`), 0644)

	contract := BuildContract(schemaPath, prodPath, "", tmpDir)
	
	if len(contract.ActiveTags) != 1 {
		t.Fatalf("Expected 1 active tag")
	}
	
	tag := contract.ActiveTags[0]
	if len(tag.Bindings) != 1 {
		t.Fatalf("Expected 1 binding for TAG_A")
	}
	
	b := tag.Bindings[0]
	if b.ProtocolType != "MODBUS" {
		t.Errorf("Expected MODBUS, got %s", b.ProtocolType)
	}
	
	// Check binding attributes normalization
	addr := 0
	if val, ok := b.Attributes["addr"].(int); ok {
		addr = val
	}
	if addr != 10 {
		t.Errorf("Expected addr 10, got %v", b.Attributes["addr"])
	}
}

func TestBuildContract_LocalOverride(t *testing.T) {
	tmpDir, err := os.MkdirTemp("", "contract_over_test_*")
	if err != nil { t.Fatal(err) }
	defer os.RemoveAll(tmpDir)

	// 1. Base Product
	schemaPath := filepath.Join(tmpDir, "tags.yaml")
	os.WriteFile(schemaPath, []byte(`
tags:
  - name: TAG_BASE
    id: 1
    type: int
  - name: TAG_DISABLE
    id: 2
    type: int
`), 0644)

	prodPath := filepath.Join(tmpDir, "product.yaml")
	os.WriteFile(prodPath, []byte(`
product: "BaseProd"
parameters:
  - name: PARAM_A
    type: int
    default: 10
enabled_tags:
  - TAG_BASE
  - TAG_DISABLE
`), 0644)

	// 2. Local Extension
	localPath := filepath.Join(tmpDir, "local.yaml")
	os.WriteFile(localPath, []byte(`
parameters:
  - name: PARAM_A
    value: 999  # Override default
disabled_tags:
  - TAG_DISABLE # Mask this tag
configs:
  - name: NEW_CFG
    value: "Yes"
`), 0644)

	// 3. Build
	contract := BuildContract(schemaPath, prodPath, localPath, tmpDir)

	// 4. Verify Parameter Override
	paramFound := false
	for _, p := range contract.Params {
		if p.Name == "PARAM_A" {
			paramFound = true
			// The merger logic updates 'Default' or 'Value' depending on impl.
			// In merger.go: if val, exists := pm["value"]; exists { effectiveParams[i].Default = val }
			if p.Default != 999 {
				t.Errorf("Expected PARAM_A to be 999, got %v", p.Default)
			}
		}
	}
	if !paramFound { t.Error("PARAM_A missing") }

	// 5. Verify Tag Disabling
	for _, tag := range contract.ActiveTags {
		if tag.Name == "TAG_DISABLE" {
			t.Error("TAG_DISABLE should have been removed")
		}
	}

	// 6. Verify New Config
	cfgFound := false
	for _, c := range contract.Configs {
		if c.Name == "NEW_CFG" && c.Value == "Yes" {
			cfgFound = true
		}
	}
	if !cfgFound { t.Error("NEW_CFG missing") }
}
