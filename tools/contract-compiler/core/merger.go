package core

import (
	"contract-compiler/model"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"gopkg.in/yaml.v3"
)

// deepMerge recursively merges source map into target map.
func deepMerge(target, source map[string]interface{}) {
	for k, v := range source {
		if srcMap, ok := v.(map[string]interface{}); ok {
			if tgtVal, exists := target[k]; exists {
				if tgtMap, ok := tgtVal.(map[string]interface{}); ok {
					deepMerge(tgtMap, srcMap)
					continue
				}
			}
		}
		target[k] = v
	}
}

// processIncludes loads tags from included files (Domains).
func processIncludes(includePaths []string, baseDir string, rawTags map[string]map[string]interface{}, effectiveScenarios *[]string, seen map[string]bool) {
	for _, inc := range includePaths {
		absPath := inc
		if !filepath.IsAbs(inc) && baseDir != "" {
			absPath = filepath.Join(baseDir, inc)
		}
		if seen[absPath] {
			continue
		}
		seen[absPath] = true
		bData, err := os.ReadFile(absPath)
		if err != nil {
			continue
		}
		var raw map[string]interface{}
		yaml.Unmarshal(bData, &raw)
		
		// Recursively process nested includes
		var subIncludes []string
		if si, ok := raw["include"].([]interface{}); ok {
			for _, item := range si {
				if s, ok := item.(string); ok {
					subIncludes = append(subIncludes, s)
				}
			}
		}
		if len(subIncludes) > 0 {
			processIncludes(subIncludes, filepath.Dir(absPath), rawTags, effectiveScenarios, seen)
		}

		// Load tags/domain definitions
		if tagsList, ok := raw["tags"].([]interface{}); ok {
			for _, item := range tagsList {
				if m, ok := item.(map[string]interface{}); ok {
					if name, ok := m["name"].(string); ok {
						if _, exists := rawTags[name]; !exists {
							rawTags[name] = m
						} else {
							// Basic merge if already exists (shouldn't happen for domains usually)
							for k, v := range m {
								if k != "attributes" {
									rawTags[name][k] = v
								}
							}
						}
					}
				}
			}
		}

		// Load bindings/mappings
		proto, _ := raw["protocol"].(string)
		mList, _ := raw["mappings"].([]interface{})
		if len(mList) == 0 {
			mList, _ = raw["map"].([]interface{})
		}
		for _, m := range mList {
			if mm, ok := m.(map[string]interface{}); ok {
				if tagName, ok := mm["tag"].(string); ok {
					injectBinding(rawTags, tagName, proto, mm)
				}
			}
		}
	}
}

func applyDefaults(rawTags map[string]map[string]interface{}, rules []model.DefaultRule) {
	for _, tag := range rawTags {
		// Get tag traits
		var traits []string
		if tVal, ok := tag["traits"].([]interface{}); ok {
			for _, t := range tVal {
				if s, ok := t.(string); ok {
					traits = append(traits, s)
				}
			}
		}

		for _, rule := range rules {
			match := false
			if strings.HasPrefix(rule.Match, "traits:") {
				targetTrait := strings.TrimPrefix(rule.Match, "traits:")
				for _, t := range traits {
					if t == targetTrait {
						match = true
						break
					}
				}
			}
			// Future: Add more matchers (e.g. range)

			if match {
				if tag["attributes"] == nil {
					tag["attributes"] = make(map[string]interface{})
				}
				tgtAttr := tag["attributes"].(map[string]interface{})
				deepMerge(tgtAttr, rule.Attributes)
			}
		}
	}
}

func processTagOverrides(rawTags map[string]map[string]interface{}, overrides []interface{}) {
	for _, item := range overrides {
		m, ok := item.(map[string]interface{})
		if !ok { continue }
		
		// Match by ID or Name
		var targetTag map[string]interface{}
		
		if idVal, ok := m["id"]; ok {
			// Find tag by ID
			targetID := uint32(0)
			switch v := idVal.(type) {
			case int: targetID = uint32(v)
			case float64: targetID = uint32(v)
			}
			
			for _, t := range rawTags {
				if tid, ok := t["id"]; ok {
					currID := uint32(0)
					switch v := tid.(type) {
					case int: currID = uint32(v)
					case float64: currID = uint32(v)
					}
					if currID == targetID {
						targetTag = t
						break
					}
				}
			}
		} else if nameVal, ok := m["name"].(string); ok {
			targetTag = rawTags[nameVal]
		}

		if targetTag != nil {
			// Apply Overrides
			for k, v := range m {
				if k == "id" || k == "name" { continue } // Identity cannot be changed easily
				if k == "attributes" {
					if targetTag["attributes"] == nil {
						targetTag["attributes"] = make(map[string]interface{})
					}
					if srcAttr, ok := v.(map[string]interface{}); ok {
						deepMerge(targetTag["attributes"].(map[string]interface{}), srcAttr)
					}
				} else {
					targetTag[k] = v
				}
			}
		}
	}
}

func BuildContract(schemaPath, productPath, localPath, baseDir string) *model.EffectiveContract {
	rawTags := make(map[string]map[string]interface{})
	
	// 1. Load Product Config (Layer 2)
	var pc model.ProductConfig
	pData, _ := os.ReadFile(productPath)
	yaml.Unmarshal(pData, &pc)
	
	effectiveScenarios := []string{}
	seenFiles := make(map[string]bool)
	
	// 2. Process Includes (Layer 1: Domains & Bindings)
	allIncludes := append(pc.Includes, pc.Include...)
	
	// If local manifest includes extra files, add them too
	if localPath != "" {
		lData, _ := os.ReadFile(localPath)
		var localMap map[string]interface{}
		yaml.Unmarshal(lData, &localMap)
		if inc, ok := localMap["include"].([]interface{}); ok {
			for _, i := range inc {
				if s, ok := i.(string); ok {
					allIncludes = append(allIncludes, s)
				}
			}
		}
	}
	
	// Recursively load all assets
	processIncludes(allIncludes, baseDir, rawTags, &effectiveScenarios, seenFiles)

	// 3. Apply Product Defaults (Injection)
	applyDefaults(rawTags, pc.Defaults)

	// 4. Apply Product Overrides
	processTagOverrides(rawTags, pc.Tags)

	// 5. Load Manifest (Layer 3) & Apply Manifest Overrides
	effectiveHardware := pc.Hardware
	effectivePipelines := pc.Pipelines
	effectiveConnectivity := pc.Connectivity
	effectiveDevices := pc.Devices
	effectiveConfigs := pc.Configs
	effectiveParams := pc.Params
	effectiveUI := pc.UI

	if localPath != "" {
		lData, _ := os.ReadFile(localPath)
		var mStruct model.ProductConfig
		yaml.Unmarshal(lData, &mStruct)

		if mStruct.Connectivity != nil { effectiveConnectivity = mStruct.Connectivity }
		if mStruct.Devices != nil { effectiveDevices = mStruct.Devices }
		if mStruct.Hardware != nil { effectiveHardware = mStruct.Hardware } // TODO: Deep merge hardware if needed
		if mStruct.Pipelines != nil { effectivePipelines = mStruct.Pipelines } // Usually replace pipelines
		
		// Apply Manifest Tag Overrides
		processTagOverrides(rawTags, mStruct.Tags)
		
		// Also support legacy 'enabled_tags' for overrides if needed, but 'tags' is preferred in V5
		processTagOverrides(rawTags, mStruct.EnabledTags)
	}

	// 6. Filter Active Tags (Implicit Linking logic can be added here)
	// For now, we activate ALL tags defined in the domain files, as they are explicitly included.
	// In V6, we might want to filter based on usage in Pipelines/UI.
	var activeTags []model.TagDef
	var allTags []model.TagDef
	
	for _, rm := range rawTags {
		var t model.TagDef
		b, _ := yaml.Marshal(rm)
		yaml.Unmarshal(b, &t)
		
		// Ensure attributes map is populated correctly from the raw map
		// yaml.Unmarshal sometimes misses dynamic map[string]interface{} inside struct if not tagged perfectly
		// So we manually copy the merged attributes back to t.Attributes
		if attr, ok := rm["attributes"].(map[string]interface{}); ok {
			t.Attributes = attr
		}

		allTags = append(allTags, t)
		// For V5, we assume if it's in the domain file, it's active.
		// Real implicit linking would require scanning Pipelines & UI.
		activeTags = append(activeTags, t)
	}

	// Sort for deterministic output
	sort.Slice(activeTags, func(i, j int) bool { return activeTags[i].ID < activeTags[j].ID })

	return &model.EffectiveContract{
		Product: pc.ProductName, ActiveTags: activeTags, AllTags: allTags,
		Configs: effectiveConfigs, Params: effectiveParams, Hardware: effectiveHardware,
		Connectivity: effectiveConnectivity, Devices: effectiveDevices,
		Pipelines: effectivePipelines, Policies: pc.Policies, UI: effectiveUI, Scenarios: effectiveScenarios,
	}
}

func loadYamlToRawMap(path, key string, target map[string]map[string]interface{}) {
	data, err := os.ReadFile(path)
	if err != nil {
		return
	}
	var raw map[string]interface{}
	yaml.Unmarshal(data, &raw)
	if list, ok := raw[key].([]interface{}); ok {
		for _, item := range list {
			if m, ok := item.(map[string]interface{}); ok {
				if name, ok := m["name"].(string); ok {
					target[name] = m
				}
			}
		}
	}
}

func injectBinding(rawTags map[string]map[string]interface{}, tagName, proto string, attr map[string]interface{}) {
	tag, ok := rawTags[tagName]
	if !ok {
		return
	}
	bindings, _ := tag["bindings"].([]interface{})
	upProto := strings.ToUpper(proto)
	if upProto == "" {
		upProto = "MODBUSRTU"
	}
	newB := make(map[string]interface{})
	newB["protocol"] = upProto
	newB["tag"] = tagName
	normalizedAttr := make(map[string]interface{})
	for k, v := range attr {
		if k == "tag" {
			continue
		}
		newK := k
		switch strings.ToLower(k) {
		case "scale":
			newK = "scaler"
		case "address":
			newK = "addr"
		}
		normalizedAttr[newK] = v
	}
	newB["attributes"] = normalizedAttr
	found := false
	for i, b := range bindings {
		if bm, ok := b.(map[string]interface{}); ok {
			p, _ := bm["protocol"].(string)
			if strings.EqualFold(p, upProto) {
				bindings[i] = newB
				found = true
				break
			}
		}
	}
	if !found {
		bindings = append(bindings, newB)
	}
	tag["bindings"] = bindings
}